#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  auto *list = new assem::InstrList();
  fs_ = frame_->GetLabel() + "_framesize";
  for (auto stm : traces_->GetStmList()->GetList())
    stm->Munch(*list, fs_);
  assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Label *jumpTo = exp_->name_;
  instr_list.Append(new assem::OperInstr("jmp " + jumpTo->Name(), nullptr,
                                         nullptr, new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *left_reg = left_->Munch(instr_list, fs);
  temp::Temp *right_reg = right_->Munch(instr_list, fs);

  std::string op_str;
  switch (op_) {
  case EQ_OP:
    op_str = std::string("je ");
    break;
  case NE_OP:
    op_str = std::string("jne ");
    break;
  case LT_OP:
    op_str = std::string("jl ");
    break;
  case GT_OP:
    op_str = std::string("jg ");
    break;
  case LE_OP:
    op_str = std::string("jle ");
    break;
  case GE_OP:
    op_str = std::string("jge ");
    break;
  default:
    break;
  }
  auto labelList = new std::vector<temp::Label *>({true_label_});
  instr_list.Append(
      new assem::OperInstr("cmpq `s0, `s1", nullptr,
                           new temp::TempList({right_reg, left_reg}), nullptr));
  instr_list.Append(new assem::OperInstr(op_str + true_label_->Name(), nullptr,
                                         nullptr,
                                         new assem::Targets(labelList)));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (typeid(*dst_) == typeid(TempExp)) {
    auto *src_reg = src_->Munch(instr_list, fs);
    temp::Temp *det_reg = ((TempExp *)dst_)->temp_;
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(det_reg),
                                           new temp::TempList(src_reg)));
  } else if (typeid(*dst_) == typeid(MemExp)) {
    auto left_reg = src_->Munch(instr_list, fs);
    auto right_reg = ((MemExp *)dst_)->exp_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(
        "movq `s0, (`s1)", nullptr, new temp::TempList({left_reg, right_reg}),
        nullptr));
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *newReg = temp::TempFactory::NewTemp();
  temp::Temp *left_reg = left_->Munch(instr_list, fs);
  temp::Temp *right_reg = right_->Munch(instr_list, fs);
  auto x64RM = (frame::X64RegManager *)(reg_manager);

  switch (op_) {
  case PLUS_OP:
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(newReg),
                                           new temp::TempList(left_reg)));
    instr_list.Append(
        new assem::OperInstr("addq `s0, `d0", new temp::TempList(newReg),
                             new temp::TempList({right_reg,newReg}), nullptr));
    break;
  case MINUS_OP:
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(newReg),
                                           new temp::TempList(left_reg)));
    instr_list.Append(
        new assem::OperInstr("subq `s0, `d0", new temp::TempList(newReg),
                             new temp::TempList({right_reg,newReg}), nullptr));
    break;
  case MUL_OP:
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(x64RM->rax),
                                           new temp::TempList(left_reg)));
    instr_list.Append(new assem::MoveInstr(
        "cqto", new temp::TempList({x64RM->rdx}),
        new temp::TempList(x64RM->rax)));
    instr_list.Append(
        new assem::OperInstr("imulq `s0", new temp::TempList({x64RM->rax,x64RM->rdx}),
                             new temp::TempList({right_reg,x64RM->rax}), nullptr));
    instr_list.Append(
        new assem::OperInstr("movq `s0, `d0", new temp::TempList(newReg),
                             new temp::TempList(x64RM->rax), nullptr));
    break;
  case DIV_OP:
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                           new temp::TempList(x64RM->rax),
                                           new temp::TempList(left_reg)));
    instr_list.Append(new assem::MoveInstr(
        "cqto", new temp::TempList({x64RM->rdx}),
        new temp::TempList(x64RM->rax)));
    instr_list.Append(new assem::OperInstr(
        "idivq `s0", new temp::TempList({x64RM->rax, x64RM->rdx}),
        new temp::TempList({right_reg,x64RM->rax}), nullptr));
    instr_list.Append(
        new assem::OperInstr("movq `s0, `d0", new temp::TempList(newReg),
                             new temp::TempList(x64RM->rax), nullptr));
    break;
  default:
    break;
  }
  return newReg;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::Temp *src_reg = exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0",
                                         new temp::TempList(reg),
                                         new temp::TempList(src_reg), nullptr));
  return reg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *reg;
  if (temp_ != reg_manager->FramePointer()) {
    reg = temp_;
  } else {
    reg = temp::TempFactory::NewTemp();
    instr_list.Append(
        new assem::OperInstr("leaq " + std::string(fs) + "(%rsp), `d0",
                             new temp::TempList(reg), nullptr, nullptr));
  }
  return reg;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *reg = temp::TempFactory::NewTemp();
  std::string assem = "leaq " + name_->Name() + "(%rip), `d0";
  instr_list.Append(
      new assem::OperInstr(assem, new temp::TempList(reg), nullptr, nullptr));
  return reg;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *reg = temp::TempFactory::NewTemp();
  std::string assem = "movq $" + std::to_string(consti_) + ", `d0";
  instr_list.Append(
      new assem::MoveInstr(assem, new temp::TempList(reg), nullptr));
  return reg;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *reg = temp::TempFactory::NewTemp();
  temp::TempList *argTemps = args_->MunchArgs(instr_list, fs);
  int argsize = args_->GetList().size();

  // set args and move rsp (fp will not be used)
  std::list<temp::Temp *> argRegList = reg_manager->ArgRegs()->GetList();
  auto argReg = argRegList.begin();
  temp::TempList *active = new temp::TempList();
  int i = 1;
  for (temp::Temp *temp : argTemps->GetList()) {
    if (i <= 6) {
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                             new temp::TempList(*argReg),
                                             new temp::TempList(temp)));
      active->Append(*argReg);
      argReg++;
      i++;
    } else {
      auto arg_rev_it = argTemps->GetList().rbegin();
      i = argsize;
      while (i > 6) {
        instr_list.Append(new assem::OperInstr(
            "subq $" + std::to_string(reg_manager->WordSize()) + ", `d0",
            new temp::TempList(reg_manager->StackPointer()), nullptr, nullptr));
        instr_list.Append(new assem::MoveInstr(
            "movq `s0, (`d0)", new temp::TempList(reg_manager->StackPointer()),
            new temp::TempList((*arg_rev_it))));
        i--;
        arg_rev_it++;
      }
      break;
    }
  }

  std::string assem = "callq " + ((tree::NameExp *)fun_)->name_->Name();
  instr_list.Append(
      new assem::OperInstr(assem, reg_manager->CallerSaves(), active, nullptr));
  instr_list.Append(
      new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg),
                           new temp::TempList(reg_manager->ReturnValue())));

  // reset rsp, discard the argument in stack after calling
  if (argsize > 6) {
    instr_list.Append(new assem::OperInstr(
        "addq $" + std::to_string((argsize - 6) * reg_manager->WordSize()) +
            ", %rsp",
        nullptr, nullptr, nullptr));
  }

  return reg;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto res = new temp::TempList();
  for (auto arg : exp_list_) {
    res->Append(arg->Munch(instr_list, fs));
  }
  return res;

}

} // namespace tree
