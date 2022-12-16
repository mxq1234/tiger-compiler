#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* toExp(tree::Exp* framePtr) const override {
    return new tree::MemExp(
      new tree::BinopExp(
        tree::BinOp::PLUS_OP, framePtr, new tree::ConstExp(offset)
      )
    );
  }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* toExp(tree::Exp* framePtr) const override {
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
  int spOffset;

  public:
    X64Frame(temp::Label* label, std::list<frame::Access*>* formals, const std::list<bool>& escapes)
      : Frame(label, formals, new std::list<bool>(escapes)), spOffset(-reg_manager->WordSize()) {
      procEntryExit1Stm = new tree::ExpStm(new tree::ConstExp(0));
    }
    Access* AllocLocal(bool escape) override;
    int GetFrameSize() const override { return -spOffset; }
};
/* TODO: Put your lab5 code here */

Frame* Frame::NewFrame(temp::Label* label, const std::list<bool>& escapes) {
  Frame* frame = new X64Frame(label, new std::list<frame::Access*>, escapes);
  int i = 0, upOffset = reg_manager->WordSize();
  temp::TempList* argRegs = reg_manager->ArgRegs();
  for(bool escape : escapes) {
    frame::Access* access;
    if(i >= (int)argRegs->GetList().size()) {
      access = new InFrameAccess(upOffset);
      upOffset += reg_manager->WordSize();
    } else {
      if(escape)  access = frame->AllocLocal(true);
      else  access = new InRegAccess(temp::TempFactory::NewTemp());    
      frame->procEntryExit1Stm = new tree::SeqStm(frame->procEntryExit1Stm,
        new tree::MoveStm(
          access->toExp(new tree::TempExp(reg_manager->FramePointer())),
          new tree::TempExp(argRegs->NthTemp(i))
        )
      );
    }
    frame->formals_->push_back(access);
    ++i;
  }
  return frame;
}

/* See again when RA, there may be not enough regs in actual */
Access* X64Frame::AllocLocal(bool escape) {
  frame::Access* access;
  if(escape) {
    spOffset -= reg_manager->WordSize();
    access = new InFrameAccess(spOffset);
  } else {
    access = new InRegAccess(temp::TempFactory::NewTemp());
  }
  return access;
}

tree::Exp* externalCall(std::string s, tree::ExpList* args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
}

tree::Stm* ProcEntryExit1(Frame* frame, tree::Stm* stm) {
  return new tree::SeqStm(frame->procEntryExit1Stm, stm);
}

assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
  body->Append(new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  return body; 
}

assem::Proc* ProcEntryExit3(Frame* frame, assem::InstrList* body) {
  std::string prologue = frame->GetLabel() + ":\n", epilogue = "";
  prologue.append("movq %rbp, -8(%rsp)\n");
  prologue.append("movq %rsp, %rbp\n");
  prologue.append("subq $" + std::to_string(frame->GetFrameSize()) + ", %rsp\n");
  epilogue.append("addq $" + std::to_string(frame->GetFrameSize()) + ", %rsp\n");
  epilogue.append("movq -8(%rsp), %rbp\n");
  epilogue.append("retq\n");
  return new assem::Proc(prologue, body, epilogue);
}

} // namespace frame
