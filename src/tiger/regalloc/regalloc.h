#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <map>
#include <set>

namespace ra {

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
  ~Result();
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> il);
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult();
  fg::FGraphPtr getFlowGraph(){return flow_graph_;}

private:
  std::unique_ptr<cg::AssemInstr> il_;
  frame::Frame *frame_;
  std::unique_ptr<ra::Result> result_;
  fg::FGraphPtr flow_graph_;
  live::LiveGraph live_graph_;
  tab::Table<temp::Temp, live::INode> *temp_node_map;

public:
  void Build();
  void MakeWorklist();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void AssginColor();
  void RewriteProgram(live::INodeListPtr);

  live::INodePtr HeuristicSelect();
  void AddEdge(live::INodePtr u, live::INodePtr v);
  live::INodeListPtr Adjacent(live::INodePtr n);
  live::MoveList *NodeMoves(live::INodePtr n);
  void Decrement(live::INodePtr node);
  void EnableMoves(live::INodeListPtr list);
  bool MoveRelated(live::INodePtr n);
  live::INodePtr GetAlias(live::INodePtr n);
  bool OK(live::INodePtr t, live::INodePtr r);
  bool Conservertive(live::INodeListPtr nodes);
  void addWorkList(live::INodePtr u);
  void Combine(live::INodePtr u, live::INodePtr v);
  void FreezeMoves(live::INodePtr u);
  void init();

private:
  live::INodeListPtr precolored = nullptr;
  live::INodeListPtr initial = nullptr;

  live::INodeListPtr simplifyWorklist = nullptr;
  live::INodeListPtr freezeWorklist = nullptr;
  live::INodeListPtr spillWorklist = nullptr;
  live::INodeListPtr spilledNodes = nullptr;
  live::INodeListPtr coalescedNodes = nullptr;
  live::INodeListPtr coloredNodes = nullptr;
  live::INodeListPtr selectStack = nullptr;


  live::MoveList *worklistMoves = nullptr;
  live::MoveList *coalescedMoves = nullptr;
  live::MoveList *frozenMoves = nullptr;
  live::MoveList *activeMoves = nullptr;
  live::MoveList *constrainedMoves = nullptr;

  std::map<live::INodePtr, live::MoveList *> moveList;

  std::set<std::pair<live::INodePtr, live::INodePtr>> adjSet;
  std::map<live::INodePtr, live::INodeListPtr> adjList;
  std::map<live::INodePtr, int> degree;
  std::map<live::INodePtr, live::INodePtr> alias;
  std::map<live::INodePtr, temp::Temp *> color;


  int K;

  // for debug
  FILE *file;
};

} // namespace ra

#endif