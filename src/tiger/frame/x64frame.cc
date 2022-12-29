#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;
  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) override;
  int getOffset() override;
};
tree::Exp *InFrameAccess::ToExp(tree::Exp *framePtr) {
  return new tree::MemExp(
      new tree::BinopExp(tree::MINUS_OP, framePtr, new tree::ConstExp(offset)));
}
int InFrameAccess::getOffset(){
  return offset;
}

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) override;
  int getOffset() override;
};
tree::Exp *InRegAccess::ToExp(tree::Exp *framePtr) {
  return new tree::TempExp(reg);
}
int InRegAccess::getOffset(){
  return -1;
}

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
private:
  int count_ = 0;

public:
  X64Frame(temp::Label *name, std::list<bool> formals);
  Access *AllocLocal(bool escape) override;
  Access *getSLAccess() override;
  int getFrameSize() override;
  int ExpandFrame(int addWord) override;
};

/* TODO: Put your lab5 code here */

X64Frame::X64Frame(temp::Label *name, std::list<bool> formals) : Frame(name) {
  for (bool escape_ : formals) {
    AllocLocal(escape_);
  }
}

int X64Frame::ExpandFrame(int addWord){
  count_+=addWord;
  return count_ * reg_manager->WordSize();
}

Access *X64Frame::AllocLocal(bool escape) {
  Access *ret = NULL;
  if (escape) {
    // offset of static link equals to 8 (relative to frame pointer)
    count_++;
    ret = new InFrameAccess(count_ * reg_manager->WordSize());
  } else {
    ret = new InRegAccess(temp::TempFactory::NewTemp());
  }
  formals_.push_back(ret);
  // printf("access offset: %d\n",ret->getOffset());
  return ret;
}

int X64Frame::getFrameSize() { return count_ * reg_manager->WordSize(); }

Access *X64Frame::getSLAccess() { return formals_.front(); }

Frame *Frame::newFrame(temp::Label *name, std::list<bool> formals) {
  formals.push_front(true);
  return new X64Frame(name, formals);
}

// add section 4, 5 <body> 8
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *body) {
  int argNum = frame->formals_.size();// include SL
  std::list<temp::Temp *> regList = reg_manager->ArgRegs()->GetList();
  regList.reverse();
  auto reg = regList.begin();
  if (argNum < 6) {
    int temp = 6 - argNum;
    for (temp; temp > 0; temp--) {
      reg++;
    }
  }
  // what happend to this iterator in reverse?????
  frame->formals_.reverse();
  for (frame::Access *it : frame->formals_) {
    if (argNum <= 6) {
      // printf("access offset: %d\n",it->getOffset());
      body = new tree::SeqStm(
          new tree::MoveStm(
              it->ToExp(new tree::TempExp(reg_manager->FramePointer())),new tree::TempExp(*reg)),
          body);
      reg++;
    } else {
      // take retaddr into account
      tree::Exp *dst = new tree::MemExp(new tree::BinopExp(
          tree::BinOp::PLUS_OP, new tree::TempExp(reg_manager->FramePointer()),
          new tree::ConstExp(reg_manager->WordSize() * (argNum - 6))));
      body = new tree::SeqStm(
          new tree::MoveStm(it->ToExp(new tree::TempExp(
                                     reg_manager->FramePointer())), dst),
          body);
    }
    argNum--;
  }
  regList.reverse();
  frame->formals_.reverse();
  return body;
}
assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(
      new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  return body;
}

// 提前调整rsp，否则找frame pointer会有问题
assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  static char instr[256];

  std::string prolog;
  auto X64frame = (frame::X64Frame *)frame;
  int size = X64frame->getFrameSize();
  sprintf(instr, ".set %s_framesize, %d\n", frame->GetLabel().data(), size);
  prolog = std::string(instr);
  sprintf(instr, "%s:\n", frame->GetLabel().data());
  prolog.append(std::string(instr));
  sprintf(instr, "subq $%d, %%rsp\n", size);
  prolog.append(std::string(instr));

  sprintf(instr, "addq $%d, %%rsp\n", size);
  std::string epilog = std::string(instr);
  epilog.append(std::string("retq\n"));
  // std::string epilog = std::string("retq\n");
  return new assem::Proc(prolog, body, epilog);
}

tree::Exp *ExternalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}

} // namespace frame
