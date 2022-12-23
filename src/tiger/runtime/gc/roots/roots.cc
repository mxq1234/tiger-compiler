#include "tiger/runtime/gc/roots/roots.h"
#include "tiger/frame/frame.h"
#include <inttypes.h>

extern frame::RegManager *reg_manager;

namespace gc {

void PointerMap::Print(FILE* out_) {
  for(temp::Temp* t : temp_list_->GetList())
    fprintf(out_, "%s ", reg_manager->temp_map_->Look(t)->c_str());
  fprintf(out_, "\n");
  for(int root : roots_)
    fprintf(out_, "%d ", root);
  fprintf(out_, "\n");
  for(auto spill : spills_)
    fprintf(out_, "%s %d\n", reg_manager->temp_map_->Look(spill.first)->c_str(), spill.second);
}

void PointerMap::OutputAssem(FILE* out_) {
  int map_size = 7 + (int)roots_.size() + 2 * (int)spills_.size() + (saves_.empty()? 0 : 1);
  fprintf(out_, ".quad %d\n", map_size);
  fprintf(out_, ".quad %d\n", frame_size_);

  uint64_t reg_map = 0;
  int i = 0;
  char* ptr = (char*)&reg_map;
  for(temp::Temp* t : reg_manager->CalleeSaves()->GetList()) {
    if(temp_list_->Contain(t))  ptr[i] = 1;
    i++;
  }
  fprintf(out_, ".quad %" PRIu64 "\n", reg_map);
  fprintf(out_, ".quad %" PRIu64 "\n", roots_.size());
  for(int x : roots_)
    fprintf(out_, ".quad %d\n", x);
  fprintf(out_, ".quad %" PRIu64 "\n", spills_.size());
  for(const auto& x : spills_) {
    fprintf(out_, ".quad %d\n", reg_manager->GetCalleeRegisterNo(x.first));
    fprintf(out_, ".quad %d\n", x.second);
  }
  if(saves_.empty()) {
    fprintf(out_, ".quad 0\n");
  } else {
    fprintf(out_, ".quad 1\n");
    fprintf(out_, ".quad %d\n", saves_[reg_manager->FramePointer()]);
  }
}

}

namespace temp {
TempTyMap* TempTyMap::Empty() { return new TempTyMap(); }

void TempTyMap::Enter(Temp* t, type::Ty* ty) {
  assert((*tab_)[t] == nullptr);
  (*tab_)[t] = ty;
}

type::Ty* TempTyMap::Look(Temp* t) {
  return (*tab_)[t];
}
}
