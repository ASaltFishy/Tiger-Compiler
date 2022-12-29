//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  temp::Temp *rax, *rbx, *rcx, *rdx, *rsp, *rbp, *rsi, *rdi, *r8, *r9, *r10,
      *r11, *r12, *r13, *r14, *r15;
  temp::TempList *registers, *caller_save, *callee_save, *arg_reg, *return_sink, *initialInterfere;
  X64RegManager() {
    rax = temp::TempFactory::NewTemp();
    rbx = temp::TempFactory::NewTemp();
    rcx = temp::TempFactory::NewTemp();
    rdx = temp::TempFactory::NewTemp();
    rbp = temp::TempFactory::NewTemp();
    rsp = temp::TempFactory::NewTemp();
    rsi = temp::TempFactory::NewTemp();
    rdi = temp::TempFactory::NewTemp();
    r8 = temp::TempFactory::NewTemp();
    r9 = temp::TempFactory::NewTemp();
    r10 = temp::TempFactory::NewTemp();
    r11 = temp::TempFactory::NewTemp();
    r12 = temp::TempFactory::NewTemp();
    r13 = temp::TempFactory::NewTemp();
    r14 = temp::TempFactory::NewTemp();
    r15 = temp::TempFactory::NewTemp();
    registers = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11, rbx,
                                    rbp, r12, r13, r14, r15, rsp});
    caller_save =
        new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11});
    callee_save = new temp::TempList({rbx, rbp, r12, r13, r14, r15});
    arg_reg = new temp::TempList({rdi, rsi, rdx, rcx, r8, r9});
    // which register is active in the exit of function
    return_sink = new temp::TempList({rsp, rax, rbx, rbp, r12, r13, r14, r15});
    initialInterfere = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11, rbx,
                                    rbp, r12, r13, r14, r15});

    temp_map_ = temp::Map::Empty();
    temp_map_->Enter(rax, new std::string("%rax"));
    temp_map_->Enter(rdi, new std::string("%rdi"));
    temp_map_->Enter(rsi, new std::string("%rsi"));
    temp_map_->Enter(rdx, new std::string("%rdx"));
    temp_map_->Enter(rcx, new std::string("%rcx"));
    temp_map_->Enter(r8, new std::string("%r8"));
    temp_map_->Enter(r9, new std::string("%r9"));
    temp_map_->Enter(r10, new std::string("%r10"));
    temp_map_->Enter(r11, new std::string("%r11"));
    temp_map_->Enter(rbx, new std::string("%rbx"));
    temp_map_->Enter(rbp, new std::string("%rbp"));
    temp_map_->Enter(r12, new std::string("%r12"));
    temp_map_->Enter(r13, new std::string("%r13"));
    temp_map_->Enter(r14, new std::string("%r14"));
    temp_map_->Enter(r15, new std::string("%r15"));
    temp_map_->Enter(rsp, new std::string("%rsp"));
  }
  temp::TempList *Registers() override { return registers; }

  temp::TempList *ArgRegs() override { return arg_reg; }

  temp::TempList *CallerSaves() override { return caller_save; }

  temp::TempList *CalleeSaves() override { return callee_save; }

  temp::TempList *ReturnSink() override { return return_sink; }

  int WordSize() override { return 8; }

  // forbidden to use %RBP as frame pointer, but in translation it will be used.
  temp::Temp *FramePointer() override { return rbp; }

  temp::Temp *StackPointer() override { return rsp; }

  temp::Temp *ReturnValue() override { return rax; }

  temp::TempList *intialInterfere() override {
     return initialInterfere;
    }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
