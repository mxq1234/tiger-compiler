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
    X64Frame(temp::Label* label, std::list<frame::Access*>* formals)
      : Frame(label, formals), spOffset(0) { }
    Access* AllocLocal(bool escape) override;
};
/* TODO: Put your lab5 code here */

Frame* Frame::NewFrame(temp::Label* label, const std::list<bool>& escapes) {
  Frame* frame = new X64Frame(label, new std::list<frame::Access*>);
  for(bool escape : escapes)
    frame->formals_->push_back(frame->AllocLocal(escape));
  return frame;
}

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

} // namespace frame