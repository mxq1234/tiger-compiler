#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <unordered_map>

namespace ra {

using WorkList = live::INodeList;
using WorkListPtr = live::INodeListPtr;
using NodesList = live::INodeList;
using NodesListPtr = live::INodeListPtr;
using MoveList = live::MoveList;
using SelectStack = live::INodeList;
using DegreeMap = std::unordered_map<live::INodePtr, int>;
using MoveListMap = std::unordered_map<live::INodePtr, MoveList*>;
using AliasMap = std::unordered_map<live::INodePtr, live::INodePtr>;
using ColorMap = std::unordered_map<live::INodePtr, col::Color>;

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() {}
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  RegAllocator(frame::Frame* frame, std::unique_ptr<cg::AssemInstr> assem_instr);
  ~RegAllocator();
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult();

private:
  void LivenessAnalysis();
  void Build();
  void MakeWorkList();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void AssignColors();
  void RewriteProgram();

private:
  NodesListPtr Adjacent(live::INodePtr node) const;
  MoveList* NodeMoves(live::INodePtr node);
  bool MoveRelated(live::INodePtr node);
  void DecrementDegree(live::INodePtr node);
  void EnableMoves(live::INodeListPtr nodes);
  void AddWorkList(live::INodePtr node);
  bool Briggs(live::INodePtr u, live::INodePtr v);
  bool Geroge(live::INodePtr u, live::INodePtr v);
  bool OK(live::INodePtr t, live::INodePtr r);
  void Combine(live::INodePtr u, live::INodePtr v);
  live::INodePtr GetAlias(live::INodePtr node);
  void FreezeMoves(live::INodePtr u);

private:
  frame::Frame* frame_;
  std::unique_ptr<cg::AssemInstr> assem_instr_;
  fg::FlowGraphFactory* fg_;
  live::LiveGraphFactory* live_;

  WorkListPtr precolored_;         // 机器寄存器集合，每个寄存器预先指派一种颜色
  WorkListPtr initial_;             // 临时寄存器集合，既没有预着色，也没有处理

  WorkListPtr simplify_list_;       // 低度数的传送无关的结点表
  WorkListPtr freeze_list_;         // 低度数的传送有关的结点表
  WorkListPtr spill_list_;          // 高度数的结点表

  NodesListPtr spilled_nodes_;      // 在本轮中要被溢出的结点集合，初始为空
  NodesListPtr coalesced_nodes_;    // 已合并的寄存器集合，将一对可以合并的结点中的一个放入此表，另一个放回工作表
  NodesListPtr colored_nodes_;      // 已成功着色的寄存器集合

  SelectStack* select_stack_;       // 包含从图中删除的临时变量的栈

  MoveList* coalesced_moves_;       // 已合并的move指令集合
  MoveList* constrained_moves_;     // 源操作数和目标操作数冲突的move指令集合
  MoveList* frozen_moves_;          // 不再考虑合并的move指令集合
  MoveList* worklist_moves_;        // 有可能合并的move指令集合
  MoveList* active_moves_;          // 还未做好合并准备的move指令集合

  DegreeMap degree_map_;            // 每个结点当前度数
  MoveListMap movelist_map_;        // 从一个结点到与该结点相关的MoveList的映射
  AliasMap alias_map_;              // (u, v)被合并，v属于coalesced_moves_，则alias[v]=u
  ColorMap color_map_;              // 算法为结点选择的颜色，对于预着色结点，其初值为给定的颜色
};

} // namespace ra

#endif