#include "straightline/slp.h"

#include <iostream>
#define MAX2(a, b) ((a > b)? a : b)

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return MAX2(stm1->MaxArgs(), stm2->MaxArgs());
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return MAX2(exps->MaxArgs(), exps->NumExps());
}

int A::IdExp::MaxArgs() const {
  return 0;
}

int A::NumExp::MaxArgs() const {
  return 0;
}

int A::OpExp::MaxArgs() const {
  return MAX2(left->MaxArgs(), right->MaxArgs());
}

int A::EseqExp::MaxArgs() const {
  return MAX2(stm->MaxArgs(), exp->MaxArgs());
}

int A::PairExpList::MaxArgs() const {
  return MAX2(exp->MaxArgs(), tail->MaxArgs());
}

int A::LastExpList::MaxArgs() const {
  return exp->MaxArgs();
}

int A::PairExpList::NumExps() const {
  return tail->NumExps() + 1;
}

int A::LastExpList::NumExps() const {
  return 1;
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = stm1->Interp(t);
  t = stm2->Interp(t);
  return t;
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable* res = exp->Interp(t);
  t = res->t;
  t = t->Update(id, res->i);
  delete res;
  return t;
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable* res = exps->Interp(t);
  t = res->t;
  std::cout << std::endl;
  return t;
}

IntAndTable *A::IdExp::Interp(Table *t) const {
  return new IntAndTable(t->Lookup(id), t);
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  return new IntAndTable(num, t);
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  IntAndTable* lftRes = left->Interp(t);
  t = lftRes->t;
  IntAndTable* rhtRes = right->Interp(t);
  t = rhtRes->t;
  int res = 0;
  switch (oper)
  {
  case PLUS:  res = lftRes->i + rhtRes->i; break;
  case MINUS: res = lftRes->i - rhtRes->i; break;
  case TIMES: res = lftRes->i * rhtRes->i; break;
  case DIV:   res = lftRes->i / rhtRes->i; break;
  default: break;
  }
  delete lftRes;
  delete rhtRes;
  return new IntAndTable(res, t);
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  t = stm->Interp(t);
  return exp->Interp(t);
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  IntAndTable* expRes = exp->Interp(t);
  int i = expRes->i;
  t = expRes->t;
  std::cout << i << " ";

  IntAndTable* tailRes = tail->Interp(t);
  t = tailRes->t;

  delete expRes;
  delete tailRes;
  return new IntAndTable(i, t);
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  IntAndTable* res = exp->Interp(t);
  t = res->t;
  std::cout << res->i << " ";

  return res;
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
