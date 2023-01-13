#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include "tiger/liveness/liveness.h"
#include <iostream>
#include <map>

extern frame::RegManager *reg_manager;

namespace gc {

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

std::vector<int> Union(std::vector<int> a, std::vector<int> b);
std::vector<int> Except(std::vector<int> a, std::vector<int> b);
bool Contain(std::vector<int> &a, int value);
bool Equal(std::vector<int> &a, std::vector<int> &b);

/***************** class Roots ********************/

class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
public:
  Roots(assem::InstrList *il, frame::Frame *frame, fg::FGraphPtr flowgraph,
        temp::Map *color, frame::Frags *frags)
      : il_(il), frame_(frame), flowgraph_(flowgraph), color_(color),
        frags_(frags) {}
  void constructPtrmap();

private:
  assem::InstrList *il_;
  frame::Frame *frame_;
  fg::FGraphPtr flowgraph_;
  temp::Map *color_;
  frame::Frags *frags_;

  std::map<fg::FNodePtr, temp::TempList *> temp_in_;
  std::map<fg::FNodePtr, std::vector<int>> address_in_;
  std::map<fg::FNodePtr, std::vector<int>> address_out_;

  void getLivePointer();
  void generatePtrmapFrag();
  std::vector<int> UseAddress(assem::Instr *instr);
  std::vector<int> DefAddress(assem::Instr *instr);
};

/***************** End of Class Roots ********************/

void Roots::constructPtrmap() {
  getLivePointer();
  generatePtrmapFrag();
}

/* 在frags队列中加入PtrmapFrag，在output阶段会打印输出至汇编文件 */
void Roots::generatePtrmapFrag() {
  for (fg::FNode *node : flowgraph_->Nodes()->GetList()) {
    assem::Instr *instr = node->NodeInfo();
    if (typeid(*instr) == typeid(assem::OperInstr)) {
      std::string assem = ((assem::OperInstr *)instr)->assem_;
      if (assem.find("call") != assem.npos) {
        // isnert retlabel after call instruction
        temp::Label *retLabel = temp::LabelFactory::NewLabel();
        il_->Insert(++il_->Find(instr),
                    new assem::LabelInstr(retLabel->Name(), retLabel));
        // construct a PtrmapFrag
        frame::PtrMapFrag *newFrag = new frame::PtrMapFrag(retLabel);
        newFrag->frame_size_ = frame_->GetLabel() + "_framesize";
        if (frame_->GetLabel() == "tigermain")
          newFrag->isMain = 1;
        else
          newFrag->isMain = 0;
        // put all the active frame access into ptrmap
        std::vector<int> pointerInFrame = address_in_[node];
        for (int offset : pointerInFrame) {
          newFrag->pointers_.push_back(offset);
        }
        // put calleeSaved reg that is a pointer into the "bitmap"
        temp::TempList *tempList = temp_in_[node];
        std::list<temp::Temp *> calleeSaved =
            reg_manager->CalleeSaves()->GetList();
        temp::Map *temp_map_ = reg_manager->temp_map_;
        std::string regString = "000000";
        for (temp::Temp *reg : tempList->GetList()) {
          if (reg->isPointer()) {
            std::string *pointerTemp = color_->Look(reg);
            int index = 0;
            for (temp::Temp *calleeReg : calleeSaved) {
              if ((*temp_map_->Look(calleeReg)) == *pointerTemp) {
                regString[index] = '1';
                break;
              }
              index++;
            }
          }
        }
        newFrag->pointers_.push_front(atol(regString.data()));
        frags_->PushBack(newFrag);
      }
    }
  }
}

/* 填充temp_in_等三个数据结构（类似liveness） */
void Roots::getLivePointer() {
  // temp_in_
  live::LiveGraphFactory *liveGraphFacPtr =
      new live::LiveGraphFactory(flowgraph_);
  liveGraphFacPtr->LiveMap();
  graph::Table<assem::Instr, temp::TempList> *in_ = liveGraphFacPtr->GetIn();

  // address_in_ and address_out_
  for (fg::FNode *node : flowgraph_->Nodes()->GetList()) {
    address_in_[node] = std::vector<int>();
    address_out_[node] = std::vector<int>();
    temp_in_[node] = in_->Look(node);
  }
  std::list<fg::FNode *> reList = flowgraph_->Nodes()->GetList();
  reList.reverse();
  int nodeNum = flowgraph_->nodecount_;
  while (true) {
    int unchangedNum = 0;
    for (fg::FNode *node : reList) {
      assem::Instr *instr = node->NodeInfo();
      std::vector<int> old_out = address_out_[node];
      std::vector<int> old_in = address_in_[node];
      // compute out set
      for (fg::FNode *succ : node->Succ()->GetList()) {
        address_out_[node] = Union(address_out_[node], address_in_[succ]);
      }
      // compute in set
      std::vector<int> out = address_out_[node];
      address_in_[node] =
          Union(UseAddress(instr), Except(out, DefAddress(instr)));

      // compute terminate (after all of nodes unchanging)
      if (Equal(old_out, address_out_[node]) &&
          Equal(old_in, address_in_[node])) {
        unchangedNum++;
      }
    }
    if (unchangedNum == nodeNum)
      break;
  }
  reList.reverse();
}

std::vector<int> Roots::DefAddress(assem::Instr *instr) {
  if (typeid(*instr) == typeid(assem::OperInstr)) {
    std::string ass = ((assem::OperInstr *)instr)->assem_;
    if (ass.find("movq") != ass.npos && ass.find("_framesize") != ass.npos) {
      // 逗号后面的内存应用才是def
      int dstStart = ass.find(',');
      int signal = ass.find("_framesize");
      if (signal > dstStart) {
        std::string dst = ass.substr(signal);
        bool negative = (dst.find('-') != dst.npos);
        int OffsetStart;
        if (negative)
          OffsetStart = dst.find('-') + 1;
        else
          OffsetStart = dst.find('+') + 1;
        int OffsetEnd = dst.find(')');
        std::string offset = dst.substr(OffsetStart, OffsetEnd - OffsetStart);
        if (negative)
          return std::vector<int>(1, 0 - std::stoi(offset));
        else
          return std::vector<int>(1, std::stoi(offset));
      }
    }
  }
  return std::vector<int>();
}

std::vector<int> Roots::UseAddress(assem::Instr *instr) {
  if (typeid(*instr) == typeid(assem::OperInstr)) {
    std::string ass = ((assem::OperInstr *)instr)->assem_;
    if (ass.find("movq") != ass.npos && ass.find("_framesize") != ass.npos) {
      // 逗号前的内存引用是use
      int srcEnd = ass.find(',');
      int signal = ass.find("_framesize");
      if (signal < srcEnd) {
        std::string src = ass.substr(signal, srcEnd - signal);
        bool negative = src.find('-') != src.npos;
        int OffsetStart;
        if (negative)
          OffsetStart = src.find('-') + 1;
        else
          OffsetStart = src.find('+') + 1;
        int OffsetEnd = src.find(')');
        std::string offset = src.substr(OffsetStart, OffsetEnd - OffsetStart);
        if (negative)
          return std::vector<int>(1, 0 - std::stoi(offset));
        else
          return std::vector<int>(1, std::stoi(offset));
      }
    }
  }
  return std::vector<int>();
}

/***************** Utility Function ********************/

std::vector<int> Union(std::vector<int> a, std::vector<int> b) {
  // 去重
  int size = b.size();
  for (int i = 0; i < size; i++) {
    if (!Contain(a, b[i]))
      a.push_back(b[i]);
  }
  return a;
}
std::vector<int> Except(std::vector<int> a, std::vector<int> b) {
  int size = a.size();
  int i = 0;
  while (i < a.size()) {
    if (Contain(b, a[i])) {
      a.erase(a.begin() + i);
    } else {
      i++;
    }
  }
  return a;
}
bool Contain(std::vector<int> &a, int value) {
  int size = a.size();
  for (int i = 0; i < size; i++) {
    if (a[i] == value)
      return true;
  }
  return false;
}
bool Equal(std::vector<int> &a, std::vector<int> &b) {
  if (a.size() != b.size())
    return false;
  int size = a.size();
  for (int i = 0; i < size; i++) {
    if (!Contain(b, a[i]))
      return false;
  }
  return true;
}

/***************** End of Utility Function ********************/

} // namespace gc

#endif // TIGER_RUNTIME_GC_ROOTS_H