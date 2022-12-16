#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
RegAllocator::RegAllocator(frame::Frame* frame, std::unique_ptr<cg::AssemInstr> assem_instr)
    : frame_(frame), assem_instr_(std::move(assem_instr)) {
  precolored_ = new WorkList;
  initial_ = new WorkList;
  simplify_list_ = new WorkList;
  freeze_list_ = new WorkList;
  spill_list_ = new WorkList;
  spilled_nodes_ = new NodesList;
  coalesced_nodes_ = new NodesList;
  colored_nodes_ = new NodesList;
  select_stack_ = new SelectStack;
  coalesced_moves_ = new MoveList;
  constrained_moves_ = new MoveList;
  frozen_moves_ = new MoveList;
  //worklist_moves_ = new MoveList;
  active_moves_ = new MoveList;
}

RegAllocator::~RegAllocator() { }

void RegAllocator::RegAlloc() {
  fprintf(stdout, "RegAlloc\n");
  LivenessAnalysis();
  Build();
  MakeWorkList();
  while(!simplify_list_->Empty() || !worklist_moves_->Empty()
    || !freeze_list_->Empty() || !spill_list_->Empty()) {
    if(!simplify_list_->Empty())        Simplify();
    else if(!worklist_moves_->Empty())  Coalesce();
    else if(!freeze_list_->Empty())     Freeze();
    else if(!spill_list_->Empty())      SelectSpill();
  }
  AssignColors();
  if(!spilled_nodes_->Empty()) {
    RewriteProgram();
    RegAlloc();
  }
}

void RegAllocator::LivenessAnalysis() {
  fg_ = new fg::FlowGraphFactory(assem_instr_->GetInstrList());
  fg_->AssemFlowGraph();
  live_ = new live::LiveGraphFactory(fg_->GetFlowGraph());
  live_->Liveness();
}

void RegAllocator::Build() {
  precolored_->Clear();
  initial_->Clear();
  simplify_list_->Clear();
  freeze_list_->Clear();
  spill_list_->Clear();

  spilled_nodes_->Clear();
  coalesced_nodes_->Clear();
  colored_nodes_->Clear();
  select_stack_->Clear();

  coalesced_moves_->Clear();
  constrained_moves_->Clear();
  frozen_moves_->Clear();
  active_moves_->Clear();

  degree_map_.clear();
  movelist_map_.clear();
  alias_map_.clear();
  color_map_.clear();

  live::LiveGraph live_graph_ = live_->GetLiveGraph();
  temp::TempList* reg_ = reg_manager->Registers();
  reg_->Append(reg_manager->StackPointer());
  const std::list<temp::Temp*>& reg_list_ = reg_->GetList();
  for(live::INodePtr node : live_graph_.interf_graph->Nodes()->GetList()) {
    if(std::find(reg_list_.begin(), reg_list_.end(), node->NodeInfo()) != reg_list_.end()) {
      precolored_->Append(node);
      color_map_[node] = col::ColorList::findColor(node->NodeInfo());
    }
    else  initial_->Append(node);
    degree_map_[node] = node->Degree();
    alias_map_[node] = node;
    movelist_map_[node] = new MoveList;
  }
  worklist_moves_ = live_graph_.moves;
  for(std::pair<live::INodePtr, live::INodePtr> move : live_graph_.moves->GetList()) {
    movelist_map_[move.first]->Append(move.first, move.second);
    movelist_map_[move.second]->Append(move.first, move.second);
  }
}

void RegAllocator::MakeWorkList() {
  for(live::INodePtr node : initial_->GetList()) {
    if(degree_map_[node] >= reg_manager->RegNum()) {
      spill_list_->Append(node);
    } else if(MoveRelated(node)) {
      freeze_list_->Append(node);
    } else {
      simplify_list_->Append(node);
    }
  }
}

void RegAllocator::Simplify() {
  live::INodePtr node = simplify_list_->Pop();
  select_stack_->Push(node);
  NodesListPtr adjList = Adjacent(node);
  for(live::INodePtr m : adjList->GetList())
    DecrementDegree(m);
}

void RegAllocator::Coalesce() {
  std::pair<live::INodePtr, live::INodePtr> move = worklist_moves_->Pop();
  live::INodePtr x = GetAlias(move.first);
  live::INodePtr y = GetAlias(move.second);
  std::pair<live::INodePtr, live::INodePtr> m;
  if(precolored_->Contain(y))   m = std::make_pair(y, x);
  else                          m = std::make_pair(x, y);
  live::INodePtr u = m.first, v = m.second;
  if(u == v) {
    coalesced_moves_->Push(m);
    AddWorkList(u);
  } else if(precolored_->Contain(v) || u->Adj(v)) {
    constrained_moves_->Push(m);
    AddWorkList(u);
    AddWorkList(v);
  } else if(precolored_->Contain(u) && Geroge(u, v)
    || !precolored_->Contain(u) && Briggs(u, v)) {
    coalesced_moves_->Push(m);
    Combine(u, v);
    AddWorkList(u);
  } else {
    active_moves_->Push(m);
  }
}

void RegAllocator::Freeze() {
  live::INodePtr node = freeze_list_->Pop();
  simplify_list_->Append(node);
  FreezeMoves(node);
}

void RegAllocator::SelectSpill() {
  live::INodePtr m;
  int cost = 0;
  for(live::INodePtr t : spill_list_->GetList())
    if(degree_map_[t] > cost) { m = t; cost = degree_map_[t]; }
  spill_list_->DeleteNode(m);
  simplify_list_->Append(m);
  FreezeMoves(m);
}

void RegAllocator::AssignColors() {
  while(!select_stack_->Empty()) {
    live::INodePtr n = select_stack_->Pop();
    col::ColorList* okColors = col::ColorList::NewColorList();
    NodesListPtr adjList = n->Pred()->Union(n->Succ());
    for(live::INodePtr w : adjList->GetList())
      if(colored_nodes_->Union(precolored_)->Contain(GetAlias(w)))
        okColors->DeleteColor(color_map_[GetAlias(w)]);
    if(okColors->Empty()) {
      spilled_nodes_->Append(n);
    } else {
      colored_nodes_->Append(n);
      color_map_[n] = okColors->getOne();
    }
  }
  for(live::INodePtr n : coalesced_nodes_->GetList())
    color_map_[n] = color_map_[GetAlias(n)];
}

void RegAllocator::RewriteProgram() {
  for(live::INodePtr v : spilled_nodes_->GetList()) {
    fprintf(stdout, "t%d must be spilled\n", v->NodeInfo()->Int());
    frame::Access* newAccess = frame_->AllocLocal(true);
    int consti_ = static_cast<tree::ConstExp*>(static_cast<tree::BinopExp*>(static_cast<tree::MemExp*>(newAccess->toExp(new tree::TempExp(reg_manager->FramePointer())))->exp_)->right_)->consti_;
    const std::list<assem::Instr *>& instr_list_ = assem_instr_->GetInstrList()->GetList();
    std::list<assem::Instr *>::const_iterator itr = instr_list_.begin();
    while(itr != instr_list_.end()) {
      if((*itr)->Use()->Contain(v->NodeInfo())) {
        fprintf(stdout, "(Use)Rewrite ");
        //(*itr)->Print(stdout, reg_manager->temp_map_);
        temp::Temp* newTemp1 = temp::TempFactory::NewTemp();
        temp::Temp* newTemp2 = temp::TempFactory::NewTemp();
        (*itr)->Use()->Replace(v->NodeInfo(), newTemp2);
        assem::Instr* newInstr1 = new assem::OperInstr("leaq " + frame_->name_->Name() + "_framesize(`s0), `d0", new temp::TempList({ newTemp1 }), new temp::TempList({ reg_manager->StackPointer() }), nullptr);
        assem::Instr* newInstr2 = new assem::MoveInstr("movq " + std::to_string(consti_) + "(`s0), `d0", new temp::TempList({ newTemp2 }), new temp::TempList({ newTemp1 }));
        fprintf(stdout, "insert ");
        //newInstr->Print(stdout, reg_manager->temp_map_);
        assem_instr_->GetInstrList()->Insert(itr, newInstr1);
        assem_instr_->GetInstrList()->Insert(itr, newInstr2);
      }
      if((*itr)->Def()->Contain(v->NodeInfo())) {
        fprintf(stdout, "(Def)Rewrite ");
        //(*itr)->Print(stdout, reg_manager->temp_map_);
        temp::Temp* newTemp1 = temp::TempFactory::NewTemp();
        temp::Temp* newTemp2 = temp::TempFactory::NewTemp();
        (*itr)->Def()->Replace(v->NodeInfo(), newTemp2);
        assem::Instr* newInstr1 = new assem::OperInstr("leaq " + frame_->name_->Name() + "_framesize(`s0), `d0", new temp::TempList({ newTemp1 }), new temp::TempList({ reg_manager->StackPointer() }), nullptr);
        assem::Instr* newInstr2 = new assem::MoveInstr("movq `s0, " + std::to_string(consti_) + "(`s1)", nullptr, new temp::TempList({newTemp2, newTemp1}));
        fprintf(stdout, "insert ");
        //newInstr->Print(stdout, reg_manager->temp_map_);
        ++itr;
        assem_instr_->GetInstrList()->Insert(itr, newInstr1);
        assem_instr_->GetInstrList()->Insert(itr, newInstr2);
      } else {
        ++itr;
      }
    }
  }
}

NodesListPtr RegAllocator::Adjacent(live::INodePtr node) const {
  NodesListPtr adjList = node->Pred()->Union(node->Succ());
  NodesListPtr removedList = select_stack_->Union(coalesced_nodes_);
  return adjList->Diff(removedList);
}

MoveList* RegAllocator::NodeMoves(live::INodePtr node) {
  return movelist_map_[node]->Intersect(active_moves_->Union(worklist_moves_));
}

bool RegAllocator::MoveRelated(live::INodePtr node) {
  return !(NodeMoves(node)->Empty());
}

void RegAllocator::DecrementDegree(live::INodePtr node) {
  int degree = degree_map_[node];
  degree_map_[node]--;
  if(degree == reg_manager->RegNum()) {
    NodesListPtr enable_moves_ = Adjacent(node);
    enable_moves_->Append(node);
    EnableMoves(enable_moves_);
    spill_list_->DeleteNode(node);
    if(MoveRelated(node))   freeze_list_->Append(node);
    else    simplify_list_->Append(node);
  }
}

void RegAllocator::EnableMoves(live::INodeListPtr nodes) {
  for(live::INodePtr n : nodes->GetList()) {
    for(std::pair<live::INodePtr, live::INodePtr> m : NodeMoves(n)->GetList()) {
      if(active_moves_->Contain(m)) {
        active_moves_->Delete(m.first, m.second);
        worklist_moves_->Append(m.first, m.second);
      }
    }
  }
}

void RegAllocator::AddWorkList(live::INodePtr node) {
  if(!precolored_->Contain(node) && !MoveRelated(node) && degree_map_[node] < reg_manager->RegNum()) {
    freeze_list_->DeleteNode(node);
    simplify_list_->Append(node);
  }
}

bool RegAllocator::Briggs(live::INodePtr u, live::INodePtr v) {
  NodesListPtr combineAdjList = Adjacent(u)->Union(Adjacent(v));
  int k = 0, K = reg_manager->RegNum();
  for(live::INodePtr node : combineAdjList->GetList())
    if(degree_map_[node] >= K)  ++k;
  return (k < K);
}

bool RegAllocator::Geroge(live::INodePtr u, live::INodePtr v) {
  NodesListPtr adjList = Adjacent(v);
  for(live::INodePtr t : adjList->GetList())
    if(!OK(t, u))   return false;
  return true;
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return degree_map_[t] < reg_manager->RegNum() || precolored_->Contain(t) || t->GoesTo(r);
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  if(freeze_list_->Contain(v))   freeze_list_->DeleteNode(v);
  else        spill_list_->DeleteNode(v);
  coalesced_nodes_->Append(v);
  alias_map_[v] = u;
  movelist_map_[u] = movelist_map_[u]->Union(movelist_map_[v]);
  NodesListPtr enable_moves_ = new NodesList;
  enable_moves_->Append(v);
  EnableMoves(enable_moves_);
  NodesListPtr adjList = Adjacent(v);
  live::LiveGraph live_graph_ = live_->GetLiveGraph();
  for(live::INodePtr t : adjList->GetList()) {
    if(t != u && !t->Adj(u)) {
      live_graph_.interf_graph->AddEdge(t, u);
      live_graph_.interf_graph->AddEdge(u, t);
      if(!precolored_->Contain(u))    degree_map_[u]++;
      if(!precolored_->Contain(t))    degree_map_[t]++;
    }
    DecrementDegree(t);
  }
  if(degree_map_[u] >= reg_manager->RegNum() && freeze_list_->Contain(u)) {
    freeze_list_->DeleteNode(u);
    spill_list_->Append(u);
  }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr node) {
  if(coalesced_nodes_->Contain(node))   return GetAlias(alias_map_[node]);
  return node;
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
  MoveList* node_moves_ = NodeMoves(u);
  live::INodePtr v;
  for(auto m : node_moves_->GetList()) {
    v = ((GetAlias(m.second) == GetAlias(u))? GetAlias(m.first) : GetAlias(m.second));
    active_moves_->Delete(m.first, m.second);
    frozen_moves_->Append(m.first, m.second);
    if(NodeMoves(v)->Empty() && degree_map_[v] < reg_manager->RegNum()) {
      freeze_list_->DeleteNode(v);
      simplify_list_->Append(v);
    }
  }
}

std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
  std::unique_ptr<ra::Result> result = std::make_unique<ra::Result>();
  result->il_ = assem_instr_.get()->GetInstrList();
  result->coloring_ = temp::Map::Empty();
  for(auto coloring : color_map_)
    result->coloring_->Enter((coloring.first)->NodeInfo(), coloring.second);

  auto itr = result->il_->GetOriginalList().begin();
  while(itr != result->il_->GetOriginalList().end()) {
    assem::Instr* instr = *itr;
    if(typeid(*instr) == typeid(assem::MoveInstr)){
      assem::MoveInstr* moveInstr = static_cast<assem::MoveInstr*>(instr);
      if(moveInstr->assem_ == "movq `s0, `d0") {
        temp::Temp* src = moveInstr->src_->GetList().front();
        temp::Temp* dst = moveInstr->dst_->GetList().front();
        if(result->coloring_->Look(src) == result->coloring_->Look(dst)) {
          itr = result->il_->GetOriginalList().erase(itr);
          continue;
        }
      }
    }
    ++itr;
  }
  //fg_->GetFlowGraph()->Show(stdout, fg_->GetFlowGraph()->Nodes(), [&](assem::Instr* instr){ instr->Print(stdout, result->coloring_); });
  return result;
}
} // namespace ra