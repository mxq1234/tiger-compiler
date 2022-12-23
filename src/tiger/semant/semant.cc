#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  int labelcount = 0;
  root_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry* entry = venv->Look(sym_);
  if(entry && typeid(*entry) == typeid(env::VarEntry)) {
    return (static_cast<env::VarEntry*>(entry))->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(varTy && typeid(*(varTy->ActualTy())) == typeid(type::RecordTy)) {
    type::FieldList* fieldList = (static_cast<type::RecordTy*>(varTy->ActualTy()))->fields_;
    type::Ty* fieldType = nullptr;
    for(type::Field* field : fieldList->GetList()) {
      if(field->name_ != sym_)  continue;
      fieldType = field->ty_;
      break;
    }
    if(fieldType == nullptr) {
      errormsg->Error(pos_, "field nam doesn't exist");
      return type::IntTy::Instance();
    }
    return fieldType;
  } else {
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty* subscriptTy = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*(varTy->ActualTy())) != typeid(type::ArrayTy))
    errormsg->Error(var_->pos_, "array type required");
  if(typeid(*subscriptTy) == typeid(type::IntTy)) {
    return (static_cast<type::ArrayTy*>(varTy->ActualTy()))->ty_;
  } else {
    errormsg->Error(subscript_->pos_, "integer required");
    return type::IntTy::Instance();
  }
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry* entry = venv->Look(func_);
  if(entry && typeid(*entry) == typeid(env::FunEntry)) {
    type::Ty* result = (static_cast<env::FunEntry*>(entry))->result_;
    const std::list<type::Ty*>& formalsList = (static_cast<env::FunEntry*>(entry))->formals_->GetList();
    const std::list<absyn::Exp*>& actualsList = args_->GetList();
    auto formalItr = formalsList.begin();
    auto actualItr = actualsList.begin();
    while(formalItr != formalsList.end() && actualItr != actualsList.end()) {
      type::Ty* actualTy = (*actualItr)->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!actualTy->IsSameType(*formalItr))
        errormsg->Error((*actualItr)->pos_, "para type mismatch");
      ++formalItr;
      ++actualItr;
    }

    if(formalItr != formalsList.end())
      errormsg->Error(pos_, "call with too little params");
    if(actualItr != actualsList.end())
      errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
    return result;
  } else {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* leftTy = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty* rightTy = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  switch (oper_)
  {
  case Oper::PLUS_OP: case Oper::MINUS_OP: case Oper::TIMES_OP: case Oper::DIVIDE_OP:
  case Oper::AND_OP: case Oper::OR_OP:
    if(typeid(*leftTy) != typeid(type::IntTy))
      errormsg->Error(left_->pos_, "integer required");
    if(typeid(*rightTy) != typeid(type::IntTy))
      errormsg->Error(right_->pos_, "integer required");
    break;
  case Oper::EQ_OP: case Oper::NEQ_OP: case Oper::GE_OP: case Oper::GT_OP:
  case Oper::LE_OP: case Oper::LT_OP:
    if(!leftTy->IsSameType(rightTy))
      errormsg->Error(pos_, "same type required");
    break;
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  /* RecordTypeID{id=exp, id=exp, ...} */
  type::Ty* typTy = tenv->Look(typ_);
  if(!typTy) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::IntTy::Instance();
  }
  if(typeid(*(typTy->ActualTy())) != typeid(type::RecordTy))
    errormsg->Error(pos_, "undefined record type %s", typ_->Name().data());

  const std::list<type::Field*>& formalsList = (static_cast<type::RecordTy*>(typTy->ActualTy()))->fields_->GetList();
  const std::list<EField*>& actualsList = fields_->GetList();
  auto formalItr = formalsList.begin(), formalItre = formalsList.end();
  auto actualItr = actualsList.begin(), actualItre = actualsList.end();
  while(formalItr != formalItre && actualItr != actualItre) {
    if((*formalItr)->name_ != (*actualItr)->name_)
      errormsg->Error(pos_, "unmatched field %s", (*actualItr)->name_);
    type::Ty* actualTy = (*actualItr)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);

    if(!(*formalItr)->ty_->IsSameType(actualTy))
      errormsg->Error(pos_, "incorrect type of %s", (*actualItr)->name_);
    ++formalItr;
    ++actualItr;
  }

  if(formalItr != formalItre)
    errormsg->Error(pos_, "create record with too little fields");
  if(actualItr != actualItre)
    errormsg->Error(pos_, "create record with too many fields");
  return typTy;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  const std::list<Exp*>& expList = seq_->GetList();
  auto itr = expList.begin(), itre = expList.end();
  type::Ty* result;
  for(; itr != itre; ++itr)
    result = (*itr)->SemAnalyze(venv, tenv, labelcount, errormsg);
  return result;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty* expTy = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if(!varTy->IsSameType(expTy))
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

  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* testTy = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(!testTy->IsSameType(type::IntTy::Instance()))
    errormsg->Error(test_->pos_, "integer exp required");

  type::Ty* thenTy = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(elsee_) {
    type::Ty* elseTy = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(!thenTy->IsSameType(elseTy))
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
    return thenTy;
  } else {
    if(typeid(*thenTy) != typeid(type::VoidTy))
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    return type::VoidTy::Instance();
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* testTy = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!testTy->IsSameType(type::IntTy::Instance()))
    errormsg->Error(test_->pos_, "integer exp required");

  labelcount++;
  type::Ty* bodyTy = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  labelcount--;
  if(typeid(*bodyTy) != typeid(type::VoidTy))
    errormsg->Error(body_->pos_, "while body must produce no value");
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();

  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty* lowTy = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty* highTy = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!lowTy->IsSameType(type::IntTy::Instance()))
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  if(!highTy->IsSameType(type::IntTy::Instance()))
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");

  labelcount++;
  type::Ty* bodyTy = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  labelcount--;
  if(!bodyTy->IsSameType(type::VoidTy::Instance()))
    errormsg->Error(body_->pos_, "void exp required");

  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(labelcount == 0)   errormsg->Error(pos_, "break is not inside any loop");
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();

  for(Dec* dec : decs_->GetList())
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);

  type::Ty* result = ((body_ != nullptr)? body_->SemAnalyze(venv, tenv, labelcount, errormsg) : type::VoidTy::Instance());

  tenv->EndScope();
  venv->EndScope();
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  /* ArrayTypeId[sizeExp] of initExp */
  type::Ty* arrTy = tenv->Look(typ_);
  if(arrTy && typeid(*(arrTy->ActualTy())) == typeid(type::ArrayTy)) {
    type::Ty* sizeTy = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
    type::Ty* initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(typeid(*(sizeTy->ActualTy())) != typeid(type::IntTy))
      errormsg->Error(size_->pos_, "integer required");
    if(!initTy->IsSameType((static_cast<type::ArrayTy*>(arrTy->ActualTy()))->ty_))
      errormsg->Error(init_->pos_, "type mismatch");
    return arrTy;
  } else {
    errormsg->Error(pos_, "undefined array %s", typ_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  /* function name(params) = exp | function name(params) : result = exp */
  for(FunDec* funDec : functions_->GetList()) {
    /* register function name, including formalTyList and return type */
    FieldList* params = funDec->params_;
    type::TyList* formals = params->MakeFormalTyList(tenv, errormsg);
    type::Ty* returnTy;
    if(funDec->result_) {
      returnTy = tenv->Look(funDec->result_);
      if(!returnTy)   errormsg->Error(funDec->pos_, "undefined typename %s", funDec->result_->Name().data());
    } else {
      returnTy = type::VoidTy::Instance();
    }
    if(venv->Look(funDec->name_))   errormsg->Error(pos_, "two functions have the same name");
    venv->Enter(funDec->name_, new env::FunEntry(formals, returnTy));
  }

  for(FunDec* funDec : functions_->GetList()) {
    venv->BeginScope();
    FieldList* params = funDec->params_;
    type::TyList* formals = params->MakeFormalTyList(tenv, errormsg);
    type::Ty* returnTy = (static_cast<env::FunEntry*>(venv->Look(funDec->name_)))->result_;

    auto formalItr = formals->GetList().begin();
    auto paramItr = params->GetList().begin(), paramItre = params->GetList().end();
    for(; paramItr != paramItre; ++paramItr, ++formalItr)
      venv->Enter((*paramItr)->name_, new env::VarEntry(*formalItr));
    type::Ty* bodyType = funDec->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(funDec->result_) {
      if(!bodyType->IsSameType(returnTy))
        errormsg->Error(funDec->body_->pos_, "unmatched return type");
    } else {
      if(!bodyType->IsSameType(type::VoidTy::Instance()))
        errormsg->Error(funDec->body_->pos_, "procedure returns value");
    }

    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(typ_ != nullptr) {
    type::Ty* typTy = tenv->Look(typ_);
    type::Ty* initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(!typTy) {
      errormsg->Error(pos_, "undefined typename %s", typ_->Name().data());
      venv->Enter(var_, new env::VarEntry(initTy));
    } else {
      if(!initTy->IsSameType(typTy))  errormsg->Error(init_->pos_, "type mismatch");
      venv->Enter(var_, new env::VarEntry(typTy));
    }
  } else {
    type::Ty* initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(typeid(*initTy) == typeid(type::NilTy))  errormsg->Error(init_->pos_, "init should not be nil without type specified");
    venv->Enter(var_, new env::VarEntry(initTy));
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  const std::list<NameAndTy*>& types = types_->GetList();
  for(NameAndTy* nameAndTy : types) {
    if(tenv->Look(nameAndTy->name_))  errormsg->Error(pos_, "two types have the same name");
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  for(NameAndTy* nameAndTy : types) {
    type::Ty* nameTy = nameAndTy->ty_->SemAnalyze(tenv, errormsg);
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
          return;
        }
      }
      ty = (static_cast<type::NameTy*>(ty))->ty_;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* nameTy = tenv->Look(name_);
  if(!nameTy) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return type::IntTy::Instance();
  }
  return new type::NameTy(name_, nameTy);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::FieldList* tyFieldList = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(tyFieldList);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* arrayTy = tenv->Look(array_);
  if(!arrayTy) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    return type::IntTy::Instance();
  }
  return new type::ArrayTy(arrayTy);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
