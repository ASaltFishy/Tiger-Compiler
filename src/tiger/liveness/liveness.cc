#include "tiger/liveness/liveness.h"
#include <set>

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

// file the in set and out set of every instruction
void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  for (fg::FNode *node : flowgraph_->Nodes()->GetList()) {
    in_->Enter(node, new temp::TempList());
    out_->Enter(node, new temp::TempList());
  }

  std::list<fg::FNode *> reList = flowgraph_->Nodes()->GetList();
  reList.reverse();
  int nodeNum = flowgraph_->nodecount_;
  while (true) {
    int unchangedNum = 0;
    for (fg::FNode *node : reList) {
      assem::Instr *instr = node->NodeInfo();
      temp::TempList *old_out = out_->Look(node);
      temp::TempList *old_in = in_->Look(node);
      // compute out set
      for (fg::FNode *succ : node->Succ()->GetList()) {
        out_->Set(node, Union(out_->Look(node), out_->Look(succ)));
      }
      // compute in set
      temp::TempList *out = out_->Look(node);
      in_->Set(node, Union(instr->Use(), Except(out, instr->Def())));

      // compute terminate (after all of nodes unchanging)
      if (Equal(old_out, out_->Look(node)) && Equal(old_in, in_->Look(node))) {
        unchangedNum++;
      }
    }
    if (unchangedNum == nodeNum)
      break;
  }
  reList.reverse();
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  IGraph *g = live_graph_.interf_graph;
  MoveList *move_list = live_graph_.moves;

  // step1 handle all the initially live register
  for (temp::Temp *reg : reg_manager->intialInterfere()->GetList()) {
    INode *node = g->NewNode(reg);
    temp_node_map_->Enter(reg, node);
  }
  for (INode *from : g->Nodes()->GetList()) {
    for (INode *to : g->Nodes()->GetList()) {
      if (from != to) {
        g->AddEdge(from, to);
      }
    }
  }

  // step2 add temporary register into interf_graph
  std::set<temp::Temp *> all_reg;
  for (fg::FNode *node : flowgraph_->Nodes()->GetList()) {
    for (temp::Temp *it : node->NodeInfo()->Def()->GetList()) {
      if (it == reg_manager->StackPointer())
        continue;
      all_reg.insert(it);
    }
    for (temp::Temp *it : node->NodeInfo()->Use()->GetList()) {
      if (it == reg_manager->StackPointer())
        continue;
      all_reg.insert(it);
    }
  }
  for (auto &it : all_reg) {
    temp_node_map_->Enter(it, g->NewNode(it));
  }

  // step3 add edge appearing in out[n]
  for (fg::FNode *node : flowgraph_->Nodes()->GetList()) {
    temp::TempList *out = out_->Look(node);
    // not move instruction
    if (!fg::isMove(node)) {
      for (temp::Temp *def : node->NodeInfo()->Def()->GetList()) {
        for (temp::Temp *out : out->GetList()) {
          if (def != reg_manager->StackPointer() && out != reg_manager->StackPointer()) {
            g->AddEdge(temp_node_map_->Look(def), temp_node_map_->Look(out));
            g->AddEdge(temp_node_map_->Look(out), temp_node_map_->Look(def));
          }
        }
      }
    } 
    // move instruction
    else {
      for (temp::Temp *def : node->NodeInfo()->Def()->GetList()) {
        for (temp::Temp *out : Except(out,node->NodeInfo()->Use())->GetList()) {
          if (def != reg_manager->StackPointer() && out != reg_manager->StackPointer()) {
            g->AddEdge(temp_node_map_->Look(def), temp_node_map_->Look(out));
            g->AddEdge(temp_node_map_->Look(out), temp_node_map_->Look(def));
          }
        }
        for(temp::Temp *use : node->NodeInfo()->Use()->GetList()){
          if (def != reg_manager->StackPointer() && use != reg_manager->StackPointer()){
            if(!move_list->Contain(temp_node_map_->Look(use), temp_node_map_->Look(def))){
              move_list->Append(temp_node_map_->Look(use), temp_node_map_->Look(def));
            }
          } 
        }
      }
    }
  }
}

  void LiveGraphFactory::Liveness() {
    LiveMap();
    InterfGraph();
  }

  // some tool function
  temp::TempList *Union(temp::TempList * a, temp::TempList * b) {
    auto res = new temp::TempList();
    std::set<temp::Temp *> temp_set;
    for (temp::Temp *it : a->GetList()) {
      temp_set.insert(it);
    }
    for (temp::Temp *it : b->GetList()) {
      temp_set.insert(it);
    }
    for (auto set_it = temp_set.begin(); set_it != temp_set.end(); set_it++) {
      res->Append(*set_it);
    }
    return res;
  }

  temp::TempList *Except(temp::TempList * origin, temp::TempList * except) {
    auto res = new temp::TempList();
    for (auto reg : origin->GetList()) {
      if (!Contain(except, reg))
        res->Append(reg);
    }
    return res;
  }

  bool Contain(temp::TempList * origin, temp::Temp * reg) {
    for (auto it : origin->GetList()) {
      if (it == reg)
        return true;
    }
    return false;
  }

  bool Equal(temp::TempList * old_list, temp::TempList * new_list) {
    if (old_list->GetList().size() != new_list->GetList().size())
      return false;
    for (auto it : old_list->GetList()) {
      if (!Contain(new_list, it))
        return false;
    }
    return true;
  }

} // namespace live
