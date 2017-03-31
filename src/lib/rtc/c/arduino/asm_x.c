#include "asm.h"
#include "rtc_emit.h"
#include "config.h"
#include <avr/pgmspace.h>

// push pop order
// Ints: Push 1, Push 0     Pop 0, Pop 1 (Ints are stored LE in the registers, so this push order stores them BE in memory since int stack grows down. This is what DJ expects)
// Refs: Push 0, Push 1     Pop 1, Pop 0

void emit_x_CALL(uint16_t target) {
    // Flush the code buffer before emitting a CALL to prevent PUSH/POP pairs being optimised across a CALL instruction.
    emit_PUSH(RXH);
    emit_PUSH(RXL);
    emit_flush_to_flash();
    emit_2_CALL(target);
    emit_POP(RXL);
    emit_POP(RXH);
}

void emit_x_PUSH_32bit(uint8_t base) {
    emit_PUSH(base+3);
    emit_PUSH(base+2);
    emit_PUSH(base+1);
    emit_PUSH(base+0);
}
void emit_x_PUSH_16bit(uint8_t base) {
    emit_PUSH(base+1);
    emit_PUSH(base+0);
}
void emit_x_PUSH_REF(uint8_t base) {
    emit_x_PUSHREF8(base+0);
    emit_x_PUSHREF8(base+1);
}

void emit_x_POP_32bit(uint8_t base) {
    emit_POP(base+0);
    emit_POP(base+1);
    emit_POP(base+2);
    emit_POP(base+3);
}
void emit_x_POP_16bit(uint8_t base) {
    emit_POP(base+0);
    emit_POP(base+1);
}
void emit_x_POP_REF(uint8_t base) {
    emit_x_POPREF8(base+1);
    emit_x_POPREF8(base+0);
}
void emit_x_avroraBeep(uint8_t beep) {
    emit_PUSH(R24);
    emit_LDI(R24, beep);
    emit_2_STS((uint16_t)&rtcMonitorVariable+1, R24);
    emit_LDI(R24, AVRORA_RTC_BEEP);
    emit_2_STS((uint16_t)&rtcMonitorVariable, R24);
    emit_POP(R24);
}
void emit_x_avroraPrintPC() {
    emit_PUSH(R24);
    emit_LDI(R24, 0x12);
    emit_2_STS((uint16_t)&(debugbuf1), R24);
    emit_POP(R24);
}
void emit_x_avroraPrintRegs() {
    emit_PUSH(R24);
    emit_LDI(R24, 0xE);
    emit_2_STS((uint16_t)&(debugbuf1), R24);
    emit_POP(R24);
}


// This needs to be a #define to calculate the instruction at compile time.
// There is also a asm_opcodeWithSingleRegOperand, which is a function. This
// needs to be a function to save programme size, because expanding this macro
// with variable parameters will be much larger than the function calls.
#define asm_const_opcodeWithSingleRegOperand(opcode, reg) (((opcode) + ((reg) << 4)))

// NOTE THAT THIS CODE ONLY WORKS ON AVR DEVICES WITH <=128KB flash.
// On larger devices, such as the WuDevice, the return address after
// a call is 24 bit, so we need to pop/push 3 bytes at the beginning
// and end of the fragments below.
// (POP R18; POP19 -> POP R18; POP R19; POP R20)
const uint16_t PROGMEM emit_x_preinvoke_code[] =
{ asm_const_POP(R18),
  asm_const_POP(R19),

  // set intStack to SP
  asm_const_PUSH(ZERO_REG), // NOTE: THE DVM STACK IS A 16 BIT POINTER, SP IS 8 BIT. 
                            // BOTH POINT TO THE NEXT free SLOT, BUT SINCE THEY GROW down THIS MEANS THE DVM POINTER SHOULD POINT TO TWO BYTES BELOW THE LAST VALUE,
                            // WHILE CURRENTLY THE NATIVE SP POINTS TO THE BYTE DIRECTLY BELOW IT. RESERVE AN EXTRA BYTE TO FIX THIS.
  asm_const_IN(R0, SP_L_port),       // Load SPL into R0
  asm_const_opcodeWithSingleRegOperand(OPCODE_STS, R0), INTSTACKADDRESS,   // Store R0 into intStackL
  asm_const_IN(R0, SP_H_port),       // Load SPH into R0
  asm_const_opcodeWithSingleRegOperand(OPCODE_STS, R0), INTSTACKADDRESS+1, // Store R0 into intStackH
  // Reserve 8 bytes of space on the stack, in case the returned int is large than passed ints
  // TODO: make this more efficient by looking up the method, and seeing if the return type is int,
  //       and if so, if the size of the return type is larger than the integers passed. Then only
  //       reserve the space that's needed.
  //       This is for the worst case, where no ints are passed, so there's no space reserved, and
  //       a 32 bit long is returned.
  asm_const_RCALL(0), // RCALL to offset 0 does nothing, except reserving 2 bytes on the stack. cheaper than two useless pushes.
  asm_const_RCALL(0),
  // Pre possible GC: need to store X in refStack: for INVOKEs to pass the references, for other cases just to make sure the GC will update the pointer if it runs.
  asm_const_opcodeWithSingleRegOperand(OPCODE_STS, RXL), REFSTACKADDRESS,
  asm_const_opcodeWithSingleRegOperand(OPCODE_STS, RXH), REFSTACKADDRESS+1,

  asm_const_PUSH(R19),
  asm_const_PUSH(R18),
  OPCODE_RET };
void emit_x_preinvoke() {
    emit_2_CALL(((uint16_t)emit_x_preinvoke_code)/2);
}

const uint16_t PROGMEM emit_x_postinvoke_code[] =
{ asm_const_POP(R18),
  asm_const_POP(R19),

#ifndef EXECUTION_FRAME_ON_STACK
  // Y is call-saved and won't move if the frame is on the stack
  // Post possible GC: need to reset Y to the start of the stack frame's local references (the frame may have moved, so the old value may not be correct)
  asm_const_opcodeWithSingleRegOperand(OPCODE_LDS, RYL), LOCALREFERENCEVARIABLESADDRESS,   // Load localReferenceVariables into Y
  asm_const_opcodeWithSingleRegOperand(OPCODE_LDS, RYH), LOCALREFERENCEVARIABLESADDRESS+1, // Load localReferenceVariables into Y
#endif
  // Post possible GC: need to restore X to refStack which may have changed either because of GC or because of passed/returned references
  asm_const_opcodeWithSingleRegOperand(OPCODE_LDS, RXL), REFSTACKADDRESS,   // Load refStack into X
  asm_const_opcodeWithSingleRegOperand(OPCODE_LDS, RXH), REFSTACKADDRESS+1, // Load refStack into X
  // get SP from intStack
  asm_const_opcodeWithSingleRegOperand(OPCODE_LDS, R0), INTSTACKADDRESS,   // Load intStackL into R0
  asm_const_OUT(SP_L_port, R0),       // Store R0 into SPL
  asm_const_opcodeWithSingleRegOperand(OPCODE_LDS, R0), INTSTACKADDRESS+1, // Load intStackH into R0
  asm_const_OUT(SP_H_port, R0),       // Store R0 into SPH
  asm_const_POP(R0), // JUST POP AND DISCARD TO CLEAR THE BYTE WE RESERVED IN THE asm_const_PUSH(ZERO_REG) LINE IN PREINVOKE

  asm_const_PUSH(R19),
  asm_const_PUSH(R18),
  OPCODE_RET };
void emit_x_postinvoke() {
    emit_2_CALL(((uint16_t)emit_x_postinvoke_code)/2);
}