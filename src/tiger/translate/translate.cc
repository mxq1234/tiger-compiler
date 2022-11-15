#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
  static Exp* GetExpForSimpleVar(tr::Access* access, tr::Level* level);
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    tree::CjumpStm* stm = new tree::CjumpStm(tree::RelOp::EQ_OP, exp_, new tree::ConstExp(1), nullptr, nullptr);
    return Cx(PatchList({&(stm->true_label_)}), PatchList({&(stm->false_label_)}), stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    errormsg->Error(0, "Can't convert NxExp to Cx");
    return Cx(PatchList({}), PatchList({}), nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    tree::TempExp* resultExp = new tree::TempExp(temp::TempFactory::NewTemp());
    temp::Label* trueLabel = temp::LabelFactory::NewLabel();
    temp::Label* falseLabel = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(trueLabel);
    cx_.falses_.DoPatch(falseLabel);

    return new tree::EseqExp(
      new tree::MoveStm(resultExp, new tree::ConstExp(1)),
      new tree::EseqExp(
        cx_.stm_, new tree::EseqExp(
          new tree::LabelStm(falseLabel), new tree::EseqExp(
            new tree::MoveStm(resultExp, new tree::ConstExp(0)),
            new tree::EseqExp(new tree::LabelStm(trueLabel), resultExp)
          )
        )
      )
    );
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    tree::TempExp* resultExp = new tree::TempExp(temp::TempFactory::NewTemp());
    temp::Label* trueLabel = temp::LabelFactory::NewLabel();
    temp::Label* falseLabel = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(trueLabel);
    cx_.falses_.DoPatch(falseLabel);

    return new tree::SeqStm(
      cx_.stm_, new tree::SeqStm(
        new tree::LabelStm(falseLabel),
        new tree::LabelStm(trueLabel)
      )
    );
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new Access(level, level->frame_->AllocLocal(escape));
}

Level* Level::NewLevel(Level* parent, temp::Label* label, std::list<bool>* formals) {
  formals->push_front(true);
  frame::Frame* frame = frame::Frame::NewFrame(label, (*formals));
  return new Level(frame, parent);
}

Exp* Exp::GetExpForSimpleVar(tr::Access* access, tr::Level* level) {
  tree::Exp* fpExp = new tree::TempExp(reg_manager->FramePointer());
  for(tr::Level* decLevel = access->level_; decLevel != level; level = level->parent_)
    fpExp = level->frame_->formals_->front()->toExp(fpExp);
  return new tr::ExExp(access->access_->toExp(fpExp));
}

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  temp::Label* tigerMainLabel = temp::LabelFactory::NamedLabel("tigermain");
  Level* mainLevel = Level::NewLevel(nullptr, temp::LabelFactory::NamedLabel("main"), new std::list<bool>({}));
  Level* tigerMainLevel = Level::NewLevel(mainLevel, tigerMainLabel, new std::list<bool>({}));
  main_level_.reset(tigerMainLevel);
  FillBaseVEnv();
  FillBaseTEnv();
  tr::ExpAndTy* absynExpAndTy = absyn_tree_->Translate(venv_.get(), tenv_.get(), tigerMainLevel, tigerMainLabel, errormsg_.get());
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry* entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)) {
    env::VarEntry* varEntry = static_cast<env::VarEntry*>(entry);
    tr::Exp* exp = tr::Exp::GetExpForSimpleVar(varEntry->access_, level);
    type::Ty* ty = varEntry->ty_->ActualTy();
    return new tr::ExpAndTy(exp, ty);
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* expAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  if(expAndTy->exp_ && typeid(*(expAndTy->ty_->ActualTy())) == typeid(type::RecordTy)) {
    type::FieldList* fieldList = (static_cast<type::RecordTy*>(expAndTy->ty_->ActualTy()))->fields_;
    int fieldOffset = 0, wordsize = reg_manager->WordSize();
    for(type::Field* field : fieldList->GetList()) {
      if(field->name_ == sym_)  return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::MemExp(
            new tree::BinopExp(
              tree::BinOp::PLUS_OP, 
              expAndTy->exp_->UnEx(),
              new tree::ConstExp(wordsize * fieldOffset)
            )
          )
        ), field->ty_
      );
      ++fieldOffset;
    }
    errormsg->Error(pos_, "field nam doesn't exist");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  } else {
    errormsg->Error(pos_, "not a record type");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* varExpAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  if(varExpAndTy->exp_ && typeid(*(varExpAndTy->ty_->ActualTy())) == typeid(type::ArrayTy)) {
    tr::ExpAndTy* subscriptExpAndTy = subscript_->Translate(venv, tenv, level, label, errormsg);
    if(subscriptExpAndTy->exp_ && typeid(*(subscriptExpAndTy->ty_)) == typeid(type::IntTy)) {
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::MemExp(
            new tree::BinopExp(
              tree::BinOp::PLUS_OP,
              varExpAndTy->exp_->UnEx(),
              subscriptExpAndTy->exp_->UnEx()
            )
          )
        ), static_cast<type::ArrayTy*>(varExpAndTy->ty_->ActualTy())->ty_
      );
    } else {
      errormsg->Error(subscript_->pos_, "integer required");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
  } else {
    if(varExpAndTy->exp_)
      errormsg->Error(var_->pos_, "array type required");
    else
      errormsg->Error(var_->pos_, "!!!");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)), type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label* stringLabel = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(stringLabel, str_));
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::NameExp(stringLabel)
      )
    ), type::StringTy::Instance()
  );
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry* entry = venv->Look(func_);
  if(entry && typeid(*entry) == typeid(env::FunEntry)) {
    env::FunEntry* funEntry = static_cast<env::FunEntry*>(entry);
    type::Ty* resultTy = funEntry->result_;
    const std::list<type::Ty*>& formalsList = (static_cast<env::FunEntry*>(entry))->formals_->GetList();
    const std::list<absyn::Exp*>& actualsList = args_->GetList();
    auto formalItr = formalsList.begin();
    auto actualItr = actualsList.begin();
    tree::ExpList* expList = new tree::ExpList;
    while(formalItr != formalsList.end() && actualItr != actualsList.end()) {
      tr::ExpAndTy* actualExpAndTy = (*actualItr)->Translate(venv, tenv, level, label, errormsg);
      if(!actualExpAndTy->ty_->IsSameType(*formalItr))
        errormsg->Error((*actualItr)->pos_, "para type mismatch");
      if(!actualExpAndTy->exp_) {
        errormsg->Error((*actualItr)->pos_, "call exp error");
        return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
      }
      expList->Append(actualExpAndTy->exp_->UnEx());
      ++formalItr;
      ++actualItr;
    }
    if(formalItr != formalsList.end()) {
      errormsg->Error(pos_, "call with too little params");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    if(actualItr != actualsList.end()) {
      errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    tree::Exp* fpExp = new tree::TempExp(reg_manager->FramePointer());
    for(tr::Level* decLevel = funEntry->level_->parent_; decLevel != level; level = level->parent_)
      fpExp = level->frame_->formals_->front()->toExp(fpExp);
    expList->Insert(fpExp);
    return new tr::ExpAndTy(new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), expList)), resultTy);
  } else {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* leftExpAndTy = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* rightExpAndTy = right_->Translate(venv, tenv, level, label, errormsg);

  switch (oper_)
  {
  case Oper::PLUS_OP: case Oper::MINUS_OP: case Oper::TIMES_OP: case Oper::DIVIDE_OP:
  case Oper::AND_OP: case Oper::OR_OP:
    if(typeid(*(leftExpAndTy->ty_)) != typeid(type::IntTy)) {
      errormsg->Error(left_->pos_, "integer required");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    if(typeid(*(rightExpAndTy->ty_)) != typeid(type::IntTy)) {
      errormsg->Error(right_->pos_, "integer required");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    break;
  case Oper::EQ_OP: case Oper::NEQ_OP: case Oper::GE_OP: case Oper::GT_OP:
  case Oper::LE_OP: case Oper::LT_OP:
    if(!leftExpAndTy->ty_->IsSameType(rightExpAndTy->ty_)) {
      errormsg->Error(pos_, "same type required");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    break;
  }
  if(!leftExpAndTy->exp_ || !rightExpAndTy->exp_) {
    errormsg->Error(pos_, "op exp error");
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }

  tree::CjumpStm* stm;
  tree::SeqStm* seqStm;
  temp::Label* trueLabel, *falseLabel;
  switch(oper_) 
  {
  case PLUS_OP: return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp(tree::BinOp::PLUS_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx())), type::IntTy::Instance());
  case MINUS_OP: return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp(tree::BinOp::MINUS_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx())), type::IntTy::Instance());
  case TIMES_OP: return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp(tree::BinOp::MUL_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx())), type::IntTy::Instance());
  case DIVIDE_OP: return new tr::ExpAndTy(new tr::ExExp(new tree::BinopExp(tree::BinOp::DIV_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx())), type::IntTy::Instance());
  case EQ_OP:
    if(typeid(*(leftExpAndTy->ty_)) != typeid(type::StringTy))
      stm = new tree::CjumpStm(
        tree::RelOp::EQ_OP, new tree::CallExp(
          new tree::NameExp(
            temp::LabelFactory::NamedLabel("string_equal")
          ), new tree::ExpList({ leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx() })
        ),
        new tree::ConstExp(1), nullptr, nullptr
      );
    else
      stm = new tree::CjumpStm(tree::RelOp::EQ_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx(), nullptr, nullptr);
    return new tr::ExpAndTy(new tr::CxExp(tr::PatchList({&(stm->true_label_)}), tr::PatchList({&(stm->false_label_)}), stm), type::IntTy::Instance());
  case NEQ_OP:
    if(typeid(*(leftExpAndTy->ty_)) != typeid(type::StringTy))
      stm = new tree::CjumpStm(
        tree::RelOp::EQ_OP, new tree::CallExp(
          new tree::NameExp(
            temp::LabelFactory::NamedLabel("string_equal")
          ), new tree::ExpList({ leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx() })
        ),
        new tree::ConstExp(0), nullptr, nullptr
      );
    else
      stm = new tree::CjumpStm(tree::RelOp::NE_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx(), nullptr, nullptr);
    return new tr::ExpAndTy(new tr::CxExp(tr::PatchList({&(stm->true_label_)}), tr::PatchList({&(stm->false_label_)}), stm), type::IntTy::Instance());
  case LE_OP:
    stm = new tree::CjumpStm(tree::RelOp::LE_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx(), nullptr, nullptr);
    return new tr::ExpAndTy(new tr::CxExp(tr::PatchList({&(stm->true_label_)}), tr::PatchList({&(stm->false_label_)}), stm), type::IntTy::Instance());
  case GE_OP:
    stm = new tree::CjumpStm(tree::RelOp::GE_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx(), nullptr, nullptr);
    return new tr::ExpAndTy(new tr::CxExp(tr::PatchList({&(stm->true_label_)}), tr::PatchList({&(stm->false_label_)}), stm), type::IntTy::Instance());
  case GT_OP:
    stm = new tree::CjumpStm(tree::RelOp::GT_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx(), nullptr, nullptr);
    return new tr::ExpAndTy(new tr::CxExp(tr::PatchList({&(stm->true_label_)}), tr::PatchList({&(stm->false_label_)}), stm), type::IntTy::Instance());
  case LT_OP:
    stm = new tree::CjumpStm(tree::RelOp::LT_OP, leftExpAndTy->exp_->UnEx(), rightExpAndTy->exp_->UnEx(), nullptr, nullptr);
    return new tr::ExpAndTy(new tr::CxExp(tr::PatchList({&(stm->true_label_)}), tr::PatchList({&(stm->false_label_)}), stm), type::IntTy::Instance());
  case AND_OP:
    trueLabel = temp::LabelFactory::NewLabel();
    leftExpAndTy->exp_->UnCx(errormsg).trues_.DoPatch(trueLabel);
    seqStm = new tree::SeqStm(
      leftExpAndTy->exp_->UnCx(errormsg).stm_,
      new tree::SeqStm(
        new tree::LabelStm(trueLabel),
        rightExpAndTy->exp_->UnCx(errormsg).stm_
      )
    );
    return new tr::ExpAndTy(new tr::CxExp(
      rightExpAndTy->exp_->UnCx(errormsg).trues_,
      tr::PatchList::JoinPatch(
        leftExpAndTy->exp_->UnCx(errormsg).falses_,
        rightExpAndTy->exp_->UnCx(errormsg).falses_
      ), seqStm
    ), type::IntTy::Instance());
  case OR_OP:
    falseLabel = temp::LabelFactory::NewLabel();
    leftExpAndTy->exp_->UnCx(errormsg).falses_.DoPatch(falseLabel);
    seqStm = new tree::SeqStm(
      leftExpAndTy->exp_->UnCx(errormsg).stm_,
      new tree::SeqStm(
        new tree::LabelStm(falseLabel),
        rightExpAndTy->exp_->UnCx(errormsg).stm_
      )
    );
    return new tr::ExpAndTy(new tr::CxExp(
      tr::PatchList::JoinPatch(
        leftExpAndTy->exp_->UnCx(errormsg).trues_,
        rightExpAndTy->exp_->UnCx(errormsg).trues_
      ), rightExpAndTy->exp_->UnCx(errormsg).falses_, seqStm
    ), type::IntTy::Instance());
  default:
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* typTy = tenv->Look(typ_);
  if(!typTy) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
  if(typeid(*(typTy->ActualTy())) != typeid(type::RecordTy))
    errormsg->Error(pos_, "undefined record type %s", typ_->Name().data());

  const std::list<type::Field*>& formalsList = (static_cast<type::RecordTy*>(typTy->ActualTy()))->fields_->GetList();
  const std::list<EField*>& actualsList = fields_->GetList();
  auto formalItr = formalsList.begin(), formalItre = formalsList.end();
  auto actualItr = actualsList.begin(), actualItre = actualsList.end();
  std::list<tree::Exp*> expList;
  while(formalItr != formalItre && actualItr != actualItre) {
    if((*formalItr)->name_ != (*actualItr)->name_)
      errormsg->Error(pos_, "unmatched field %s", (*actualItr)->name_);
    tr::ExpAndTy* actualExpAndTy = (*actualItr)->exp_->Translate(venv, tenv, level, label, errormsg);

    if(!(*formalItr)->ty_->IsSameType(actualExpAndTy->ty_))
      errormsg->Error(pos_, "incorrect type of %s", (*actualItr)->name_);

    if(!actualExpAndTy->exp_) {
      errormsg->Error(pos_, "record exp error");
      return new tr::ExpAndTy(nullptr, typTy);
    }
    expList.push_back(actualExpAndTy->exp_->UnEx());
    ++formalItr;
    ++actualItr;
  }

  if(formalItr != formalItre)
    errormsg->Error(pos_, "create record with too little fields");
  if(actualItr != actualItre)
    errormsg->Error(pos_, "create record with too many fields");

  int wordSize = reg_manager->WordSize(), fieldNum = expList.size(), offset = 0;
  tree::Exp* recordPtrExp = new tree::TempExp(temp::TempFactory::NewTemp());
  tree::Stm* recordStm = new tree::MoveStm(recordPtrExp, 
    new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel("alloc_record")),
      new tree::ExpList({ new tree::ConstExp(wordSize * fieldNum) })
    )
  );
  for(tree::Exp* exp : expList)
    recordStm = new tree::SeqStm(recordStm,
      new tree::MoveStm(
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP,
            recordPtrExp,
            new tree::ConstExp((offset++) * wordSize)
          )
        ), exp
      )
    );
  return new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(recordStm, recordPtrExp)), typTy);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* expAndTy;
  tree::Exp* resultExp = new tree::ConstExp(0);
  for(Exp* exp : seq_->GetList()) {
    expAndTy = exp->Translate(venv, tenv, level, label, errormsg);
    if(expAndTy->exp_) {
      resultExp = new tree::EseqExp(new tree::ExpStm(resultExp), expAndTy->exp_->UnEx());
    } else {
      errormsg->Error(pos_, "seq exp error");
      resultExp = new tree::EseqExp(new tree::ExpStm(resultExp), new tree::ConstExp(0));
    }
  }
  return new tr::ExpAndTy(new tr::ExExp(resultExp), expAndTy->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* varExpAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* expExpAndTy = exp_->Translate(venv, tenv, level, label, errormsg);

  if(!varExpAndTy->ty_->IsSameType(expExpAndTy->ty_))
    errormsg->Error(pos_, "unmatched assign exp");

  if(typeid(*var_) == typeid(SimpleVar)) {
    env::EnvEntry* entry = venv->Look((static_cast<SimpleVar*>(var_))->sym_);
    if(typeid(*entry) != typeid(env::VarEntry)) {
      errormsg->Error(var_->pos_, "undefined variable %s", (static_cast<SimpleVar*>(var_))->sym_->Name().data());
    } else {
      if(static_cast<env::VarEntry*>(entry)->readonly_)
        errormsg->Error(pos_, "loop variable can't be assigned");
    }
  }

  if(varExpAndTy->exp_ && expExpAndTy->exp_)
    return new tr::ExpAndTy(
      new tr::NxExp(
        new tree::MoveStm(
          varExpAndTy->exp_->UnEx(),
          expExpAndTy->exp_->UnEx()
        )
      )
    , type::VoidTy::Instance());

  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* testExpAndTy = test_->Translate(venv, tenv, level, label, errormsg);
  if(!testExpAndTy->ty_->IsSameType(type::IntTy::Instance()))
    errormsg->Error(test_->pos_, "integer exp required");

  tr::ExpAndTy* thenExpAndTy = then_->Translate(venv, tenv, level, label, errormsg);
  if(elsee_) {
    tr::ExpAndTy* elseExpAndTy = elsee_->Translate(venv, tenv, level, label, errormsg);
    if(!thenExpAndTy->ty_->IsSameType(elseExpAndTy->ty_)) {
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
      return new tr::ExpAndTy(nullptr, thenExpAndTy->ty_);
    }
    if(!testExpAndTy->exp_ || !thenExpAndTy->exp_ || !elseExpAndTy->exp_) {
      errormsg->Error(pos_, "if exp error");
      return new tr::ExpAndTy(nullptr, thenExpAndTy->ty_);
    }
    tr::Cx cx = testExpAndTy->exp_->UnCx(errormsg);
    temp::Label* trueTarget = temp::LabelFactory::NewLabel();
    temp::Label* falseTarget = temp::LabelFactory::NewLabel();
    temp::Label* doneTarget = temp::LabelFactory::NewLabel();
    cx.trues_.DoPatch(trueTarget);
    cx.falses_.DoPatch(falseTarget);
    tree::TempExp* tmpExp = new tree::TempExp(temp::TempFactory::NewTemp());
    if(typeid(*(thenExpAndTy->ty_)) != typeid(type::VoidTy)) {
      return new tr::ExpAndTy(new tr::ExExp(
        new tree::EseqExp(
          cx.stm_,
          new tree::EseqExp(
            new tree::LabelStm(falseTarget),
            new tree::EseqExp(
              new tree::MoveStm(tmpExp, elseExpAndTy->exp_->UnEx()),
              new tree::EseqExp(
                new tree::JumpStm(
                  new tree::NameExp(doneTarget),
                  new std::vector<temp::Label *>({doneTarget})
                ),
                new tree::EseqExp(
                  new tree::LabelStm(trueTarget),
                  new tree::EseqExp(
                    new tree::MoveStm(tmpExp, thenExpAndTy->exp_->UnEx()),
                    new tree::EseqExp(new tree::LabelStm(doneTarget), tmpExp)
                  )
                )
              )
            )
          )
        )
      ), thenExpAndTy->ty_);
    } else {
      return new tr::ExpAndTy(new tr::NxExp(
        new tree::SeqStm(
          cx.stm_,
          new tree::SeqStm(
            new tree::LabelStm(falseTarget),
            new tree::SeqStm(
              elseExpAndTy->exp_->UnNx(),
              new tree::SeqStm(
                new tree::JumpStm(
                  new tree::NameExp(doneTarget),
                  new std::vector<temp::Label *>({doneTarget})
                ),
                new tree::SeqStm(
                  new tree::LabelStm(trueTarget),
                  new tree::SeqStm(
                    thenExpAndTy->exp_->UnNx(),
                    new tree::LabelStm(doneTarget)
                  )
                )
              )
            )
          )
        )
      ), thenExpAndTy->ty_);
    }
  } else {
    if(typeid(*(thenExpAndTy->ty_)) != typeid(type::VoidTy)) {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
    }
    if(!testExpAndTy->exp_ || !thenExpAndTy->exp_) {
      errormsg->Error(pos_, "if exp error");
      return new tr::ExpAndTy(nullptr, thenExpAndTy->ty_);
    }
    tr::Cx cx = testExpAndTy->exp_->UnCx(errormsg);
    temp::Label* trueTarget = temp::LabelFactory::NewLabel();
    temp::Label* falseTarget = temp::LabelFactory::NewLabel();
    cx.trues_.DoPatch(trueTarget);
    cx.falses_.DoPatch(falseTarget);
    return new tr::ExpAndTy(new tr::NxExp(
        new tree::SeqStm(
          cx.stm_, 
          new tree::SeqStm(
            new tree::LabelStm(trueTarget),
            new tree::SeqStm(
              thenExpAndTy->exp_->UnNx(),
              new tree::LabelStm(falseTarget)
            )
          )
        )
      ), type::VoidTy::Instance()
    );
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* testExpAndTy = test_->Translate(venv, tenv, level, label, errormsg);
  if(!testExpAndTy->ty_->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(test_->pos_, "integer exp required");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  if(!testExpAndTy->exp_) {
    errormsg->Error(pos_, "while exp error");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  tr::Cx cx = testExpAndTy->exp_->UnCx(errormsg);
  temp::Label* testLabel = temp::LabelFactory::NewLabel();
  temp::Label* bodyLabel = temp::LabelFactory::NewLabel();
  temp::Label* doneLabel = temp::LabelFactory::NewLabel();
  cx.trues_.DoPatch(bodyLabel);
  cx.falses_.DoPatch(doneLabel);

  tr::ExpAndTy* bodyExpAndTy = body_->Translate(venv, tenv, level, doneLabel, errormsg);
  if(typeid(*(bodyExpAndTy->ty_)) != typeid(type::VoidTy)) {
    errormsg->Error(body_->pos_, "while body must produce no value");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  if(!bodyExpAndTy->exp_) {
    errormsg->Error(pos_, "while exp error");
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  return new tr::ExpAndTy(new tr::NxExp(
    new tree::SeqStm(
      new tree::LabelStm(testLabel),
      new tree::SeqStm(
        cx.stm_,
        new tree::SeqStm(
          new tree::LabelStm(bodyLabel),
          new tree::SeqStm(
            bodyExpAndTy->exp_->UnNx(),
            new tree::SeqStm(
              new tree::JumpStm(new tree::NameExp(testLabel), new std::vector<temp::Label *>({testLabel})),
              new tree::LabelStm(doneLabel)
            )
          )
        )
      )
    )
  ), type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();

  tr::Access* access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, type::IntTy::Instance(), true));
  tr::ExpAndTy* lowExpAndTy = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* highExpAndTy = hi_->Translate(venv, tenv, level, label, errormsg);
  if(!lowExpAndTy->ty_->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
    venv->EndScope();
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  if(!highExpAndTy->ty_->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
    venv->EndScope();
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  if(!lowExpAndTy->exp_ || !highExpAndTy->exp_) {
    errormsg->Error(pos_, "for exp error");
    venv->EndScope();
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  temp::Label* loopLabel = temp::LabelFactory::NewLabel();
  temp::Label* bodyLabel = temp::LabelFactory::NewLabel();
  temp::Label* doneLabel = temp::LabelFactory::NewLabel();
  tree::TempExp* iExp = new tree::TempExp(temp::TempFactory::NewTemp());
  tree::TempExp* limitExp = new tree::TempExp(temp::TempFactory::NewTemp());

  tr::ExpAndTy* bodyExpAndTy = body_->Translate(venv, tenv, level, doneLabel, errormsg);
  if(!bodyExpAndTy->ty_->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "void exp required");
    venv->EndScope();
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }

  if(!bodyExpAndTy->exp_) {
    errormsg->Error(pos_, "for exp error");
    venv->EndScope();
    return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  }
  venv->EndScope();
  
  return new tr::ExpAndTy(new tr::NxExp(
    new tree::SeqStm(
      new tree::MoveStm(iExp, lowExpAndTy->exp_->UnEx()),
      new tree::SeqStm(
        new tree::MoveStm(limitExp, highExpAndTy->exp_->UnEx()),
        new tree::SeqStm(
          new tree::CjumpStm(tree::RelOp::GT_OP, iExp, limitExp, doneLabel, bodyLabel),
          new tree::SeqStm(
            new tree::LabelStm(bodyLabel),
            new tree::SeqStm(
              bodyExpAndTy->exp_->UnNx(),
              new tree::SeqStm(
                new tree::CjumpStm(tree::RelOp::EQ_OP, iExp, limitExp, doneLabel, loopLabel),
                new tree::SeqStm(
                  new tree::LabelStm(loopLabel),
                  new tree::SeqStm(
                    new tree::MoveStm(iExp, new tree::BinopExp(tree::BinOp::PLUS_OP, iExp, new tree::ConstExp(1))),
                    new tree::SeqStm(
                      bodyExpAndTy->exp_->UnNx(),
                      new tree::SeqStm(
                        new tree::CjumpStm(tree::RelOp::LT_OP, iExp, limitExp, loopLabel, doneLabel),
                        new tree::LabelStm(doneLabel)
                      )
                    )
                  )
                )
              )
            )
          )
        )
      )
    )
  ), type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::NxExp(
    new tree::JumpStm(new tree::NameExp(label), new std::vector<temp::Label *>({label}))
  ), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm* stm = nullptr;

  for(Dec* dec : decs_->GetList()) {
    if(stm == nullptr) {
      stm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
    } else {
      stm = new tree::SeqStm(stm, dec->Translate(venv, tenv, level, label, errormsg)->UnNx());
    }
  }

  tr::ExpAndTy* resultExpAndTy;
  if(body_ != nullptr) {
    resultExpAndTy = body_->Translate(venv, tenv, level, label, errormsg);
    tenv->EndScope();
    venv->EndScope();
    if(!resultExpAndTy->exp_) {
      errormsg->Error(body_->pos_, "let exp error");
      return new tr::ExpAndTy(nullptr, resultExpAndTy->ty_);
    }
    if(stm != nullptr)
      return new tr::ExpAndTy(new tr::ExExp(new tree::EseqExp(stm, resultExpAndTy->exp_->UnEx())), resultExpAndTy->ty_);
    return new tr::ExpAndTy(new tr::ExExp(resultExpAndTy->exp_->UnEx()), resultExpAndTy->ty_);
  } else {
    tenv->EndScope();
    venv->EndScope();
    if(stm != nullptr)
      return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
  }
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* arrTy = tenv->Look(typ_);
  if(arrTy && typeid(*(arrTy->ActualTy())) == typeid(type::ArrayTy)) {
    tr::ExpAndTy* sizeExpAndTy = size_->Translate(venv, tenv, level, label, errormsg);
    tr::ExpAndTy* initExpAndTy = init_->Translate(venv, tenv, level, label, errormsg);
    if(typeid(*(sizeExpAndTy->ty_->ActualTy())) != typeid(type::IntTy)) {
      errormsg->Error(size_->pos_, "integer required");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    if(!initExpAndTy->ty_->IsSameType((static_cast<type::ArrayTy*>(arrTy->ActualTy()))->ty_)) {
      errormsg->Error(init_->pos_, "type mismatch");
      return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
    }
    if(sizeExpAndTy->exp_ && initExpAndTy->exp_) {
      return new tr::ExpAndTy(new tr::ExExp(
        new tree::CallExp(
          new tree::NameExp(temp::LabelFactory::NamedLabel("init_array")),
          new tree::ExpList({ sizeExpAndTy->exp_->UnEx(), initExpAndTy->exp_->UnEx() })
        )
      ), arrTy);
    } else {
      errormsg->Error(pos_, "array exp error");
      return new tr::ExpAndTy(nullptr, arrTy);
    }
  } else {
    errormsg->Error(pos_, "undefined array %s", typ_->Name().data());
    return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
  }
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::NxExp(new tree::ExpStm(new tree::ConstExp(0))), type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  /* function name(params) = exp | function name(params) : result = exp */
  for(FunDec* funDec : functions_->GetList()) {
    /* register function name, including formalTyList and return type */
    FieldList* params = funDec->params_;
    type::TyList* formals = params->MakeFormalTyList(tenv, errormsg);
    type::Ty* returnTy;
    if(funDec->result_) {
      returnTy = tenv->Look(funDec->result_);
      if(!returnTy)  {
        errormsg->Error(funDec->pos_, "undefined typename %s", funDec->result_->Name().data());
        returnTy = type::IntTy::Instance();
      }
    } else {
      returnTy = type::VoidTy::Instance();
    }
    if(venv->Look(funDec->name_))   errormsg->Error(pos_, "two functions have the same name");

    std::list<bool>* escapes = new std::list<bool>;
    for(Field* field : params->GetList())
      escapes->push_back(field->escape_);

    temp::Label* funcLabel = temp::LabelFactory::NamedLabel(funDec->name_->Name());
    tr::Level* funcLevel = tr::Level::NewLevel(level, funcLabel, escapes);
    venv->Enter(funDec->name_, new env::FunEntry(funcLevel, funcLabel, formals, returnTy));
  }

  for(FunDec* funDec : functions_->GetList()) {
    venv->BeginScope();
    FieldList* params = funDec->params_;
    type::TyList* formals = params->MakeFormalTyList(tenv, errormsg);
    env::FunEntry* funEntry = (static_cast<env::FunEntry*>(venv->Look(funDec->name_)));
    std::list<frame::Access*>* accesses = funEntry->level_->frame_->formals_;
    type::Ty* returnTy = funEntry->result_;

    auto formalItr = formals->GetList().begin();
    auto paramItr = params->GetList().begin(), paramItre = params->GetList().end();
    auto accessItr = accesses->begin();
    for(++accessItr; paramItr != paramItre; ++paramItr, ++formalItr, ++accessItr)
      venv->Enter((*paramItr)->name_, new env::VarEntry(new tr::Access(funEntry->level_, (*accessItr)), *formalItr));
    
    tr::ExpAndTy* bodyExpAndTy = funDec->body_->Translate(venv, tenv, funEntry->level_, funEntry->label_, errormsg);
    venv->EndScope();
    if(funDec->result_) {
      if(!bodyExpAndTy->ty_->IsSameType(returnTy))
        errormsg->Error(funDec->body_->pos_, "unmatched return type");
    } else {
      if(!bodyExpAndTy->ty_->IsSameType(type::VoidTy::Instance()))
        errormsg->Error(funDec->body_->pos_, "procedure returns value");
    }

    if(!bodyExpAndTy->exp_) {
      errormsg->Error(funDec->body_->pos_, "func dec error");
    } else {
      tree::Stm* returnStm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), bodyExpAndTy->exp_->UnEx());
    }
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Access* access;
  tr::ExpAndTy* initExpAndTy;
  if(typ_ != nullptr) {
    type::Ty* typTy = tenv->Look(typ_);
    initExpAndTy = init_->Translate(venv, tenv, level, label, errormsg);
    if(!typTy) {
      errormsg->Error(pos_, "undefined typename %s", typ_->Name().data());
      access = tr::Access::AllocLocal(level, escape_);
      venv->Enter(var_, new env::VarEntry(access, initExpAndTy->ty_));
    } else {
      if(!initExpAndTy->ty_->IsSameType(typTy))  errormsg->Error(init_->pos_, "type mismatch");
      access = tr::Access::AllocLocal(level, escape_);
      venv->Enter(var_, new env::VarEntry(access, typTy));
    }
  } else {
    initExpAndTy = init_->Translate(venv, tenv, level, label, errormsg);
    if(typeid(*(initExpAndTy->ty_)) == typeid(type::NilTy))  errormsg->Error(init_->pos_, "init should not be nil without type specified");
    access = tr::Access::AllocLocal(level, escape_);
    venv->Enter(var_, new env::VarEntry(access, initExpAndTy->ty_));
  }
  if(initExpAndTy->exp_)
    return new tr::NxExp(new tree::MoveStm(tr::Exp::GetExpForSimpleVar(access, level)->UnEx(), initExpAndTy->exp_->UnEx()));
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  const std::list<NameAndTy*>& types = types_->GetList();
  for(NameAndTy* nameAndTy : types) {
    if(tenv->Look(nameAndTy->name_))  errormsg->Error(pos_, "two types have the same name");
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  for(NameAndTy* nameAndTy : types) {
    type::Ty* nameTy = nameAndTy->ty_->Translate(tenv, errormsg);
    type::NameTy* oldNameTy = (type::NameTy*)(tenv->Look(nameAndTy->name_));
    oldNameTy->ty_ = nameTy;
    tenv->Set(nameAndTy->name_, oldNameTy);
  }
  for(NameAndTy* nameAndTy : types){
    type::Ty* ty = tenv->Look(nameAndTy->name_);
    type::Ty* st = ty;
    bool flag = false;
    while(typeid(*ty) == typeid(type::NameTy)) {
      if(st == ty) {
        if(!flag) {
          flag = true;
        } else {
          errormsg->Error(pos_, "illegal type cycle");
          return new tr::ExExp(new tree::ConstExp(0));
        }
      }
      ty = (static_cast<type::NameTy*>(ty))->ty_;
    }
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* nameTy = tenv->Look(name_);
  if(!nameTy) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return type::IntTy::Instance();
  }
  return new type::NameTy(name_, nameTy);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList* tyFieldList = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(tyFieldList);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* arrayTy = tenv->Look(array_);
  if(!arrayTy) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    return type::IntTy::Instance();
  }
  return new type::ArrayTy(arrayTy);
}

} // namespace absyn
