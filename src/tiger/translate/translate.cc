#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/x64frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  return new Access(level, (level->frame_)->AllocLocal(escape));
}

Level *Level::NewLevel(temp::Label *name, std::list<bool> escape,
                       Level *parent) {
  // push static link into formal parameter, have to put it in newFrame
  // escape.push_front(true);
  return new Level(frame::Frame::newFrame(name, escape), parent);
}

temp::Temp *Level::getFramePointer() {
  return reg_manager->FramePointer();
  // temp::Temp *rsp = reg_manager->StackPointer();
  // int frameSize = frame_->getFrameSize();
  // tree::BinopExp *address =
  //     new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(rsp),
  //                        new tree::ConstExp(frameSize));
  // return new tree::MemExp(address);

  // just return virtual frame pointer
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    tree::CjumpStm *stm =
        new tree::CjumpStm(tree::NE_OP, exp_, new tree::ConstExp(0), t, f);
    return Cx(tr::PatchList({&t}), tr::PatchList({&f}), stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    // don't know the position
    errormsg->Error(0, "cx expect to see a value");
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    ((tree::CjumpStm *)cx_.stm_)->true_label_ = t;
    ((tree::CjumpStm *)cx_.stm_)->false_label_ = f;
    return new tree::EseqExp(
        new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
        new tree::EseqExp(
            cx_.stm_,
            new tree::EseqExp(
                new tree::LabelStm(f),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),
                                                    new tree::ConstExp(0)),
                                  new tree::EseqExp(new tree::LabelStm(t),
                                                    new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), NULL,
                         errormsg_.get());
}

} // namespace tr

namespace absyn {
/* tool fuction */
tree::MemExp *mem_gen(tree::Exp *exp, int wordOffset) {
  return new tree::MemExp(new tree::BinopExp(
      tree::PLUS_OP, exp,
      new tree::ConstExp(wordOffset * reg_manager->WordSize())));
}

tree::Exp *findBySL(tr::Level *currentLevel, tr::Level *targetLevel) {
  // find the SL of target access
  tree::Exp *static_link = new tree::TempExp(currentLevel->getFramePointer());
  tr::Level *tempLevel = currentLevel;
  while (tempLevel != targetLevel) {
    frame::Access *sl = tempLevel->frame_->getSLAccess();
    static_link = sl->ToExp(static_link);
    tempLevel = tempLevel->parent_;
  }
  return static_link;
}

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *tempEntry = venv->Look(sym_);
  env::VarEntry *entry = (env::VarEntry *)tempEntry;
  tr::Access *targetAccess = entry->access_;
  tr::Level *targetLevel = targetAccess->level_, *tempLevel = level;
  tr::Exp *retExp;

  // find the SL of target access
  tree::Exp *static_link = findBySL(level, targetLevel);
  retExp = new tr::ExExp(targetAccess->access_->ToExp(static_link));
  return new tr::ExpAndTy(retExp, entry->ty_);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *fieldAddress =
      var_->Translate(venv, tenv, level, label, errormsg);
  std::list<type::Field *> fieldList =
      ((type::RecordTy *)(fieldAddress->ty_))->fields_->GetList();
  int offset = 0;
  type::Ty *retType;
  for (type::Field *item : fieldList) {
    if (item->name_ == sym_) {
      retType = item->ty_;
      tr::Exp *retExp =
          new tr::ExExp(mem_gen(fieldAddress->exp_->UnEx(), offset));
      return new tr::ExpAndTy(retExp, retType);
    }
    offset++;
  }
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *arrayAddress =
      var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *indexAddress =
      subscript_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *retType = ((type::ArrayTy *)(arrayAddress->ty_))->ActualTy();
  tree::Exp *offsetExp =
      new tree::BinopExp(tree::MUL_OP, indexAddress->exp_->UnEx(),
                         new tree::ConstExp(reg_manager->WordSize()));
  tree::Exp *retExp =
      new tree::BinopExp(tree::PLUS_OP, arrayAddress->exp_->UnEx(), offsetExp);
  return new tr::ExpAndTy(new tr::ExExp(new tree::MemExp(retExp)), retType);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *stringLabel = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(stringLabel, str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(stringLabel)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::FunEntry *entry = (env::FunEntry *)(venv->Look(func_));
  temp::Label *funcLabel = temp::LabelFactory::NamedLabel(func_->Name());
  tree::NameExp *name = new tree::NameExp(funcLabel);
  tr::ExExp *retExp;

  tree::ExpList *argList = new tree::ExpList();
  // arglist in callExp which is generated from parsing don't contain static
  // link
  for (Exp *item : args_->GetList()) {
    tr::ExpAndTy *temp = item->Translate(venv, tenv, level, label, errormsg);
    argList->Append(temp->exp_->UnEx());
  }

  if (!entry->level_->parent_) {
    retExp = new tr::ExExp(frame::ExternalCall(func_->Name(), argList));
  } else {
    // add sl into arguement list
    // target level is parent of the function
    tree::Exp *static_link = findBySL(level, entry->level_->parent_);
    argList->Insert(static_link);
    retExp = new tr::ExExp(new tree::CallExp(name, argList));
  }

  type::Ty *retType = entry->result_;
  return new tr::ExpAndTy(retExp, retType);
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tree::BinOp Op;
  tree::Exp *tempExp;
  tree::Stm *tempStm;
  type::Ty *retType;
  tr::Exp *retExp;
  tr::ExpAndTy *left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right = right_->Translate(venv, tenv, level, label, errormsg);
  tree::Exp *leftExp = left->exp_->UnEx();
  tree::Exp *rightExp = right->exp_->UnEx();
  if (oper_ <= DIVIDE_OP && oper_ >= PLUS_OP) {
    tree::BinOp op;
    switch (oper_) {
    case PLUS_OP:
      op = tree::PLUS_OP;
      break;
    case MINUS_OP:
      op = tree::MINUS_OP;
      break;
    case TIMES_OP:
      op = tree::MUL_OP;
      break;
    case DIVIDE_OP:
      op = tree::DIV_OP;
      break;
    default:
      break;
    }
    tempExp = new tree::BinopExp(op, leftExp, rightExp);
    retExp = new tr::ExExp(tempExp);
    retType = left->ty_;
  } else if (oper_ >= LT_OP) {
    tree::RelOp op;
    switch (oper_) {
    case LT_OP:
      op = tree::LT_OP;
      break;
    case LE_OP:
      op = tree::LE_OP;
      break;
    case GT_OP:
      op = tree::GT_OP;
      break;
    case GE_OP:
      op = tree::GE_OP;
      break;
    default:
      break;
    }
    temp::Label *trueLabel = temp::LabelFactory::NewLabel();
    temp::Label *falseLabel = temp::LabelFactory::NewLabel();
    tempStm = new tree::CjumpStm(op, leftExp, rightExp, trueLabel, falseLabel);
    retType = left->ty_;
    retExp = new tr::CxExp(tr::PatchList({&trueLabel}),
                           tr::PatchList({&falseLabel}), tempStm);
  } else {
    tree::Stm *stm;
    temp::Label *trueLabel = temp::LabelFactory::NewLabel();
    temp::Label *falseLabel = temp::LabelFactory::NewLabel();
    switch (oper_) {
    case EQ_OP:
      // handle string compare
      if (left->ty_->IsSameType(type::StringTy::Instance())) {
        auto expList = new tree::ExpList({leftExp, rightExp});
        stm = new tree::CjumpStm(tree::RelOp::EQ_OP,
                                 frame::ExternalCall("string_equal", expList),
                                 new tree::ConstExp(1), trueLabel, falseLabel);
      } else {
        stm = new tree::CjumpStm(tree::RelOp::EQ_OP, leftExp, rightExp,
                                 trueLabel, falseLabel);
      }
      break;
    case NEQ_OP:
      stm = new tree::CjumpStm(tree::RelOp::NE_OP, leftExp, rightExp, trueLabel,
                               falseLabel);
      break;
    case AND_OP:{
      IfExp *changeToIf = new IfExp(pos_,left_,right_,new IntExp(pos_,0));
      return changeToIf->Translate(venv, tenv, level, label, errormsg);
      break;
    }
    case OR_OP:{
      IfExp *changeToIf = new IfExp(pos_,left_,new IntExp(pos_,1),right_);
      return changeToIf->Translate(venv, tenv, level, label, errormsg);
      break;
    }
    default:
      break;
    }
    retType = left->ty_;
    retExp = new tr::CxExp(tr::PatchList({&trueLabel}),
                           tr::PatchList({&falseLabel}), stm);
  }
  return new tr::ExpAndTy(retExp, retType);
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *recordTy = tenv->Look(typ_);
  std::list<EField *> fieldList = fields_->GetList();
  tree::TempExp *r = new tree::TempExp(temp::TempFactory::NewTemp());
  // call malloc to allocate the spacce on heap ,generate the move stm
  tree::ExpList *args = new tree::ExpList(
      {new tree::ConstExp(fieldList.size() * reg_manager->WordSize())});
  tree::Stm *setRe =
      new tree::MoveStm(r, frame::ExternalCall("alloc_record", args));

  // generate SEQ tree
  int count = 0;
  for (EField *item : fieldList) {
    tr::ExpAndTy *tempTy =
        item->exp_->Translate(venv, tenv, level, label, errormsg);
    setRe = new tree::SeqStm(
        setRe, new tree::MoveStm(mem_gen(r, count), tempTy->exp_->UnEx()));
    count++;
  }
  tree::Exp *retExp = new tree::EseqExp(setRe, r);
  return new tr::ExpAndTy(new tr::ExExp(retExp), recordTy);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> expList = seq_->GetList();
  tr::ExpAndTy *result;
  tr::Exp *exp = new tr::ExExp(new tree::ConstExp(0));
  for (absyn::Exp *item : expList) {
    result = item->Translate(venv, tenv, level, label, errormsg);
    exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), result->exp_->UnEx()));
  }
  // return type of last exp
  return new tr::ExpAndTy(exp, result->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label, errormsg);
  tree::MoveStm *retExp =
      new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx());
  return new tr::ExpAndTy(new tr::NxExp(retExp), var->ty_);
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);

  tr::Cx test_cx = test->exp_->UnCx(errormsg);
  tree::CjumpStm *cjump = (tree::CjumpStm *)(test_cx.stm_);
  tree::LabelStm *trueLabelStm = new tree::LabelStm(cjump->true_label_);
  tree::LabelStm *falseLabelStm = new tree::LabelStm(cjump->false_label_);

  tr::ExpAndTy *then = then_->Translate(venv, tenv, level, label, errormsg);

  if (elsee_ != NULL) {
    tr::ExpAndTy *elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
    temp::Label *endLabel = temp::LabelFactory::NewLabel();
    std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>();
    jumps->push_back(endLabel);
    tree::LabelStm *end = new tree::LabelStm(endLabel);
    tree::Exp *r = new tree::TempExp(temp::TempFactory::NewTemp());

    tree::EseqExp *temp = new tree::EseqExp(
        cjump,
        new tree::EseqExp(
            trueLabelStm,
            new tree::EseqExp(
                new tree::MoveStm(r, then->exp_->UnEx()),
                new tree::EseqExp(
                    new tree::JumpStm(new tree::NameExp(endLabel), jumps),
                    new tree::EseqExp(
                        falseLabelStm,
                        new tree::EseqExp(
                            new tree::MoveStm(r, elsee->exp_->UnEx()),
                            new tree::EseqExp(
                                new tree::JumpStm(new tree::NameExp(endLabel),
                                                  jumps),
                                new tree::EseqExp(end, r))))))));
    tr::ExExp *retExp = new tr::ExExp(temp);
    return new tr::ExpAndTy(retExp, then->ty_);

  } else {
    tr::NxExp *retExp = new tr::NxExp(new tree::SeqStm(
        cjump,
        new tree::SeqStm(trueLabelStm,
                         new tree::SeqStm(then->exp_->UnNx(), falseLabelStm))));
    return new tr::ExpAndTy(retExp, then->ty_);
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *check_test =
      test_->Translate(venv, tenv, level, label, errormsg);
  tr::Cx test_cx = check_test->exp_->UnCx(errormsg);
  tree::CjumpStm *cjump = (tree::CjumpStm *)(test_cx.stm_);
  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = cjump->true_label_;
  temp::Label *done_label = cjump->false_label_;
  auto *jumpList = new std::vector<temp::Label *>();
  jumpList->push_back(test_label);
  tr::ExpAndTy *check_body =
      body_->Translate(venv, tenv, level, done_label, errormsg);
  tree::SeqStm *tempStm = new tree::SeqStm(
      new tree::LabelStm(test_label),
      new tree::SeqStm(
          cjump,
          new tree::SeqStm(
              new tree::LabelStm(body_label),
              new tree::SeqStm(
                  check_body->exp_->UnNx(),
                  new tree::SeqStm(new tree::JumpStm(
                                       new tree::NameExp(test_label), jumpList),
                                   new tree::LabelStm(done_label))))));
  tr::NxExp *retExp = new tr::NxExp(tempStm);
  return new tr::ExpAndTy(retExp, check_body->ty_);
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *check_lo = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_hi = hi_->Translate(venv, tenv, level, label, errormsg);
  temp::Label *loop_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();

  // add local variable
  venv->BeginScope();
  env::VarEntry *i =
      new env::VarEntry(tr::Access::AllocLocal(level, escape_), check_lo->ty_);
  venv->Enter(var_, i);
  tr::ExpAndTy *check_body =
      body_->Translate(venv, tenv, level, done_label, errormsg);
  venv->EndScope();

  // genarate varExp and initialize it
  tree::Exp *limitVar = check_hi->exp_->UnEx();
  tree::Exp *loopVar;
  if (escape_) {
    env::VarEntry *targetEntry = (env::VarEntry *)(venv->Look(var_));
    tr::Level *targetLevel = targetEntry->access_->level_;
    tree::Exp *static_link = findBySL(level, targetLevel);
    loopVar = targetEntry->access_->access_->ToExp(static_link);
  } else {
    loopVar = i->access_->access_->ToExp(NULL);
  }
  tree::Stm *init_loopVar_stm =
      new tree::MoveStm(loopVar, check_lo->exp_->UnEx());

  auto labelList = new std::vector<temp::Label *>();
  labelList->push_back(body_label);

  tree::CjumpStm *largerThan = new tree::CjumpStm(
      tree::GT_OP, loopVar, limitVar, done_label, body_label);
  tree::CjumpStm *equalTo = new tree::CjumpStm(tree::EQ_OP, loopVar, limitVar,
                                               done_label, loop_label);
  tree::CjumpStm *lessThan = new tree::CjumpStm(tree::LT_OP, loopVar, limitVar,
                                                loop_label, done_label);
  tree::Stm *increase_loop_var =
      new tree::MoveStm(loopVar, new tree::BinopExp(tree::PLUS_OP, loopVar,
                                                    new tree::ConstExp(1)));
  tree::Stm *loop_stm = new tree::SeqStm(
      new tree::LabelStm(loop_label),
      new tree::SeqStm(
          increase_loop_var,
          new tree::JumpStm(new tree::NameExp(body_label), labelList)));

  tree::Stm *totalStm = new tree::SeqStm(
      init_loopVar_stm,
      new tree::SeqStm(
          largerThan,
          new tree::SeqStm(
              new tree::LabelStm(body_label),
              new tree::SeqStm(
                  check_body->exp_->UnNx(),
                  new tree::SeqStm(
                      equalTo,
                      new tree::SeqStm(
                          loop_stm,
                          new tree::SeqStm(
                              lessThan, new tree::LabelStm(done_label))))))));
  return new tr::ExpAndTy(new tr::NxExp(totalStm), type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tree::Stm *stm = new tree::JumpStm(new tree::NameExp(label),
                                     new std::vector<temp::Label *>({label}));
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // record if it is main declaration
  static bool first = true;
  bool isMain = false;
  if (first) {
    isMain = true;
    first = false;
  }
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm *decStm;
  int firstStm = true;
  for (Dec *dec : decs_->GetList()) {
    if (firstStm) {
      decStm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
      firstStm = false;
    } else {
      decStm = new tree::SeqStm(
          decStm, dec->Translate(venv, tenv, level, label, errormsg)->UnNx());
    }
  }
  tr::ExpAndTy *check_body =
      body_->Translate(venv, tenv, level, label, errormsg);
  venv->EndScope();
  tenv->EndScope();

  tree::Exp *tempExp;
  if (decStm) {
    tempExp = new tree::EseqExp(decStm, check_body->exp_->UnEx());
  } else {
    tempExp = check_body->exp_->UnEx();
  }
  decStm = new tree::ExpStm(tempExp);
  if (isMain) {
    frags->PushBack(new frame::ProcFrag(decStm, level->frame_));
  }
  return new tr::ExpAndTy(new tr::ExExp(tempExp), check_body->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *check_size =
      size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *check_init =
      init_->Translate(venv, tenv, level, label, errormsg);
  tree::ExpList *args =
      new tree::ExpList({check_size->exp_->UnEx(), check_init->exp_->UnEx()});
  tr::Exp *retExp = new tr::ExExp(frame::ExternalCall("init_array", args));
  return new tr::ExpAndTy(retExp, check_init->ty_);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> declist = functions_->GetList();
  // finish section 6
  // first pass
  for (FunDec *function : declist) {
    type::TyList *typeArgList =
        function->params_->MakeFormalTyList(tenv, errormsg);
    temp::Label *funLabel =
        temp::LabelFactory::NamedLabel(function->name_->Name());
    // add sl into it
    tr::Level *new_level = tr::Level::NewLevel(
        funLabel, function->params_->MakeEscapeList(), level);
    if (function->result_) {
      type::Ty *resultTy = tenv->Look(function->result_);
      venv->Enter(function->name_, new env::FunEntry(new_level, funLabel,
                                                     typeArgList, resultTy));
    } else {
      venv->Enter(function->name_,
                  new env::FunEntry(new_level, funLabel, typeArgList,
                                    type::VoidTy::Instance()));
    }
  }
  // second pass
  for (FunDec *function : declist) {
    venv->BeginScope();
    env::FunEntry *functionEntry =
        (env::FunEntry *)(venv->Look(function->name_));

    std::list<frame::Access *> accessList =
        functionEntry->level_->frame_->formals_;
    // skip SL in accessList_it
    // printf("size: %ld\n", accessList.size());
    auto accessList_it = accessList.begin();
    // printf("access offset: %d\n", (*accessList_it)->getOffset());
    accessList_it++;
    std::list<type::Ty *> formaltys = functionEntry->formals_->GetList();
    auto type_it = formaltys.begin();

    for (Field *arg : function->params_->GetList()) {
      venv->Enter(
          arg->name_,
          new env::VarEntry(
              new tr::Access(functionEntry->level_, *accessList_it), *type_it));
      type_it++;
      accessList_it++;
    }

    auto res = function->body_->Translate(venv, tenv, functionEntry->level_,
                                          functionEntry->label_, errormsg);
    tree::MoveStm *stm = new tree::MoveStm(
        new tree::TempExp(reg_manager->ReturnValue()), res->exp_->UnEx());
    tree::Stm *proc = ProcEntryExit1(functionEntry->level_->frame_, stm);
    frags->PushBack(new frame::ProcFrag(proc, functionEntry->level_->frame_));
    venv->EndScope();
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *check_init =
      init_->Translate(venv, tenv, level, label, errormsg);
  tr::Access *localVar = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(localVar, check_init->ty_));
  tree::Stm *init_stm = new tree::MoveStm(
      localVar->access_->ToExp(new tree::TempExp(reg_manager->FramePointer())),
      check_init->exp_->UnEx());
  return new tr::NxExp(init_stm);
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // first pass
  std::list<NameAndTy *> list = types_->GetList();
  for (NameAndTy *item : list) {
    tenv->Enter(item->name_, type::VoidTy::Instance());
  }
  // second pass (there may forward definition)
  for (NameAndTy *item : list) {
    tenv->Set(item->name_,item->ty_->Translate(tenv, errormsg));
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

// transfer type in absyn to the one in types
type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *exactType = tenv->Look(name_);
  return new type::NameTy(name_, exactType);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList *field = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(field);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *array_ty = tenv->Look(array_);
  return new type::ArrayTy(array_ty);
}

} // namespace absyn
