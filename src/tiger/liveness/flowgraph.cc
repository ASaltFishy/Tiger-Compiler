#include "tiger/liveness/flowgraph.h"

namespace fg {

// construct control flow graph
void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  // one pass
  for (assem::Instr *instruction : instr_list_->GetList()) {
    FNode *prev = nullptr;
    if (instr_list_->GetList().size() != 0) {
      prev = flowgraph_->Nodes()->GetList().back();
    }
    FNode *newNode = flowgraph_->NewNode(instruction);
    if (prev && !isDirectJump(prev)) {
      flowgraph_->AddEdge(prev, newNode);
    }
    if (isLabel(newNode)) {
      assem::LabelInstr *instr = (assem::LabelInstr *)newNode->NodeInfo();
      label_map_->Enter(instr->label_, newNode);
    }
  }

  // two pass for jump condition
  for (auto node : flowgraph_->Nodes()->GetList()) {
    if (!isOper(node))
      continue;
    assem::OperInstr *opInstr = (assem::OperInstr *)(node->NodeInfo());
    if (opInstr->jumps_) {
      for (auto dst : *opInstr->jumps_->labels_) {
        if (dst == nullptr)
          break;
        FNode *jumpTo = label_map_->Look(dst);
        flowgraph_->AddEdge(node, jumpTo);
      }
    }
  }
}

bool isDirectJump(FNode *node) {
  assem::Instr *instr = node->NodeInfo();
  if (typeid(*instr) == typeid(assem::OperInstr)) {
    assem::OperInstr *opInstr = (assem::OperInstr *)instr;
    if (opInstr->assem_.find("jmp") != std::string::npos)
      return true;
  }
  return false;
}

bool isLabel(FNode *node) {
  assem::Instr *instr = node->NodeInfo();
  return typeid(*instr) == typeid(assem::LabelInstr);
}

bool isMove(FNode *node) {
  assem::Instr *instr = node->NodeInfo();
  return typeid(*instr) == typeid(assem::MoveInstr);
}

bool isOper(FNode *node) {
  assem::Instr *instr = node->NodeInfo();
  return typeid(*instr) == typeid(assem::OperInstr);
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if (dst_)
    return dst_;
  else
    return new temp::TempList();
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if (dst_)
    return dst_;
  else
    return new temp::TempList();
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if (src_)
    return src_;
  else
    return new temp::TempList();
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if (src_)
    return src_;
  else
    return new temp::TempList();
}
} // namespace assem
