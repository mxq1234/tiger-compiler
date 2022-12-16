#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"

namespace col {
using Color = std::string*;

struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class ColorList {
  /* TODO: Put your lab6 code here */
public:
  static ColorList* NewColorList();
  static Color findColor(temp::Temp* t);
  void DeleteColor(Color color) { colors_.remove(color); }
  bool Empty() const { return colors_.empty(); }
  Color getOne() const { return colors_.front(); }

private:
  ColorList() = default;
  void Append(Color color) { colors_.push_back(color); }
  std::list<Color> colors_;
};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
