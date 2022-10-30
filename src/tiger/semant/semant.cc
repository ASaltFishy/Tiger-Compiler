#include "tiger/semant/semant.h"
#include "tiger/absyn/absyn.h"

namespace absyn {
type::FieldList *makeFiledTyList(env::TEnvPtr tenv, err::ErrorMsg *errormsg,
                                 FieldList *field) {
  if (!field)
    return NULL;
  type::FieldList *temp;
  bool first = true;
  for (Field *item : field->GetList()) {
    type::Ty *item_ty = tenv->Look(item->typ_);
    if (!item_ty) {
      errormsg->Error(item->pos_, "undefined type %s",
                      item->typ_->Name().data());
    }
    if (first) {
      temp = new type::FieldList(new type::Field(item->name_, item_ty));
      first = false;
    } else {
      temp->Append(new type::Field(item->name_, item_ty));
    }
  }
  return temp;
}

type::TyList *makeFormalTyList(env::TEnvPtr tenv, err::ErrorMsg *errormsg,
                               FieldList *formal) {
  if (!formal)
    return NULL;
  type::TyList *temp;
  bool first = true;
  for (Field *item : formal->GetList()) {
    type::Ty *item_ty = tenv->Look(item->typ_);
    if (!item_ty) {
      errormsg->Error(item->pos_, "undefined type %s",
                      item->typ_->Name().data());
    }
    if (first) {
      temp = new type::TyList(item_ty);
      first = false;
    } else {
      temp->Append(item_ty);
    }
  }
  return temp;
}

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return (static_cast<env::VarEntry *>(entry))->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ret = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*ret) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  } else {
    type::RecordTy *record = (type::RecordTy *)ret;
    std::list<type::Field *> list = record->fields_->GetList();
    for (auto it = list.begin(); it != list.end(); it++) {
      if ((*it)->name_ == sym_) {
        return (*it)->ty_->ActualTy();
      }
    }
    errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ret = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*ret) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
  type::Ty *subRet = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*subRet) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "invalid array index");
    return type::IntTy::Instance();
  }
  return ((type::ArrayTy *)ret)->ty_->ActualTy();
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(func_);
  if (entry && typeid(*entry) == typeid(env::FunEntry)) {
    env::FunEntry *fun = (env::FunEntry *)entry;
    std::list<type::Ty *> formal_list = fun->formals_->GetList();
    std::list<Exp *> actual_list = args_->GetList();
    auto formal = formal_list.begin();
    auto actual = actual_list.begin();
    while (formal != formal_list.end() && actual != actual_list.end()) {
      if (!(*actual)
               ->SemAnalyze(venv, tenv, labelcount, errormsg)
               ->IsSameType((*formal)->ActualTy())) {
        errormsg->Error((*actual)->pos_, "missmatch para type");
        return type::IntTy::Instance();
      }
      formal++;
      actual++;
    }
    if ((*formal)) {
      errormsg->Error(pos_, "missing para");
      return type::IntTy::Instance();
    }
    if ((*actual)) {
      errormsg->Error(pos_, "too much para");
      return type::IntTy::Instance();
    }
    return fun->result_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *left_type =
      left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *right_type =
      right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (oper_ == absyn::PLUS_OP || oper_ == absyn::MINUS_OP ||
      oper_ == absyn::TIMES_OP || oper_ == absyn::DIVIDE_OP) {
    if (typeid(*left_type) != typeid(type::IntTy))
      errormsg->Error(left_->pos_, "integer required");
    if (typeid(*right_type) != typeid(type::IntTy))
      errormsg->Error(right_->pos_, "integer required");
  } else {
    if (!left_type->IsSameType(right_type)) {
      errormsg->Error(pos_, "same type required");
    }
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ret = tenv->Look(typ_)->ActualTy();
  if (!ret) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
  } else {
    // why no need to check fieldlit's type?
  }
  return ret;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ret;
  for (Exp *item : seq_->GetList()) {
    ret = item->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return ret;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *var_ty =
      var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *exp_ty =
      exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (!var_ty->IsSameType(exp_ty)) {
    errormsg->Error(pos_, "unmatched assign exp");
  }
  if (typeid(*var_) == typeid(SimpleVar)) {
    env::EnvEntry *entry = venv->Look(((SimpleVar *)var_)->sym_);
    if (entry->readonly_) {
      errormsg->Error(var_->pos_, "loop variable can't be assigned");
    }
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (then_ty && !then_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
  }
  if (elsee_) {
    type::Ty *elsee_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (elsee_ty && !elsee_ty->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(elsee_->pos_, "if-then exp's body must produce no value");
    }
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!test_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(test_->pos_, "integer required");
  }
  if (!body_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "while body must produce no value");
  }
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *lo_ty = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *hi_ty = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!lo_ty->IsSameType(type::IntTy::Instance()) ||
      !hi_ty->IsSameType(type::IntTy::Instance())) {
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  venv->BeginScope();
  tenv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty *var_ty = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!var_ty->IsSameType(type::VoidTy::Instance())) {
    errormsg->Error(body_->pos_, "for body must produce no value");
  }
  venv->EndScope();
  tenv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  for (Dec *dec : decs_->GetList()) {
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  type::Ty *result;
  if (!body_)
    result = type::VoidTy::Instance();
  else
    result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  venv->EndScope();
  tenv->EndScope();
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *entry = tenv->Look(typ_)->ActualTy();
  if (!entry || typeid(*entry) == typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
  type::Ty *expected = ((type::ArrayTy *)entry)->ty_->ActualTy();
  type::Ty *size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!size_ty->IsSameType(type::IntTy::Instance()))
    errormsg->Error(pos_, "type mismatch");
  if (!init_ty->IsSameType(expected))
    errormsg->Error(pos_, "type mismatch");
  return expected;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec *> function_list = functions_->GetList();
  for (FunDec *function : function_list) {
    venv->BeginScope();

    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typ_) {
    type::Ty *typ_ty = tenv->Look(typ_);
    if (!typ_ty) {
      errormsg->Error(pos_, "undefined type of %s", typ_->Name().data());
      return;
    }
    if (!typ_ty->IsSameType(init_ty)) {
      errormsg->Error(pos_, "type mismatch");
      return;
    }
  }
  venv->Enter(var_, new env::VarEntry(init_ty));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<NameAndTy *> list = types_->GetList();
  // first pass
  for (NameAndTy *tyDec : list) {
    if (tenv->Look(tyDec->name_)) {
      errormsg->Error(pos_, "already exit type name");
      return;
    }
    tenv->Enter(tyDec->name_, new type::NameTy(tyDec->name_, NULL));
  }
  // second pass
  for (NameAndTy *tyDec : list) {
    type::Ty *entry = tenv->Look(tyDec->name_);
    ((type::NameTy *)entry)->ty_ = (tyDec->ty_)->SemAnalyze(tenv, errormsg);
  }
  // check recurssive type definition
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *name_ty = tenv->Look(name_);
  if (!name_ty) {
    errormsg->Error(pos_, "undefined type of %s", name_->Name().data());
  }
  return name_ty;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::FieldList *field = makeFiledTyList(tenv, errormsg, record_);
  return new type::RecordTy(field);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *array_ty = tenv->Look(array_);
  if (!array_ty) {
    errormsg->Error(pos_, "undefined type of %s", array_->Name().data());
  }
  return array_ty;
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}
} // namespace sem
