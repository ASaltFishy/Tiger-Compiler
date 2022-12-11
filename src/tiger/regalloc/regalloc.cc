#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
Result::~Result() {
  delete coloring_;
  delete il_;
}

std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
  return std::move(result_);
}

RegAllocator::RegAllocator(frame::Frame *frame,
                           std::unique_ptr<cg::AssemInstr> il)
    : frame_(frame), il_(std::move(il)), result_(nullptr) {

  // flow graph
  printf("il_ size: %ld\n", il_->GetInstrList()->GetList().size());
  fg::FlowGraphFactory flow_graph_factory(il_.get()->GetInstrList());
  flow_graph_factory.AssemFlowGraph();
  flow_graph_ = flow_graph_factory.GetFlowGraph();
  printf("flow_graph_ size:%d\n", flow_graph_->nodecount_);

  // live graph
  live::LiveGraphFactory live_graph_factory(flow_graph_);
  live_graph_factory.Liveness();
  live_graph_ = live_graph_factory.GetLiveGraph();
  temp_node_map = live_graph_factory.GetTempNodeMap();

  // initialize k
  K = reg_manager->intialInterfere()->GetList().size();

  // initialize other data structure
  simplifyWorklist = new live::INodeList();
  freezeWorklist = new live::INodeList();
  spillWorklist = new live::INodeList();
  spilledNodes = new live::INodeList();
  precolored = new live::INodeList();
  initial = new live::INodeList();
  coalescedNodes = new live::INodeList();
  coloredNodes = new live::INodeList();
  selectStack = new live::INodeList();
  worklistMoves = new live::MoveList();
  coalescedMoves = new live::MoveList();
  constrainedMoves = new live::MoveList();
  frozenMoves = new live::MoveList();
  activeMoves = new live::MoveList();

  for (temp::Temp *reg : reg_manager->intialInterfere()->GetList()) {
    precolored->Append(temp_node_map->Look(reg));
  }
  for (live::INodePtr node : live_graph_.interf_graph->Nodes()->GetList()) {
    initial->Append(node);
  }
  for (auto node : live_graph_.interf_graph->Nodes()->GetList()) {
    adjList[node] = new live::INodeList();
    degree[node] = 0;
  }
}

void RegAllocator::RegAlloc() {

  printf("begin build\n");
  // Build
  Build();

  // make worklist
  MakeWorklist();

  bool terminate = false;
  while (!terminate) {
    terminate = true;
    if (!simplifyWorklist->GetList().empty()) {
      Simplify();
      terminate = false;
    } else if (!worklistMoves->GetList().empty()) {
      Coalesce();
      terminate = false;
    } else if (!freezeWorklist->GetList().empty()) {
      Freeze();
      terminate = false;
    } else if (!spillWorklist->GetList().empty()) {
      SelectSpill();
      terminate = false;
    }
  }

  AssginColor();
  if (!spilledNodes->GetList().empty()) {
    RewriteProgram(spilledNodes);
    RegAlloc();
  }
}

void RegAllocator::Build() {
  worklistMoves = live_graph_.moves;
  printf("woeklistmoves size: %ld\n", worklistMoves->GetList().size());
  for (std::pair<live::INodePtr, live::INodePtr> item :
       worklistMoves->GetList()) {
    live::INodePtr src = item.first;
    live::INodePtr dst = item.second;
    if (moveList.find(src) == moveList.end())
      moveList[src] = new live::MoveList();
    moveList[src]->Append(src, dst);
    if (moveList.find(dst) == moveList.end())
      moveList[dst] = new live::MoveList();
    moveList[dst]->Append(src, dst);
  }
  // init adjList and adjSet
  for (live::INodePtr node : live_graph_.interf_graph->Nodes()->GetList()) {
    for (live::INodePtr adj : node->Adj()->GetList()) {
      AddEdge(node, adj);
    }
  }
}

void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  if (u != v && (adjSet.find(std::make_pair(u, v)) == adjSet.end())) {
    adjSet.insert(std::make_pair(u, v));
    adjSet.insert(std::make_pair(v, u));
    if (!precolored->Contain(u)) {
      adjList[u]->Append(v);
      degree[u]++;
    }
    if (!precolored->Contain(v)) {
      adjList[v]->Append(u);
      degree[v]++;
    }
  }
}

void RegAllocator::MakeWorklist() {
  for (live::INodePtr node : initial->GetList()) {
    if (degree[node] >= K)
      spillWorklist->Append(node);
    else if (moveList.find(node) != moveList.end())
      freezeWorklist->Append(node);
    else
      simplifyWorklist->Append(node);
  }
  initial->Clear();
}

void RegAllocator::Simplify() {
  if (simplifyWorklist->GetList().size() != 0) {
    auto node = simplifyWorklist->GetList().front();
    selectStack->Prepend(node);
    for (auto adj : Adjacent(node)->GetList()) {
      degree[adj]--;
    }
    simplifyWorklist->DeleteNode(node);
  }
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr n) {
  return adjList[n]->Diff(selectStack->Union(coalescedNodes));
}
live::MoveList *RegAllocator::NodeMoves(live::INodePtr n) {
  return moveList[n]->Intersect(activeMoves->Union(worklistMoves));
}
bool RegAllocator::MoveRelated(live::INodePtr n) {
  return !NodeMoves(n)->GetList().empty();
}

void RegAllocator::Decrement(live::INodePtr node) {
  int d = degree[node];
  degree[node]--;
  if (d == K) {
    live::INodeList list = *node->Adj();
    list.Append(node);
    EnableMoves(&list);
    spillWorklist->DeleteNode(node);
    if (MoveRelated(node)) {
      freezeWorklist->Append(node);
    } else {
      simplifyWorklist->Append(node);
    }
  }
}

void RegAllocator::EnableMoves(live::INodeListPtr list) {
  for (auto node : list->GetList()) {
    for (auto move : NodeMoves(node)->GetList()) {
      if (activeMoves->Contain(move.first, move.second)) {
        activeMoves->Delete(move.first, move.second);
        worklistMoves->Append(move.first, move.second);
      }
    }
  }
}

void RegAllocator::Coalesce() {
  for (auto move : worklistMoves->GetList()) {
    live::INodePtr x = GetAlias(move.first);
    live::INodePtr y = GetAlias(move.second);
    live::INodePtr u = x, v = y;
    if (precolored->Contain(y)) {
      u = y;
      v = x;
    }
    worklistMoves->Delete(move.first, move.second);
    if (u == v) {
      coalescedMoves->Append(move.first, move.second);
      addWorkList(u);
    } else if (precolored->Contain(v) ||
               adjSet.find(std::make_pair(u, v)) != adjSet.end()) {
      constrainedMoves->Append(move.first, move.second);
      addWorkList(u);
      addWorkList(v);
    } else {
      bool ok = true;
      for (auto t : Adjacent(v)->GetList()) {
        if (!OK(t, u)) {
          ok = false;
          break;
        }
      }
      if (precolored->Contain(u) && ok ||
          !precolored->Contain(u) &&
              Conservertive(Adjacent(v)->Union(Adjacent(u)))) {
        coalescedMoves->Append(move.first, move.second);
        Combine(u, v);
        addWorkList(u);
      } else {
        activeMoves->Append(move.first, move.second);
      }
    }
  }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr n) {
  if (coalescedNodes->Contain(n))
    return GetAlias(alias[n]);
  else
    return n;
}
bool RegAllocator::OK(live::INodePtr t, live::INodePtr r) {
  return degree[t] < K || precolored->Contain(t) ||
         adjSet.find(std::make_pair(t, r)) != adjSet.end();
}
bool RegAllocator::Conservertive(live::INodeListPtr nodes) {
  int k = 0;
  for (auto n : nodes->GetList()) {
    if (degree[n] >= K)
      k++;
  }
  return k < K;
}
void RegAllocator::addWorkList(live::INodePtr u) {
  if (!precolored->Contain(u) && !MoveRelated(u) && degree[u] < K) {
    freezeWorklist->DeleteNode(u);
    simplifyWorklist->Append(u);
  }
}
void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  if (freezeWorklist->Contain(v)) {
    freezeWorklist->DeleteNode(v);
  } else {
    spillWorklist->DeleteNode(v);
  }
  coalescedNodes->Append(v);
  alias[v] = u;
  moveList[u]->Union(moveList[v]);
  auto toBeEnable = new live::INodeList();
  toBeEnable->Append(v);
  EnableMoves(toBeEnable);
  for (auto t : Adjacent(v)->GetList()) {
    AddEdge(t, u);
    Decrement(t);
  }
  if (degree[u] >= K && freezeWorklist->Contain(u)) {
    freezeWorklist->DeleteNode(u);
    spillWorklist->Append(u);
  }
}

void RegAllocator::Freeze() {
  for (auto u : freezeWorklist->GetList()) {
    freezeWorklist->DeleteNode(u);
    simplifyWorklist->Append(u);
    FreezeMoves(u);
  }
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
  live::INodePtr v;
  for (auto m : NodeMoves(u)->GetList()) {
    if (GetAlias(m.second) == GetAlias(u))
      v = GetAlias(m.first);
    else
      v = GetAlias(m.second);
    activeMoves->Delete(m.first, m.second);
    frozenMoves->Append(m.first, m.second);
    if (!MoveRelated(v) && degree[v] < K) {
      freezeWorklist->DeleteNode(v);
      simplifyWorklist->Append(v);
    }
  }
}
void RegAllocator::SelectSpill() {
  for (auto m : spillWorklist->GetList()) {
    spillWorklist->DeleteNode(m);
    simplifyWorklist->Append(m);
    FreezeMoves(m);
  }
}
void RegAllocator::AssginColor() {
  auto stack = selectStack->GetList();
  while (!stack.empty()) {
    live::INodePtr n = stack.front();
    stack.pop_front();

    std::vector<temp::Temp *> okColors;
    for (auto reg : reg_manager->intialInterfere()->GetList()) {
      okColors.push_back(reg);
    }
    for (auto w : adjList[n]->GetList()) {
      live::INodePtr alias = GetAlias(w);
      if (coloredNodes->Contain(alias) || precolored->Contain(alias))
        for (auto iter = okColors.begin(); iter != okColors.end(); iter++) {
          if (*iter == color[alias]) {
            okColors.erase(iter);
            break;
          }
        }
    }
    if (okColors.empty()) {
      spillWorklist->Append(n);
    } else {
      coloredNodes->Append(n);
      color[n] = okColors.front();
    }
  }
}
void RegAllocator::RewriteProgram(live::INodeListPtr spilledList) {
  std::list<assem::Instr *> instr_list = il_->GetInstrList()->GetList();
  std::vector<temp::Temp *> newTemps;
  for (auto node : spilledList->GetList()) {
    for (auto instr = instr_list.begin(); instr != instr_list.end(); instr++) {
      // def
      if ((*instr)->Def()->Contain(node->NodeInfo())) {
        temp::Temp *newTemp = temp::TempFactory::NewTemp();
        newTemps.push_back(newTemp);
        (*instr)->Def()->Replace(node->NodeInfo(), newTemp);
        int offset = frame_->ExpandFrame(1);
        std::string assem = "movq `s0, (" + frame_->GetLabel() + "_framesize-" +
                            std::to_string(offset) + ")(`d0)";
        instr_list.insert(++instr,
                          new assem::OperInstr(
                              assem,
                              new temp::TempList({reg_manager->StackPointer()}),
                              new temp::TempList({newTemp}), nullptr));
        instr--;
      }
      // use
      if ((*instr)->Use()->Contain(node->NodeInfo())) {
        temp::Temp *newTemp = temp::TempFactory::NewTemp();
        newTemps.push_back(newTemp);
        (*instr)->Use()->Replace(node->NodeInfo(), newTemp);
        int offset = frame_->ExpandFrame(1);
        std::string assem = "movq (" + frame_->GetLabel() + "_framesize-" +
                            std::to_string(offset) + ")(`s0), `d0";
        instr_list.insert(instr,
                          new assem::OperInstr(
                              assem, new temp::TempList({newTemp}),
                              new temp::TempList({reg_manager->StackPointer()}),
                              nullptr));
      }
    }
  }
}

} // namespace ra