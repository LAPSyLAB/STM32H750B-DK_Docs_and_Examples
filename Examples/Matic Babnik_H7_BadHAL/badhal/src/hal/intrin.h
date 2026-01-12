/*
ARM pseudo-intrinsics
*/
#include "bad.h"

INLINE_ALWAYS void a_dmb() { asm volatile("dmb"); }
INLINE_ALWAYS void a_isb() { asm volatile("isb"); }
INLINE_ALWAYS void a_dsb() { asm volatile("dsb"); }
INLINE_ALWAYS void a_nop() { asm volatile("nop"); }

INLINE_ALWAYS void a_interrupts_on() { asm volatile("cpsie i"); }
INLINE_ALWAYS void a_interrupts_off() { asm volatile("cpsid i"); }