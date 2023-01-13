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
}

void RegAllocator::init() {
  precolored->Clear();
  initial->Clear();
  spilledNodes->Clear();
  coloredNodes->Clear();
  coalescedNodes->Clear();
  if (selectStack)
    delete selectStack;
  selectStack = new live::INodeList();
  if (worklistMoves)
    delete worklistMoves;
  worklistMoves = new live::MoveList();
  if (coalescedMoves)
    delete coalescedMoves;
  coalescedMoves = new live::MoveList();
  if (constrainedMoves)
    delete constrainedMoves;
  constrainedMoves = new live::MoveList();
  if (frozenMoves)
    delete frozenMoves;
  frozenMoves = new live::MoveList();
  if (activeMoves)
    delete activeMoves;
  activeMoves = new live::MoveList();

  moveList.clear();
  adjSet.clear();
  adjList.clear();
  degree.clear();
  color.clear();
  alias.clear();
}

bool isTempMove(assem::Instr *instr){
  if(typeid(*instr) == typeid(assem::MoveInstr)){
    std::string str = ((assem::MoveInstr *)instr)->assem_;
    if(str.find("movq")!=std::string::npos && str.find("(")==std::string::npos)return true;
    else return false;
  }else if(typeid(*instr) == typeid(assem::OperInstr)){
    std::string str = ((assem::OperInstr *)instr)->assem_;
    if(str.find("movq")!=std::string::npos && str.find("(")==std::string::npos)return true;
    else return false;
  }else{
    return false;
  }
}

void RegAllocator::RegAlloc() {

  file = fopen("../debug.log", "w");
  while (true) {
    // liveness analysis
    fprintf(file, "-------====liveness analysis=====-----\n");
    // flow graph
    fprintf(file, "il_ size: %ld\n", il_->GetInstrList()->GetList().size());
    fg::FlowGraphFactory flow_graph_factory(il_.get()->GetInstrList());
    flow_graph_factory.AssemFlowGraph();
    flow_graph_ = flow_graph_factory.GetFlowGraph();
    fprintf(file, "flow_graph_ size:%d\n", flow_graph_->nodecount_);

    // live graph
    live::LiveGraphFactory live_graph_factory(flow_graph_);
    live_graph_factory.Liveness();
    live_graph_ = live_graph_factory.GetLiveGraph();
    temp_node_map = live_graph_factory.GetTempNodeMap();

    fprintf(file, "-------====init initial and precolored=====-----\n");
    for (temp::Temp *reg : reg_manager->intialInterfere()->GetList()) {
      precolored->Append(temp_node_map->Look(reg));
    }
    for (live::INodePtr node : live_graph_.interf_graph->Nodes()->GetList()) {
      if (!precolored->Contain(node))
        initial->Append(node);
    }
    fprintf(file, "initial register size: %ld\n", initial->GetList().size());

    for (auto node : live_graph_.interf_graph->Nodes()->GetList()) {
      adjList[node] = new live::INodeList();
      degree[node] = 0;
    }

    // Build
    fprintf(file, "-------====Build=====-----\n");
    Build();

    // make worklist
    fprintf(file, "-------====MakeWorklist=====-----\n");
    MakeWorklist();

    bool terminate = false;
    while (!terminate) {
      terminate = true;
      if (!simplifyWorklist->GetList().empty()) {
        fprintf(file, "-------====Simplify=====-----\n");
        Simplify();
        terminate = false;
      } else if (!worklistMoves->GetList().empty()) {
        fprintf(file, "-------====Coalesce=====-----\n");
        Coalesce();
        terminate = false;
      } else if (!freezeWorklist->GetList().empty()) {
        fprintf(file, "-------====Freeze=====-----\n");
        Freeze();
        terminate = false;
      } else if (!spillWorklist->GetList().empty()) {
        fprintf(file, "-------====SelectSpill=====-----\n");
        SelectSpill();
        terminate = false;
      }
    }

    fprintf(file, "-------====AssignColor=====-----\n");
    AssginColor();
    if (!spilledNodes->GetList().empty()) {
      fprintf(file, "-------====Rewrite program=====-----\n");
      fprintf(file, "spilledNodes size: %ld\n", spilledNodes->GetList().size());
      RewriteProgram(spilledNodes);
      init();
    } else {
      // fill in result
      // construct color map
      auto colorMap = temp::Map::Empty();
      temp::Map *nameMap = reg_manager->temp_map_;
      for (auto &tmp : color) {
        live::INode *t = tmp.first;
        temp::Temp *c = tmp.second;
        fprintf(file, "%d\t\t%s\n", t->NodeInfo()->Int(),
                nameMap->Look(c)->data());
        colorMap->Enter(t->NodeInfo(), nameMap->Look(c));
      }
      colorMap->Enter(reg_manager->StackPointer(),
                      nameMap->Look(reg_manager->StackPointer()));
      fclose(file);

      // eliminate the coalesced move
      assem::InstrList *remove_instr = new assem::InstrList();
      for (auto instr : il_.get()->GetInstrList()->GetList()) {
        if (isTempMove(instr)) {
          if (!instr->Def()->GetList().empty() &&
              !instr->Use()->GetList().empty() &&
              colorMap->Look(instr->Def()->GetList().front()) ==
                  colorMap->Look(instr->Use()->GetList().front())) {
            remove_instr->Append(instr);
          }
        }
      }
      auto instrList = il_.get()->GetInstrList()->GetListPtr();
      for (auto item : remove_instr->GetList()) {
        instrList->remove(item);
      }
      result_ =
          std::make_unique<ra::Result>(colorMap, il_.get()->GetInstrList());
      break;
    }
  }
}

void RegAllocator::Build() {
  worklistMoves = live_graph_.moves;
  // printf("worklistmoves size: %ld\n", worklistMoves->GetList().size());
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
      // 不会添加重复的边（虽然Adj中是有重复的，后继与前驱是重复的）
      AddEdge(node, adj);
    }
  }
  // assign precolored node
  color.clear();
  for (auto node : precolored->GetList()) {
    color[node] = node->NodeInfo();
  }
}

void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v) {
  if (u != v && (adjSet.find(std::make_pair(u, v)) == adjSet.end())) {
    adjSet.insert(std::make_pair(u, v));
    adjSet.insert(std::make_pair(v, u));
    if (!precolored->Contain(u)) {
      if (!adjList[u]->Contain(v)) {
        degree[u]++;
        adjList[u]->Append(v);
      }
    }
    if (!precolored->Contain(v)) {
      if (!adjList[v]->Contain(u)) {
        adjList[v]->Append(u);
        degree[v]++;
      }
    }
  }
}

void RegAllocator::MakeWorklist() {
  for (live::INodePtr node : initial->GetList()) {
    if (degree[node] >= K){
      spillWorklist->Append(node);
      fprintf(file,"degree of %d: %d\n",node->NodeInfo()->Int(),degree[node]);
    }
    else if (MoveRelated(node))
      freezeWorklist->Append(node);
    else
      simplifyWorklist->Append(node);
  }
  fprintf(file, "simplifyWorklist: ");
  simplifyWorklist->Print(file);
  fprintf(file, "freezeWorklist: ");
  freezeWorklist->Print(file);
  fprintf(file, "spillWorklist: ");
  spillWorklist->Print(file);
  initial->Clear();
}

void RegAllocator::Simplify() {

  auto node = simplifyWorklist->GetList().front();
  selectStack->Prepend(node);
  fprintf(file, "push %d to selectStack\n", node->NodeInfo()->Int());
  fprintf(file, "minus adj degree, number: %ld\n",
          Adjacent(node)->GetList().size());
  for (auto adj : Adjacent(node)->GetList()) {
    Decrement(adj);
  }
  simplifyWorklist->DeleteNode(node);
}

live::INodeListPtr RegAllocator::Adjacent(live::INodePtr n) {
  return adjList[n]->Diff(selectStack->Union(coalescedNodes));
}
live::MoveList *RegAllocator::NodeMoves(live::INodePtr n) {
  if (moveList.find(n) == moveList.end())
    return new live::MoveList();
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
      fprintf(file, "move %d from spill to freeze\n", node->NodeInfo()->Int());
      freezeWorklist->Append(node);
    } else {
      fprintf(file, "remove %d from spill to simplify\n", node->NodeInfo()->Int());
      simplifyWorklist->Append(node);
    }
  }
}

void RegAllocator::EnableMoves(live::INodeListPtr list) {
  for (auto node : list->GetList()) {
    for (auto move : NodeMoves(node)->GetList()) {
      if (activeMoves->Contain(move.first, move.second)) {
        activeMoves->Delete(move.first, move.second);
        // 下次再尝试合并u与v的move邻居
        worklistMoves->Append(move.first, move.second);
      }
    }
  }
}

void RegAllocator::Coalesce() {
  auto move = worklistMoves->GetList().front();
  live::INodePtr x = GetAlias(move.first);
  live::INodePtr y = GetAlias(move.second);
  live::INodePtr u = x, v = y;
  if (precolored->Contain(y) || degree[x]<degree[y]) {
    u = y;
    v = x;
  }
  worklistMoves->Delete(move.first, move.second);
  // 已合并成功
  if (u == v) {
    coalescedMoves->Append(move.first, move.second);
    addWorkList(u);
    // 两个节点都是预着色的或者已经有边连接（constrained）
  } else if (precolored->Contain(v) ||
             adjSet.find(std::make_pair(u, v)) != adjSet.end()) {
    fprintf(file, "put %d--%d into constrainedMoves\n",
            move.first->NodeInfo()->Int(), move.second->NodeInfo()->Int());
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
    // 可合并的情况
    if (precolored->Contain(u) && ok ||
        !precolored->Contain(u) &&
            Conservertive(Adjacent(v)->Union(Adjacent(u)))) {
      fprintf(file, "coalesce %d--%d\n", move.first->NodeInfo()->Int(),
              move.second->NodeInfo()->Int());
      coalescedMoves->Append(move.first, move.second);
      Combine(u, v);
      addWorkList(u);
    } else {
      // 未通过 Gerge或 Briggs的情况
      fprintf(file, "put %d--%d into activedMoves\n",
              move.first->NodeInfo()->Int(), move.second->NodeInfo()->Int());
      activeMoves->Append(move.first, move.second);
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
    fprintf(file, "move %d from freezeWorklist to simplify\n",
            u->NodeInfo()->Int());
    freezeWorklist->DeleteNode(u);
    simplifyWorklist->Append(u);
  }
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
  if (freezeWorklist->Contain(v)) {
    freezeWorklist->DeleteNode(v);
  } else {
    fprintf(file, "remove %d from spill to coalescedNodes\n", v->NodeInfo()->Int());
    spillWorklist->DeleteNode(v);
  }
  coalescedNodes->Append(v);
  alias[v] = u;
  moveList[u] = moveList[u]->Union(moveList[v]);
  auto toBeEnable = new live::INodeList();
  toBeEnable->Append(v);
  EnableMoves(toBeEnable);
  for (auto t : Adjacent(v)->GetList()) {
    AddEdge(t, u);
    Decrement(t);
  }
  fprintf(file, "degree of %d: %d\n", u->NodeInfo()->Int(),degree[u]);
  if (degree[u] >= K && freezeWorklist->Contain(u)) {
    fprintf(file, "move %d from freezeWorklist to spill\n",
            u->NodeInfo()->Int());
    freezeWorklist->DeleteNode(u);
    spillWorklist->Append(u);
  }
}

void RegAllocator::Freeze() {
  auto u = freezeWorklist->GetList().front();
  freezeWorklist->DeleteNode(u);
  fprintf(file, "move %d from freezeWorklist to simplify\n",
          u->NodeInfo()->Int());
  simplifyWorklist->Append(u);
  FreezeMoves(u);
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
      fprintf(file, "move %d from freezeWorklist to simplify\n",
              v->NodeInfo()->Int());
      freezeWorklist->DeleteNode(v);
      simplifyWorklist->Append(v);
    }
  }
}
void RegAllocator::SelectSpill() {
  auto m = HeuristicSelect();
  fprintf(file, "move %d from spillWorklist to simplify\n",
          m->NodeInfo()->Int());
  spillWorklist->DeleteNode(m);
  simplifyWorklist->Append(m);
  FreezeMoves(m);
}

/* Implement furthest-next-use algorithm to heuristically find a node to spill
 */
live::INodePtr RegAllocator::HeuristicSelect() {
  live::INode *res;
  int max_distance = 0;
  assem::InstrList *instr_list = il_->GetInstrList();

  for (live::INode *n : spillWorklist->GetList()) {
    int pos = 0;
    int start;
    int distance = -1;
    for (assem::Instr *instr : instr_list->GetList()) {
      if (instr->Def()->Contain(n->NodeInfo())) {
        start = pos;
      }
      if (instr->Use()->Contain(n->NodeInfo())) {
        distance = pos - start;
        if (distance > max_distance) {
          max_distance = distance;
          res = n;
        }
      }
      pos++;
    }

    // Defined but never used
    if (distance == -1)
      return n;
  }

  return res;
}

void RegAllocator::AssginColor() {
  fflush(file);
  while (!selectStack->GetList().empty()) {
    live::INodePtr n = selectStack->GetList().front();
    selectStack->DeleteNode(n);

    std::set<temp::Temp *> okColors;
    for (auto reg : reg_manager->intialInterfere()->GetList()) {
      okColors.emplace(reg);
    }
    for (auto w : adjList[n]->GetList()) {
      live::INodePtr alias = GetAlias(w);
      if (coloredNodes->Contain(alias) || precolored->Contain(alias)) {
        okColors.erase(color[alias]);
      }
    }
    if (okColors.empty()) {
      spilledNodes->Append(n);
      fprintf(file, "select spilled node %d\n", n->NodeInfo()->Int());
    } else {
      coloredNodes->Append(n);
      color[n] = *(okColors.begin());
    }
  }

  if (spilledNodes->GetList().empty()) {
    for (auto n : coalescedNodes->GetList()) {
      live::INodePtr ali = GetAlias(n);
      assert(color[ali]);
      color[n] = color[ali];
    }
  }
}

void RegAllocator::RewriteProgram(live::INodeListPtr spilledList) {
  std::list<assem::Instr *> *instr_list = il_->GetInstrList()->GetListPtr();
  // it seems that there's no need to refresh initial
  // live::INodeListPtr newTemps;
  for (auto node : spilledList->GetList()) {
    bool create = false;
    temp::Temp *newTemp;
    int offset = 0;
    for (auto instr = instr_list->begin(); instr != instr_list->end();
         instr++) {
      // def
      if ((*instr)->Def()->Contain(node->NodeInfo())) {
        if (!create) {
          newTemp = temp::TempFactory::NewTemp();
          offset = frame_->ExpandFrame(1);
          // for GC: if a pilled temp is a pointer, the access in frame should be set as a pointer
          if(node->NodeInfo()->isPointer()){
            newTemp->setPointer();
          }
          create = true;
          fprintf(file, "replace %d with %d, offset: %d\n",
                  node->NodeInfo()->Int(), newTemp->Int(), offset);
        }
        (*instr)->Def()->Replace(node->NodeInfo(), newTemp);
        std::string assem = "movq `s0, (" + frame_->GetLabel() + "_framesize-" +
                            std::to_string(offset) + ")(`d0)";
        instr++;
        instr_list->insert(
            instr, new assem::OperInstr(
                       assem, new temp::TempList({reg_manager->StackPointer()}),
                       new temp::TempList({newTemp}), nullptr));
        instr--;
      }
      // use
      if ((*instr)->Use()->Contain(node->NodeInfo())) {
        if (!create) {
          newTemp = temp::TempFactory::NewTemp();
          offset = frame_->ExpandFrame(1);
          // for GC: if a pilled temp is a pointer, the access in frame should be set as a pointer
          if(node->NodeInfo()->isPointer()){
            newTemp->setPointer();
          }
          create = true;
          fprintf(file, "replace %d with %d, offset: %d\n",
                  node->NodeInfo()->Int(), newTemp->Int(), offset);
        }
        (*instr)->Use()->Replace(node->NodeInfo(), newTemp);
        std::string assem = "movq (" + frame_->GetLabel() + "_framesize-" +
                            std::to_string(offset) + ")(`s0), `d0";
        instr_list->insert(
            instr,
            new assem::OperInstr(
                assem, new temp::TempList({newTemp}),
                new temp::TempList({reg_manager->StackPointer()}), nullptr));
      }
    }
  }
}

} // namespace ra