#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  for(assem::Instr* instr : instr_list_->GetList()) {
    if(typeid(*instr) != typeid(assem::LabelInstr))   continue;
    FNodePtr node = flowgraph_->NewNode(instr);
    temp::Label* label = static_cast<assem::LabelInstr*>(instr)->label_;
    label_map_->Enter(label, node);
  }
  FNodePtr lastNode = nullptr, thisNode = nullptr;
  for(assem::Instr* instr : instr_list_->GetList()) {
    if(typeid(*instr) != typeid(assem::LabelInstr)) {
      thisNode = flowgraph_->NewNode(instr);
      if(lastNode != nullptr)   flowgraph_->AddEdge(lastNode, thisNode);
      if(typeid(*instr) == typeid(assem::OperInstr)) {
        assem::Targets* targets = static_cast<assem::OperInstr*>(instr)->jumps_;
        if(targets != nullptr) {
          for(temp::Label* label : *(targets->labels_))
            flowgraph_->AddEdge(thisNode, label_map_->Look(label));
        }
        if(static_cast<assem::OperInstr*>(instr)->assem_.find("jmp") != std::string::npos)
          lastNode = nullptr;
        else
          lastNode = thisNode;
      } else {
        lastNode = thisNode;
      }
    } else {
      thisNode = label_map_->Look(static_cast<assem::LabelInstr*>(instr)->label_);
      if(lastNode != nullptr)   flowgraph_->AddEdge(lastNode, thisNode);
      lastNode = thisNode;
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return ((dst_ != nullptr)? dst_ : new temp::TempList());
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return ((dst_ != nullptr)? dst_ : new temp::TempList());
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return ((src_ != nullptr)? src_ : new temp::TempList());
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return ((src_ != nullptr)? src_ : new temp::TempList());
}
} // namespace assem
