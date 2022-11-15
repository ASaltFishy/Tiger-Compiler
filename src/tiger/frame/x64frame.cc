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
    tree::BinopExp *address = new tree::BinopExp(tree::BinOp::PLUS_OP,framePtr,new tree::ConstExp(offset));
    return new tree::MemExp(address);
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
  std::list<Access *> *formals_;

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
    //offset of static link equals to 8
    count_++;
    ret = new InFrameAccess(count_ * reg_manager->WordSize());
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

} // namespace frame
