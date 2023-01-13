#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  [[nodiscard]] virtual temp::TempList *intialInterfere() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;

protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  virtual tree::Exp *ToExp(tree::Exp *framePtr) = 0;
  virtual ~Access() = default;

  // for debug
  virtual int getOffset() = 0;
  virtual void setPointer() = 0;
  virtual bool isPointer() = 0;
  virtual void discardPointer() = 0;
};

class Frame {
  /* TODO: Put your lab5 code here */
private:
  temp::Label *name_;

public:
  std::list<Access *> formals_;

  Frame(temp::Label *name) : name_(name) {}
  std::string GetLabel() { return name_->Name(); }
  temp::Label *getLabel(){return name_;}
  virtual int getFrameSize() = 0;
  virtual Access *getSLAccess() = 0;
  virtual Access *AllocLocal(bool escape) = 0;
  virtual int ExpandFrame(int addWord) = 0;
  virtual std::list<long> getPointerList() = 0;
  [[nodiscard]] static Frame *newFrame(temp::Label *name,
                                       std::list<bool> formals);
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase { Proc, String, PtrMap };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase,
                           bool need_ra) const = 0;
  // virtual Frag *getPtrMap(temp::Label *function) = 0;
};

class PtrMapFrag : public Frag {
public:
  // temp::Label *frame_;
  temp::Label *index_;
  // temp::Label *prev_;
  std::string frame_size_;
  long isMain;
  std::list<long> pointers_;

  PtrMapFrag(temp::Label *label)
      :index_(label) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
  // Frag *getPtrMap(temp::Label *function) override{
  //   if(frame_== function){
  //     return this;
  //   }else{
  //     return nullptr;
  //   }
  // }
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
  // Frag *getPtrMap(temp::Label *function) override{ return nullptr;}
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
  // Frag *getPtrMap(temp::Label *function) override{ return nullptr;}
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag *> &GetList() { return frags_; }
  // Frag *getPtrMap(temp::Label *function){
  //   for(Frag *f : frags_){
  //     Frag *temp = f->getPtrMap(function);
  //     if(temp!=nullptr){
  //       return temp;
  //     }
  //   }
  //   return nullptr;
  // }

private:
  std::list<Frag *> frags_;
};

/* TODO: Put your lab5 code here */
// for view shift, called by FunDec.translation()
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *body);
assem::InstrList *ProcEntryExit2(assem::InstrList *body);
assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body);
tree::Exp *ExternalCall(std::string s, tree::ExpList *args);

} // namespace frame

#endif