#include "tiger/frame/x64frame.h"
#include "tiger/semant/types.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  InFrameAccess(int offset, type::Ty* result_ty) : offset(offset) { result_ty_ = result_ty; }
  /* TODO: Put your lab5 code here */
  tree::Exp* toExp(tree::Exp* framePtr) const override {
    return new tree::MemExp(
      new tree::BinopExp(
        tree::BinOp::PLUS_OP, framePtr, new tree::ConstExp(offset), type::IntTy::Instance()
      ),
      result_ty_
    );
  }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg, type::Ty* result_ty) : reg(reg) { result_ty_ = result_ty; }
  /* TODO: Put your lab5 code here */
  tree::Exp* toExp(tree::Exp* framePtr) const override {
    return new tree::TempExp(reg, result_ty_);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
  int spOffset;

  public:
    X64Frame(temp::Label* label, std::list<frame::Access*>* formals, const std::list<bool>& escapes)
      : Frame(label, formals, new std::list<bool>(escapes)), spOffset(0) {
      procEntryExit1Stm = new tree::ExpStm(new tree::ConstExp(0));
    }
    Access* AllocLocal(bool escape, type::Ty* result_ty) override;
    int GetFrameSize() const override { return -spOffset; }
};
/* TODO: Put your lab5 code here */

Frame* Frame::NewFrame(temp::Label* label, const std::list<bool>& escapes, const std::vector<type::Ty*>& types) {
  Frame* frame = new X64Frame(label, new std::list<frame::Access*>, escapes);
  int i = (int)escapes.size() - 1;
  temp::TempList* argRegs = reg_manager->ArgRegs();
  for(auto itr = escapes.rbegin(); itr != escapes.rend(); ++itr) {
    frame::Access* access;
    if(i >= (int)argRegs->GetList().size()) {
      access = frame->AllocLocal(true, types[i]);
    } else {
      access = frame->AllocLocal(*itr, types[i]);
      frame->procEntryExit1Stm = new tree::SeqStm(frame->procEntryExit1Stm,
        new tree::MoveStm(
          access->toExp(new tree::TempExp(reg_manager->FramePointer())),
          new tree::TempExp(argRegs->NthTemp(i), types[i])
        )
      );
    }
    frame->formals_->push_front(access);
    --i;
  }
  return frame;
}

/* See again when RA, there may be not enough regs in actual */
Access* X64Frame::AllocLocal(bool escape, type::Ty* result_ty) {
  frame::Access* access;
  if(escape) {
    spOffset -= reg_manager->WordSize();
    if(typeid(*(result_ty->ActualTy())) == typeid(type::RecordTy)
    || typeid(*(result_ty->ActualTy())) == typeid(type::ArrayTy)
    || typeid(*(result_ty->ActualTy())) == typeid(type::NilTy))
      roots_->push_back(spOffset);
    if(typeid(*(result_ty->ActualTy())) == typeid(type::SpillTy))
      (*spills_)[(static_cast<type::SpillTy*>(result_ty))->temp_] = spOffset;
    if(typeid(*(result_ty->ActualTy())) == typeid(type::SaveTy))
      (*saves_)[(static_cast<type::SaveTy*>(result_ty))->temp_] = spOffset;
    access = new InFrameAccess(spOffset, result_ty);
  } else {
    temp::Temp* tmp = temp::TempFactory::NewTemp();
    access = new InRegAccess(tmp, result_ty);
  }
  return access;
}

tree::Exp* externalCall(std::string s, tree::ExpList* args) {
  // if(s == "alloc_record" || s == "init_array")
  //   return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s), type::IntTy::Instance()), args, type::ArrayTy::Instance());
  // else
  //   return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s), type::IntTy::Instance()), args, type::IntTy::Instance());
  return nullptr;
}

tree::Stm* ProcEntryExit1(Frame* frame, tree::Stm* stm) {
  return new tree::SeqStm(frame->procEntryExit1Stm, stm);
}

assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
  body->Append(new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  return body; 
}

assem::Proc* ProcEntryExit3(Frame* frame, assem::InstrList* body) {
  std::string prologue = ".set " + frame->name_->Name() + "_framesize, " + std::to_string(frame->GetFrameSize()) + "\n", epilogue = "";
  prologue.append(frame->GetLabel() + ":\n");
  prologue.append("subq $" + std::to_string(frame->GetFrameSize()) + ", %rsp\n");
  epilogue.append("addq $" + std::to_string(frame->GetFrameSize()) + ", %rsp\n");
  epilogue.append("retq\n");
  return new assem::Proc(prologue, body, epilogue);
}

} // namespace frame