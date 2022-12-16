#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

bool MoveList::Contain(std::pair<INodePtr, INodePtr> move) {
  return std::find(move_list_.begin(), move_list_.end(), move) != move_list_.end();
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

bool MoveList::Empty() const {
  return move_list_.empty();
}

void MoveList::Push(std::pair<INodePtr, INodePtr> m) {
  move_list_.push_back(m);
}

std::pair<INodePtr, INodePtr> MoveList::Pop() {
  std::pair<INodePtr, INodePtr> move = move_list_.back();
  move_list_.pop_back();
  return move;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  for(const fg::FNodePtr& node : flowgraph_->Nodes()->GetList()) {
    in_->Enter(node, new temp::TempList);
    out_->Enter(node, new temp::TempList);
  }
  bool flag = false;
  while(!flag) {
    flag = true;
    for(const fg::FNodePtr& node : flowgraph_->Nodes()->GetList()) {
      temp::TempList* in = new temp::TempList;
      temp::TempList* out = new temp::TempList;
      for(const fg::FNodePtr& succ : node->Succ()->GetList())
        out->Union(in_->Look(succ));
      in->Union(node->NodeInfo()->Use());
      in->Union(out->Remove(node->NodeInfo()->Def()));
      if(!in->Equal(in_->Look(node)) || !out->Equal(out_->Look(node)))
        flag = false;
      in_->Set(node, in);
      out_->Set(node, out);
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  INodePtr spInodePtr = live_graph_.interf_graph->NewNode(reg_manager->StackPointer());
  temp_node_map_->Enter(reg_manager->StackPointer(), spInodePtr);
  for(temp::Temp* t : reg_manager->Registers()->GetList()) {
    INodePtr ptr = live_graph_.interf_graph->NewNode(t);
    temp_node_map_->Enter(t, ptr);
    live_graph_.interf_graph->AddEdge(ptr, spInodePtr);
    live_graph_.interf_graph->AddEdge(spInodePtr, ptr);
  }
  for(const fg::FNodePtr& node : flowgraph_->Nodes()->GetList()) {
    for(temp::Temp* t : node->NodeInfo()->Use()->GetList()) {
      if(temp_node_map_->Look(t))   continue;
      INodePtr ptr = live_graph_.interf_graph->NewNode(t);
      temp_node_map_->Enter(t, ptr);
      live_graph_.interf_graph->AddEdge(ptr, spInodePtr);
      live_graph_.interf_graph->AddEdge(spInodePtr, ptr);
    }
    for(temp::Temp* t : node->NodeInfo()->Def()->GetList()) {
      if(temp_node_map_->Look(t))   continue;
      INodePtr ptr = live_graph_.interf_graph->NewNode(t);
      temp_node_map_->Enter(t, ptr);
      live_graph_.interf_graph->AddEdge(ptr, spInodePtr);
      live_graph_.interf_graph->AddEdge(spInodePtr, ptr);
    }
  }
  for(temp::Temp* t1 : reg_manager->Registers()->GetList()) {
    for(temp::Temp* t2 : reg_manager->Registers()->GetList()) {
      if(t1 != t2) {
        live_graph_.interf_graph->AddEdge(temp_node_map_->Look(t1), temp_node_map_->Look(t2));
        live_graph_.interf_graph->AddEdge(temp_node_map_->Look(t2), temp_node_map_->Look(t1));
      }
    }
  }
  for(const fg::FNodePtr& node : flowgraph_->Nodes()->GetList()) {
    if(typeid(*(node->NodeInfo())) != typeid(assem::MoveInstr)) {
      for(temp::Temp* a : node->NodeInfo()->Def()->GetList()) {
        for(temp::Temp* b : out_->Look(node)->GetList()) {
          if(a == b)  continue;
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(a), temp_node_map_->Look(b));
          live_graph_.interf_graph->AddEdge(temp_node_map_->Look(b), temp_node_map_->Look(a));
        }
      }
    } else {
      for(temp::Temp* a : node->NodeInfo()->Def()->GetList()) {
        for(temp::Temp* c : node->NodeInfo()->Use()->GetList()) {
          if(live_graph_.moves->Contain(temp_node_map_->Look(a), temp_node_map_->Look(c)))  continue;
          live_graph_.moves->Append(temp_node_map_->Look(a), temp_node_map_->Look(c));
        }
        assem::MoveInstr* moveInstr = static_cast<assem::MoveInstr*>(node->NodeInfo());
        if(moveInstr->assem_ == "movq `s0, `d0") {
          for(temp::Temp* b : out_->Look(node)->GetList()) {
            temp::Temp* c = moveInstr->src_->NthTemp(0);
            if(a == b || c == b)   continue;
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(a), temp_node_map_->Look(b));
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(b), temp_node_map_->Look(a));
          }
        } else {
          for(temp::Temp* b : out_->Look(node)->GetList()) {
            if(a == b)   continue;
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(a), temp_node_map_->Look(b));
            live_graph_.interf_graph->AddEdge(temp_node_map_->Look(b), temp_node_map_->Look(a));
          }
        }
      }
    }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
  fprintf(stdout, "%s\n", "LivenessAnalysis");
  live_graph_.interf_graph->Show(stdout, live_graph_.interf_graph->Nodes(), [&](temp::Temp* t){
    auto p = reg_manager->temp_map_->Look(t);
    if(p)   fprintf(stdout, "%s", p->c_str());
    else    fprintf(stdout, "t%d", t->Int());
  });
}

} // namespace live
