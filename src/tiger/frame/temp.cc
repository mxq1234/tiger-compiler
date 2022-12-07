#include "tiger/frame/temp.h"

#include <cstdio>
#include <set>
#include <sstream>

namespace temp {

LabelFactory LabelFactory::label_factory;
TempFactory TempFactory::temp_factory;

Label *LabelFactory::NewLabel() {
  char buf[100];
  sprintf(buf, "L%d", label_factory.label_id_++);
  return NamedLabel(std::string(buf));
}

/**
 * Get symbol of a label_. The label_ will be created only if it is not found.
 * @param s label_ string
 * @return symbol
 */
Label *LabelFactory::NamedLabel(std::string_view s) {
  return sym::Symbol::UniqueSymbol(s);
}

std::string LabelFactory::LabelString(Label *s) { return s->Name(); }

Temp *TempFactory::NewTemp() {
  Temp *p = new Temp(temp_factory.temp_id_++);
  std::stringstream stream;
  stream << 't';
  stream << p->num_;
  Map::Name()->Enter(p, new std::string(stream.str()));

  return p;
}

int Temp::Int() const { return num_; }

Map *Map::Empty() { return new Map(); }

Map *Map::Name() {
  static Map *m = nullptr;
  if (!m)
    m = Empty();
  return m;
}

Map *Map::LayerMap(Map *over, Map *under) {
  if (over == nullptr)
    return under;
  else
    return new Map(over->tab_, LayerMap(over->under_, under));
}

void Map::Enter(Temp *t, std::string *s) {
  assert(tab_);
  tab_->Enter(t, s);
}

std::string *Map::Look(Temp *t) {
  std::string *s;
  assert(tab_);
  s = tab_->Look(t);
  if (s)
    return s;
  else if (under_)
    return under_->Look(t);
  else
    return nullptr;
}

void Map::DumpMap(FILE *out) {
  tab_->Dump([out](temp::Temp *t, std::string *r) {
    fprintf(out, "t%d -> %s\n", t->Int(), r->data());
  });
  if (under_) {
    fprintf(out, "---------\n");
    under_->DumpMap(out);
  }
}

void TempList::Union(TempList* list) {
  for(temp::Temp* t : list->GetList())
    temp_list_.push_back(t);
  temp_list_.sort();
  temp_list_.unique();
}

TempList* TempList::Remove(TempList* list) {
  TempList* newTempList = new temp::TempList;
  std::list<temp::Temp*> newList(GetList());
  for(temp::Temp* t : list->GetList())
    newList.remove(t);
  for(temp::Temp* t : newList)
    newTempList->Append(t);
  return newTempList;
}

bool TempList::Equal(TempList* list) const {
  if(list->GetList().size() != temp_list_.size())   return false;
  for(temp::Temp* t : temp_list_) {
    auto itr = std::find(list->GetList().begin(), list->GetList().end(), t);
    if(itr == list->GetList().end())  return false;
  }
  for(temp::Temp* t : list->GetList()) {
    auto itr = std::find(temp_list_.begin(), temp_list_.end(), t);
    if(itr == temp_list_.end()) return false;
  }
  return true;
}

void TempList::Replace(Temp* oldTemp, Temp* newTemp) {
  std::list<Temp *>::iterator itr;
  itr = std::find(temp_list_.begin(), temp_list_.end(), oldTemp);
  if(itr == temp_list_.end())   return;
  *itr = newTemp;
}

bool TempList::Contain(Temp* t) const {
  return (std::find(temp_list_.begin(), temp_list_.end(), t) != temp_list_.end());
}

} // namespace temp