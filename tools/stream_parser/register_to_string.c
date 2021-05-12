#include "register_to_string.h"

#include <bun/stream.h>

const char *bun_register_to_string(uint16_t val)
{
#define CASE(x) case x: return #x
    switch (val) {
    CASE(BUN_REGISTER_X86_64_RAX);
    CASE(BUN_REGISTER_X86_64_RBX);
    CASE(BUN_REGISTER_X86_64_RCX);
    CASE(BUN_REGISTER_X86_64_RDX);
    CASE(BUN_REGISTER_X86_64_RSI);
    CASE(BUN_REGISTER_X86_64_RDI);
    CASE(BUN_REGISTER_X86_64_RBP);
    CASE(BUN_REGISTER_X86_64_RSP);
    CASE(BUN_REGISTER_X86_64_R8);
    CASE(BUN_REGISTER_X86_64_R9);
    CASE(BUN_REGISTER_X86_64_R10);
    CASE(BUN_REGISTER_X86_64_R11);
    CASE(BUN_REGISTER_X86_64_R12);
    CASE(BUN_REGISTER_X86_64_R13);
    CASE(BUN_REGISTER_X86_64_R14);
    CASE(BUN_REGISTER_X86_64_R15);
    CASE(BUN_REGISTER_X86_64_RIP);
    CASE(BUN_REGISTER_X86_EAX);
    CASE(BUN_REGISTER_X86_EBX);
    CASE(BUN_REGISTER_X86_ECX);
    CASE(BUN_REGISTER_X86_EDX);
    CASE(BUN_REGISTER_X86_ESI);
    CASE(BUN_REGISTER_X86_EDI);
    CASE(BUN_REGISTER_X86_EBP);
    CASE(BUN_REGISTER_X86_ESP);
    CASE(BUN_REGISTER_X86_EIP);
    CASE(BUN_REGISTER_AARCH64_X0);
    CASE(BUN_REGISTER_AARCH64_X1);
    CASE(BUN_REGISTER_AARCH64_X2);
    CASE(BUN_REGISTER_AARCH64_X3);
    CASE(BUN_REGISTER_AARCH64_X4);
    CASE(BUN_REGISTER_AARCH64_X5);
    CASE(BUN_REGISTER_AARCH64_X6);
    CASE(BUN_REGISTER_AARCH64_X7);
    CASE(BUN_REGISTER_AARCH64_X8);
    CASE(BUN_REGISTER_AARCH64_X9);
    CASE(BUN_REGISTER_AARCH64_X10);
    CASE(BUN_REGISTER_AARCH64_X11);
    CASE(BUN_REGISTER_AARCH64_X12);
    CASE(BUN_REGISTER_AARCH64_X13);
    CASE(BUN_REGISTER_AARCH64_X14);
    CASE(BUN_REGISTER_AARCH64_X15);
    CASE(BUN_REGISTER_AARCH64_X16);
    CASE(BUN_REGISTER_AARCH64_X17);
    CASE(BUN_REGISTER_AARCH64_X18);
    CASE(BUN_REGISTER_AARCH64_X19);
    CASE(BUN_REGISTER_AARCH64_X20);
    CASE(BUN_REGISTER_AARCH64_X21);
    CASE(BUN_REGISTER_AARCH64_X22);
    CASE(BUN_REGISTER_AARCH64_X23);
    CASE(BUN_REGISTER_AARCH64_X24);
    CASE(BUN_REGISTER_AARCH64_X25);
    CASE(BUN_REGISTER_AARCH64_X26);
    CASE(BUN_REGISTER_AARCH64_X27);
    CASE(BUN_REGISTER_AARCH64_X28);
    CASE(BUN_REGISTER_AARCH64_X29);
    CASE(BUN_REGISTER_AARCH64_X30);
    CASE(BUN_REGISTER_AARCH64_X31);
    CASE(BUN_REGISTER_AARCH64_PC);
    CASE(BUN_REGISTER_AARCH64_PSTATE);
    CASE(BUN_REGISTER_ARM_R0);
    CASE(BUN_REGISTER_ARM_R1);
    CASE(BUN_REGISTER_ARM_R2);
    CASE(BUN_REGISTER_ARM_R3);
    CASE(BUN_REGISTER_ARM_R4);
    CASE(BUN_REGISTER_ARM_R5);
    CASE(BUN_REGISTER_ARM_R6);
    CASE(BUN_REGISTER_ARM_R7);
    CASE(BUN_REGISTER_ARM_R8);
    CASE(BUN_REGISTER_ARM_R9);
    CASE(BUN_REGISTER_ARM_R10);
    CASE(BUN_REGISTER_ARM_R11);
    CASE(BUN_REGISTER_ARM_R12);
    CASE(BUN_REGISTER_ARM_R13);
    CASE(BUN_REGISTER_ARM_R14);
    CASE(BUN_REGISTER_ARM_R15);
    CASE(BUN_REGISTER_ARM_PSTATE);
    default: return "<unknown>";
    }
}
