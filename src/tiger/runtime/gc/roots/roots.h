#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <iostream>
#include <unordered_map>
#include "tiger/frame/temp.h"
#include "tiger/semant/types.h"
#include <map>

namespace gc {

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
};

class PointerMap {
public:
  temp::TempList* temp_list_;
  std::vector<int> roots_;
  std::map<temp::Temp*, int> spills_;
  std::map<temp::Temp*, int> saves_;
  int frame_size_;

  PointerMap() : temp_list_(new temp::TempList) { }
  PointerMap(temp::TempList* temp_list) : temp_list_(temp_list) { }
  PointerMap(temp::TempList* temp_list, std::vector<int> roots, std::map<temp::Temp*, int> spills,
  std::map<temp::Temp*, int> saves, int frame_size)
    : temp_list_(temp_list), roots_(roots), spills_(spills), saves_(saves), frame_size_(frame_size) { }
  void Print(FILE* out_);
  void OutputAssem(FILE* out_);
};

}

namespace temp {

class TempTyMap {
public:
  void Enter(Temp* t, type::Ty* ty);
  type::Ty* Look(Temp* t);

  static TempTyMap* Empty();
  const std::unordered_map<Temp*, type::Ty*>& GetMap() { return *tab_; }

private:
  std::unordered_map<Temp*, type::Ty*> *tab_;

  TempTyMap() : tab_(new std::unordered_map<Temp*, type::Ty*>) { }
  TempTyMap(std::unordered_map<Temp*, type::Ty*> *tab) : tab_(tab) { }
};
}
#endif // TIGER_RUNTIME_GC_ROOTS_H