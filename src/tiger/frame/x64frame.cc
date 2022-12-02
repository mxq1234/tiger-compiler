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
      : Frame(label, formals, new std::list<bool>(escapes)), spOffset(-reg_manager->WordSize()) { }
    Access* AllocLocal(bool escape) override;
    int GetFrameSize() const override { return -spOffset; }
};
/* TODO: Put your lab5 code here */

Frame* Frame::NewFrame(temp::Label* label, const std::list<bool>& escapes) {
  Frame* frame = new X64Frame(label, new std::list<frame::Access*>, escapes);
  int i = 0, offset = reg_manager->WordSize();
  temp::TempList* argRegs = reg_manager->ArgRegs();
  for(bool escape : escapes) {
    frame::Access* access;
    if(escape || i >= (int)argRegs->GetList().size()) {
      access = new InFrameAccess(offset);
      offset += reg_manager->WordSize();
    } else {
      access = new InRegAccess(argRegs->NthTemp(i));
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

assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
  body->Append(new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  return body; 
}

assem::Proc* ProcEntryExit3(Frame* frame, assem::InstrList* body) {
  std::string prologue = frame->GetLabel() + ":\n", epilogue = "";
  prologue.append("movq %rbp, -8(%rsp)\n");
  prologue.append("movq %rsp, %rbp\n");
  prologue.append("subq $128, %rsp\n");
  epilogue.append("addq $128, %rsp\n");
  epilogue.append("movq -8(%rsp), %rbp\n");
  epilogue.append("retq\n");
  return new assem::Proc(prologue, body, epilogue);
}

} // namespace frame
