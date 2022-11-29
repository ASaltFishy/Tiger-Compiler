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
};
tree::Exp *InFrameAccess::ToExp(tree::Exp *framePtr){
    return new tree::MemExp(new tree::BinopExp(tree::MINUS_OP,framePtr,new tree::ConstExp(offset)));
  }



class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) override;
};
tree::Exp *InRegAccess::ToExp(tree::Exp *framePtr){
    return new tree::TempExp(reg);
  }



class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
private:
  int count_ = 0;

public:
  X64Frame(temp::Label *name, std::list<bool> formals);
  Access *AllocLocal(bool escape) override;
  Access *getSLAccess()override;
  int getFrameSize() override;
};

/* TODO: Put your lab5 code here */

X64Frame::X64Frame(temp::Label *name, std::list<bool> formals) : Frame(name) {
    for (bool escape_ : formals) {
      AllocLocal(escape_);
    }
  }

Access *X64Frame::AllocLocal(bool escape) {
  Access *ret = NULL;
  if (escape) {
    //offset of static link equals to 0 (relative to frame pointer)
    ret = new InFrameAccess(count_ * reg_manager->WordSize());
    count_++;
  } else {
    ret = new InRegAccess(temp::TempFactory::NewTemp());
  }
  formals_->push_back(ret);
  return ret;
}

int X64Frame::getFrameSize(){
  return count_*reg_manager->WordSize();
}

Access *X64Frame::getSLAccess(){
  return formals_->front();
}

Frame *Frame::newFrame(temp::Label *name, std::list<bool> formals) {
  return new X64Frame(name, formals);
}

// add section 4, 5 <body> 8
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *body) {
  int argNum = frame->formals_->size();
  std::list<temp::Temp *> regList = reg_manager->ArgRegs()->GetList();
  auto reg = regList.end();
  for (auto it = frame->formals_->begin(); it != frame->formals_->end(); it++) {
    if (argNum <= 6) {
      reg--;
      body = new tree::SeqStm(
          new tree::MoveStm(
              new tree::TempExp(*reg),
              (*it)->ToExp(new tree::TempExp(reg_manager->FramePointer()))),
          body);
    } else {
      tree::Exp *dst = new tree::MemExp(new tree::BinopExp(
          tree::BinOp::MINUS_OP, new tree::TempExp(reg_manager->FramePointer()),
                                                  new tree::ConstExp(reg_manager->WordSize()*(argNum-6))));
      body = new tree::SeqStm(
          new tree::MoveStm(dst, (*it)->ToExp(new tree::TempExp(
                                     reg_manager->FramePointer()))),
          body);
    }
    argNum--;
  }
}
assem::InstrList *ProcEntryExit2(assem::InstrList *body) {}

assem::Proc *ProcEntryExit3(frame::Frame *frame,
                                   assem::InstrList *body) {}

tree::Exp *ExternalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}


} // namespace frame
