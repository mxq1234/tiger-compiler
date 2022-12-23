#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>
#include <map>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"
#include "tiger/runtime/gc/roots/roots.h"
#include "tiger/semant/types.h"

namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()), temp_ty_map_(temp::TempTyMap::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual int RegNum() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  [[nodiscard]] virtual bool IsArgsRegister(temp::Temp*) = 0;

  [[nodiscard]] virtual bool IsCalleeRegister(temp::Temp*) = 0;

  [[nodiscard]] virtual temp::Temp* GetRegisterByName(std::string s) = 0;

  [[nodiscard]] virtual int GetCalleeRegisterNo(temp::Temp* t) = 0;

  temp::Map *temp_map_;

  temp::TempTyMap* temp_ty_map_;

  std::map<temp::Label*, gc::PointerMap*> pointer_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  type::Ty* result_ty_;
  /* TODO: Put your lab5 code here */
  virtual tree::Exp* toExp(tree::Exp* framePtr) const = 0;
  virtual ~Access() = default;
};

class Frame {
  /* TODO: Put your lab5 code here */
  public:
    std::list<frame::Access*>* formals_;
    std::list<bool>* escapes_;
    std::vector<int>* roots_;
    std::map<temp::Temp*, int>* spills_;
    std::map<temp::Temp*, int>* saves_;
    temp::Label* name_;
    tree::Stm* procEntryExit1Stm;

  protected:
    Frame(temp::Label* label, std::list<frame::Access*>* formals, std::list<bool>* escapes)
        : name_(label), formals_(formals), escapes_(escapes), roots_(new std::vector<int>()),
        spills_(new std::map<temp::Temp*, int>), saves_(new std::map<temp::Temp*, int>) {}
  public:
    static Frame* NewFrame(temp::Label* label, const std::list<bool>& escapes, const std::vector<type::Ty*>& types);
    std::string GetLabel() const { return name_->Name().data(); }
    std::list<bool>* GetEscapesList() const { return escapes_; }
    virtual int GetFrameSize() const = 0;
    virtual Access* AllocLocal(bool escape, type::Ty* result_ty) = 0;
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */

tree::Exp* externalCall(std::string s, tree::ExpList* args);

/* 4, 5, 8 */
tree::Stm* ProcEntryExit1(Frame* frame, tree::Stm* stm);

assem::InstrList* ProcEntryExit2(assem::InstrList* body);

assem::Proc* ProcEntryExit3(Frame* frame, assem::InstrList* body);

} // namespace frame

#endif