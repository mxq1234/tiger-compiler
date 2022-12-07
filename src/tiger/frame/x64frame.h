//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
private:
  temp::Temp *rax, *rsp, *rbp;
  temp::Temp *rdi, *rsi, *rdx, *rcx, *r8, *r9, *r10, *r11;
  temp::Temp *rbx, *r12, *r13, *r14, *r15;

public:
  X64RegManager() {
    rax = temp::TempFactory::NewTemp();
    rsp = temp::TempFactory::NewTemp();
    rbp = temp::TempFactory::NewTemp();
    rsi = temp::TempFactory::NewTemp();
    rdi = temp::TempFactory::NewTemp();
    rdx = temp::TempFactory::NewTemp();
    rcx = temp::TempFactory::NewTemp();
    rbx = temp::TempFactory::NewTemp();
    r8 = temp::TempFactory::NewTemp();
    r9 = temp::TempFactory::NewTemp();
    r10 = temp::TempFactory::NewTemp();
    r11 = temp::TempFactory::NewTemp();
    r12 = temp::TempFactory::NewTemp();
    r13 = temp::TempFactory::NewTemp();
    r14 = temp::TempFactory::NewTemp();
    r15 = temp::TempFactory::NewTemp();
    regs_.assign({ rax, rsp, rbp, rsi, rdi, rdx, rcx, rbx, r8, r9, r10, r11, r12, r13, r14, r15 });
    std::vector<std::string> regsName;
    regsName.assign({ "%rax", "%rsp", "%rbp", "%rsi", "%rdi", "%rdx", "%rcx", "%rbx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15" });
    for(int i = 0; i < 16; ++i)
      temp_map_->Enter(regs_[i], new std::string(regsName[i]));
  }
  temp::TempList *Registers() override {
    return new temp::TempList({ rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11, rbx, r12, r13, r14, r15 });
  }
  temp::TempList *ArgRegs() override {
    return new temp::TempList({ rdi, rsi, rdx, rcx, r8, r9 });
  }
  temp::TempList *CallerSaves() override {
    return new temp::TempList({ rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11 });
  }
  temp::TempList *CalleeSaves() override {
    return new temp::TempList({ rbp, rsp, rbx, r12, r13, r14, r15 });
  }
  temp::TempList *ReturnSink() override {
    return new temp::TempList({ rbp, rsp, rbx, r12, r13, r14, r15, rax });
  }
  int WordSize() override { return 8; }
  int RegNum() override { return 16; }
  temp::Temp *FramePointer() override { return rbp; }
  temp::Temp *StackPointer() override { return rsp; }
  temp::Temp *ReturnValue() override { return rax; }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
