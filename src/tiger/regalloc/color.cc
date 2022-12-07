#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */
ColorList* ColorList::NewColorList() {
  ColorList* colorList = new ColorList;
  temp::TempList* reg_list_ = reg_manager->Registers();
  reg_list_->Append(reg_manager->StackPointer());
  for(temp::Temp* t : reg_list_->GetList())
    colorList->Append(reg_manager->temp_map_->Look(t));
  return colorList;
}

Color ColorList::findColor(temp::Temp* t) {
  return reg_manager->temp_map_->Look(t);
}
} // namespace col
