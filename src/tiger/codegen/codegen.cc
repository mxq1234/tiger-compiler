#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  assem_instr_ = std::make_unique<AssemInstr>(new assem::InstrList);
  temp::TempList* saveCalleeList = new temp::TempList;

  for(temp::Temp* argTemp : reg_manager->CalleeSaves()->GetList()) {
    temp::Temp* saveCalleeTmp = temp::TempFactory::NewTemp();
    saveCalleeList->Append(saveCalleeTmp);
    (*(assem_instr_->GetInstrList())).Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ saveCalleeTmp }), new temp::TempList({ argTemp })));
  }
  for(tree::Stm* stm : traces_->GetStmList()->GetList())
    stm->Munch(*(assem_instr_->GetInstrList()), frame_->GetLabel() + "_framesize");

  auto saveCalleeTmpItr = saveCalleeList->GetList().begin();
  for(temp::Temp* argTemp : reg_manager->CalleeSaves()->GetList())
    (*(assem_instr_->GetInstrList())).Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ argTemp }), new temp::TempList({ *(saveCalleeTmpItr++) })));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
   left_->Munch(instr_list, fs);
   right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::OperInstr("jmp " + exp_->name_->Name(), nullptr, nullptr, new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* leftTemp = left_->Munch(instr_list, fs);
  temp::Temp* rightTemp = right_->Munch(instr_list, fs);
  std::string instr = "cmpq `s0, `s1";
  instr_list.Append(new assem::OperInstr(instr, nullptr, new temp::TempList({ rightTemp, leftTemp }), nullptr));

  std::vector<temp::Label*>* targets = new std::vector<temp::Label*>;
  targets->push_back(true_label_);
  switch (op_)
  {
  case RelOp::EQ_OP: instr = "je " + true_label_->Name(); break;
  case RelOp::NE_OP: instr = "jne " + true_label_->Name(); break;
  case RelOp::GT_OP: instr = "jg " + true_label_->Name(); break;
  case RelOp::GE_OP: instr = "jge " + true_label_->Name(); break;
  case RelOp::LE_OP: instr = "jle " + true_label_->Name(); break;
  case RelOp::LT_OP: instr = "jl " + true_label_->Name(); break;
  default: break;
  }
  instr_list.Append(new assem::OperInstr(
    instr, nullptr, nullptr, new assem::Targets(targets)
  ));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::string instr = "movq ";
  temp::TempList* srcTempList = new temp::TempList;
  temp::TempList* dstTempList = new temp::TempList;
  int srcNum = 0, dstNum = 0, flag = 0;
  if(typeid(*src_) == typeid(ConstExp)) {
    int immediate = static_cast<ConstExp*>(src_)->consti_;
    instr.append("$" + std::to_string(immediate) + ", ");
  }
  else if(typeid(*src_) == typeid(MemExp)) {
    if(typeid(*(static_cast<MemExp*>(src_)->exp_)) == typeid(BinopExp)) {
      BinopExp* binopExp = static_cast<BinopExp*>(static_cast<MemExp*>(src_)->exp_);
      temp::Temp* baseAddrTmp = binopExp->left_->Munch(instr_list, fs);
      Exp* offsetExp = binopExp->right_;
      if(typeid(*offsetExp) == typeid(ConstExp)) {
        int offset = static_cast<ConstExp*>(offsetExp)->consti_;
        instr.append(std::to_string(offset) + "(`s" + std::to_string(srcNum++) + "), ");
        srcTempList->Append(baseAddrTmp);
      }
      else if(typeid(*offsetExp) == typeid(BinopExp)) {
        BinopExp* binopExp2 = static_cast<BinopExp*>(offsetExp);
        temp::Temp* offsetTmp = binopExp2->left_->Munch(instr_list, fs);
        int wordSize = static_cast<ConstExp*>(binopExp2->right_)->consti_;
        instr.append("(`s" + std::to_string(srcNum++) + ",`s");
        srcTempList->Append(baseAddrTmp);
        instr.append(std::to_string(srcNum++) + "," + std::to_string(wordSize) + "), ");
        srcTempList->Append(offsetTmp);
      } else {
        throw "Mem Exp Error";
      }
    } else {
      temp::Temp* addrTmp = static_cast<MemExp*>(src_)->exp_->Munch(instr_list, fs);
      instr.append("(`s" + std::to_string(srcNum++) + "), ");
      srcTempList->Append(addrTmp);
    }
  }
  else {
    temp::Temp* srcTmp = src_->Munch(instr_list, fs);
    instr.append("`s" + std::to_string(srcNum++) + ", ");
    srcTempList->Append(srcTmp);
  }

  if(typeid(*dst_) == typeid(MemExp)) {
    if(typeid(*src_) == typeid(MemExp)) {
      temp::Temp* newTmp = temp::TempFactory::NewTemp();
      instr.append("`d" + std::to_string(dstNum++));
      dstTempList->Append(newTmp);
      instr_list.Append(new assem::MoveInstr(instr, dstTempList, srcTempList));
      dstNum = srcNum = 0;
      dstTempList = new temp::TempList;
      srcTempList = new temp::TempList;
      instr = "movq `s" + std::to_string(srcNum++) + ", ";
      srcTempList->Append(newTmp);
    }
    if(typeid(*(static_cast<MemExp*>(dst_)->exp_)) == typeid(BinopExp)) {
      BinopExp* binopExp = static_cast<BinopExp*>(static_cast<MemExp*>(dst_)->exp_);
      temp::Temp* baseAddrTmp = binopExp->left_->Munch(instr_list, fs);
      Exp* offsetExp = binopExp->right_;
      if(typeid(*offsetExp) == typeid(ConstExp)) {
        int offset = static_cast<ConstExp*>(offsetExp)->consti_;
        instr.append(std::to_string(offset) + "(`s" + std::to_string(srcNum++) + ")");
        srcTempList->Append(baseAddrTmp);
      }
      else if(typeid(*offsetExp) == typeid(BinopExp)) {
        BinopExp* binopExp2 = static_cast<BinopExp*>(offsetExp);
        temp::Temp* offsetTmp = binopExp2->left_->Munch(instr_list, fs);
        int wordSize = static_cast<ConstExp*>(binopExp2->right_)->consti_;
        instr.append("(`s" + std::to_string(srcNum++) + ",`s");
        srcTempList->Append(baseAddrTmp);
        instr.append(std::to_string(srcNum++) + "," + std::to_string(wordSize) + ")");
        srcTempList->Append(offsetTmp);
      } else {
        throw "Mem Exp Error";
      }
    } else {
      temp::Temp* addrTmp = static_cast<MemExp*>(dst_)->exp_->Munch(instr_list, fs);
      instr.append("(`s" + std::to_string(srcNum++) + ")");
      srcTempList->Append(addrTmp);
    }
  }
  else {
    temp::Temp* dstTmp = dst_->Munch(instr_list, fs);
    instr.append("`d" + std::to_string(dstNum++));
    dstTempList->Append(dstTmp);
  }
  if(!dstNum)   dstTempList = nullptr;
  instr_list.Append(new assem::MoveInstr(instr, dstTempList, srcTempList));
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* leftTmp = left_->Munch(instr_list, fs);
  temp::Temp* rightTmp;
  temp::Temp* dstTmp = temp::TempFactory::NewTemp();
  temp::Temp* saveRaxTmp = temp::TempFactory::NewTemp();
  temp::Temp* saveRdxTmp = temp::TempFactory::NewTemp();
  temp::Temp* raxTmp = reg_manager->ReturnValue();
  temp::Temp* rdxTmp = reg_manager->ArgRegs()->NthTemp(2);
  switch (op_)
  {
  case BinOp::PLUS_OP:
    if(typeid(*right_) == typeid(ConstExp)) {
      int immediate = static_cast<ConstExp*>(right_)->consti_;
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ leftTmp })));
      instr_list.Append(new assem::OperInstr("addq $" + std::to_string(immediate) + ", `d0", new temp::TempList({ dstTmp }), nullptr, nullptr));
    } else {
      rightTmp = right_->Munch(instr_list, fs);
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ leftTmp })));
      instr_list.Append(new assem::OperInstr("addq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ rightTmp }), nullptr));
    }
    break;
  case BinOp::MINUS_OP:
    if(typeid(*right_) == typeid(ConstExp)) {
      int immediate = static_cast<ConstExp*>(right_)->consti_;
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ leftTmp })));
      instr_list.Append(new assem::OperInstr("subq $" + std::to_string(immediate) + ", `d0", new temp::TempList({ dstTmp }), nullptr, nullptr));
    } else {
      rightTmp = right_->Munch(instr_list, fs);
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ leftTmp })));
      instr_list.Append(new assem::OperInstr("subq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ rightTmp }), nullptr));
    }
    break;
  case BinOp::MUL_OP:
    rightTmp = right_->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ saveRaxTmp }), new temp::TempList({ raxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ saveRdxTmp }), new temp::TempList({ rdxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ raxTmp }), new temp::TempList({ leftTmp })));
    instr_list.Append(new assem::OperInstr("cqto", nullptr, nullptr, nullptr));
    instr_list.Append(new assem::OperInstr("imulq `s0", nullptr, new temp::TempList({ rightTmp }), nullptr));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ raxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ rdxTmp }), new temp::TempList({ saveRdxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ raxTmp }), new temp::TempList({ saveRaxTmp })));
    break;
  case BinOp::DIV_OP:
    rightTmp = right_->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ saveRaxTmp }), new temp::TempList({ raxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ saveRdxTmp }), new temp::TempList({ rdxTmp })));    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ raxTmp }), new temp::TempList({ leftTmp })));
    instr_list.Append(new assem::OperInstr("cqto", nullptr, nullptr, nullptr));
    instr_list.Append(new assem::OperInstr("idivq `s0", nullptr, new temp::TempList({ rightTmp }), nullptr));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ dstTmp }), new temp::TempList({ raxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ rdxTmp }), new temp::TempList({ saveRdxTmp })));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ raxTmp }), new temp::TempList({ saveRaxTmp })));    break;
  default: break;
  }
  return dstTmp;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* dstTmp = temp::TempFactory::NewTemp();
  if(typeid(*exp_) == typeid(BinopExp)) {
    BinopExp* binopExp = static_cast<BinopExp*>(exp_);
    temp::Temp* baseAddrTmp = binopExp->left_->Munch(instr_list, fs);
    Exp* offsetExp = binopExp->right_;
    if(typeid(*offsetExp) == typeid(ConstExp)) {
      int offset = static_cast<ConstExp*>(offsetExp)->consti_;
      instr_list.Append(new assem::MoveInstr("movq " + std::to_string(offset) + "(`s0), `d0", new temp::TempList({ dstTmp }), new temp::TempList({ baseAddrTmp })));
      return dstTmp;
    }
    if(typeid(*offsetExp) == typeid(BinopExp)) {
      BinopExp* binopExp2 = static_cast<BinopExp*>(offsetExp);
      temp::Temp* offsetTmp = binopExp2->left_->Munch(instr_list, fs);
      int wordSize = static_cast<ConstExp*>(binopExp2->right_)->consti_;
      instr_list.Append(new assem::MoveInstr("movq (`s0,`s1," + std::to_string(wordSize) + "),`d0", new temp::TempList({ dstTmp }), new temp::TempList({ baseAddrTmp, offsetTmp })));
      return dstTmp;
    }
    throw "MemExp Error";
  }
  temp::Temp* addrTmp = exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr("movq (`s0), `d0", new temp::TempList({ dstTmp }), new temp::TempList({ addrTmp })));
  return dstTmp;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // if(temp_ != reg_manager->FramePointer())   return temp_;
  // temp::Temp* tmp = temp::TempFactory::NewTemp();
  // instr_list.Append(new assem::OperInstr("leaq " + std::string(fs) + "(`s0), `d0", new temp::TempList({ tmp }), new temp::TempList({ reg_manager->StackPointer() }), nullptr));
  // return tmp;
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* dstTmp = temp::TempFactory::NewTemp();
  std::string instr = "leaq " + name_->Name() + "(%rip), `d0";
  instr_list.Append(new assem::MoveInstr(instr, new temp::TempList({ dstTmp }), nullptr));
  return dstTmp;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* dstTmp = temp::TempFactory::NewTemp();
  std::string instr = "movq $" + std::to_string(consti_) + ", `d0";
  instr_list.Append(new assem::MoveInstr(instr, new temp::TempList({ dstTmp }), nullptr));
  return dstTmp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*fun_) != typeid(NameExp))    throw "CallExp Error";
  temp::Label* label = static_cast<NameExp*>(fun_)->name_;
  int size = 0;
  temp::TempList* saveCallerList = new temp::TempList;

  for(temp::Temp* argTemp : reg_manager->CallerSaves()->GetList()) {
    temp::Temp* saveCallerTmp = temp::TempFactory::NewTemp();
    saveCallerList->Append(saveCallerTmp);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ saveCallerTmp }), new temp::TempList({ argTemp })));
  }

  temp::TempList* srcTmpList = args_->MunchArgs(instr_list, fs, escapes_, size);
  temp::TempList* dstTmpList = reg_manager->CallerSaves();

  if(size > 0)  instr_list.Append(new assem::OperInstr("subq $" + std::to_string(size) + ", `d0", new temp::TempList({ reg_manager->StackPointer() }), nullptr, nullptr));
  instr_list.Append(new assem::OperInstr("callq " + label->Name(), dstTmpList, srcTmpList, nullptr));
  if(size > 0)  instr_list.Append(new assem::OperInstr("addq $" + std::to_string(size) + ", `d0", new temp::TempList({ reg_manager->StackPointer() }), nullptr, nullptr));
  
  temp::Temp* resultTmp = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ resultTmp }), new temp::TempList({ reg_manager->ReturnValue() })));

  auto saveCallerTmpItr = saveCallerList->GetList().begin();
  for(temp::Temp* argTemp : reg_manager->CallerSaves()->GetList())
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ argTemp }), new temp::TempList({ *(saveCallerTmpItr++) })));
  return resultTmp;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs, std::list<bool>* escapes, int& spOff) {
  /* TODO: Put your lab5 code here */
  temp::TempList* tmpList = new temp::TempList;
  temp::TempList* argList = reg_manager->ArgRegs();
  int i = (int)exp_list_.size() - 1, argRegNum = argList->GetList().size();
  if(escapes) {
    auto escapeRItr = escapes->rbegin();
    for(auto ritr = exp_list_.rbegin(); ritr != exp_list_.rend(); ++ritr, ++escapeRItr) {
      temp::Temp* resultTmp = (*ritr)->Munch(instr_list, fs);
      tmpList->Append(resultTmp);
      if(i >= argRegNum || *escapeRItr) {
        spOff += reg_manager->WordSize();
        instr_list.Append(new assem::MoveInstr("movq `s0, -" + std::to_string(spOff) + "(`s1)", nullptr, new temp::TempList({ resultTmp, reg_manager->StackPointer() })));
      } else {
        instr_list.Append(new assem::MoveInstr("movq `s0 ,`d0", new temp::TempList({ argList->NthTemp(i) }), new temp::TempList({ resultTmp })));
      }
      --i;
    }
  } else {
    for(auto ritr = exp_list_.rbegin(); ritr != exp_list_.rend(); ++ritr) {
      temp::Temp* resultTmp = (*ritr)->Munch(instr_list, fs);
      tmpList->Append(resultTmp);
      if(i >= argRegNum) {
        spOff += reg_manager->WordSize();
        instr_list.Append(new assem::MoveInstr("movq `s0, -" + std::to_string(spOff) + "(`s1)", nullptr, new temp::TempList({ resultTmp, reg_manager->StackPointer() })));
      } else {
        instr_list.Append(new assem::MoveInstr("movq `s0 ,`d0", new temp::TempList({ argList->NthTemp(i) }), new temp::TempList({ resultTmp })));
      }
      --i;
    }
  }
  return tmpList;
}

} // namespace tree