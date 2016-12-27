#include <string.h>
#include "panic.h"
#include "asm.h"

#if defined (AOT_STRATEGY_BASELINE)  || defined (AOT_STRATEGY_IMPROVEDPEEPHOLE)
// LEAVING THE OLD COMMENT HERE JUST AS ILLUSTRATION OF WHAT WENT WRONG:
    // ST_XINC and LD_DECX are basically single operand opcodes just like PUSH/POP,
    // so the same optimisation code will work for both stacks as long as we respect
    // the proper sequence of POPs for instructions that get operands from both stacks.
// IT TURNS OUT THAT AROUND DUP INSTRUCTIONS, IT'S NOT GUARANTEED THAT A JVM PUSH TO
// ONE STACK (INT OR REF), IS ALWAYS FOLLOWED BY A POP FROM THE SAME STACK!! FOR EXAMPLE,
// ONE BENCHMARK GENERATED A IDUP2, ADUP SEQUENCE. IF IT'S NOT CERTAIN THE NEXT POP WILL
// ACCESS THE SAME STACK AS THE LAST PUSH, THEN TREATING BOTH OPERATIONS AS THE SAME WILL
// GENERATE BROKEN CODE BECAUSE THE OPTIMISER MAY ELIMINATE A PUSH/POP WHERE BOTH INSTRUCTIONS
// ACCESS DIFFERENT STACKS
#define IS_INT_PUSH(x)          (((x) & 0xFE0F) == OPCODE_PUSH)
#define IS_INT_POP(x)           (((x) & 0xFE0F) == OPCODE_POP)
#define IS_REF_PUSH(x)          (((x) & 0xFE0F) == OPCODE_ST_XINC)
#define IS_REF_POP(x)           (((x) & 0xFE0F) == OPCODE_LD_DECX)
#define IS_MOV(x)               (((x) & 0xFC00) == OPCODE_MOV)
#define IS_MOVW(x)              (((x) & 0xFC00) == OPCODE_MOVW)

// 0000 000d dddd 0000
#define GET_1REG_OPERAND(x)		(((x) & 0x01F0) >> 4)
#define GET_DEST_REG_OPERAND(x)	(((x) & 0x01F0) >> 4)
// 0000 10rd dddd rrrr
#define GET_SRC_REG_OPERAND(x)  (((x) & 0x000F) + (((x) & 0x0200) >> 5))

void rtc_optimise_drop_instruction(uint16_t *instr, uint16_t **code_end) {
    void *address = (void *)instr;

    // avroraPrintStr("rtc_optimise_drop_1_instruction\n\r");
    // avroraPrintPtr(first_instr);
    // avroraPrintPtr(*code_end);
    // avroraPrintPtr(address);
    // avroraPrintPtr((address+4));
    // avroraPrintHex32((((void *)*code_end) - (address+4)));

    memcpy (address, address+2, ((void *)*code_end) - (address+2));
    *code_end -= 1;
}

void rtc_optimise_drop_2_instructions(uint16_t *first_instr, uint16_t **code_end) {
	void *address = (void *)first_instr;

	// avroraPrintStr("rtc_optimise_drop_2_instructions\n\r");
	// avroraPrintPtr(first_instr);
	// avroraPrintPtr(*code_end);
	// avroraPrintPtr(address);
	// avroraPrintPtr((address+4));
	// avroraPrintHex32((((void *)*code_end) - (address+4)));

	memcpy (address, address+4, ((void *)*code_end) - (address+4));
	*code_end -= 2;
}


bool rtc_instruction_fails_for_baseline_pushpop_optimisation(uint16_t instruction, uint16_t target_reg) {
    const uint16_t TWO_REG_OPERAND_MASK = 0xFC00;
    const uint16_t MOVW_MASK            = 0xFF00;

    if (       (instruction & TWO_REG_OPERAND_MASK) == OPCODE_MOV          // 0010 11rd dddd rrrr
        )
    {
        return GET_DEST_REG_OPERAND(instruction) == target_reg
            || GET_SRC_REG_OPERAND(instruction) == target_reg;
    }

    if (       (instruction & MOVW_MASK)            == OPCODE_MOVW         // 0000 0001 dddd rrrr, with d=dest register/2, r=source register/2
        )
    {
        // This will write both to the dest register, and the one after that.
        // The four bits in the instruction are the four most significant bits,
        // and both matching registers are written to, so we clear the lowest
        // bit on target_reg.
        return ((instruction & 0x00F0) >> 3) == (target_reg & 0x001E) // dest
               || ((instruction & 0x000F) << 1) == (target_reg & 0x001E); // src
    }

    return true;
}


bool rtc_instruction_fails_for_improved_pushpop_optimisation(uint16_t instruction, uint16_t target_reg) {
	const uint16_t TWO_REG_OPERAND_MASK = 0xFC00;
    const uint16_t ONE_REG_OPERAND_MASK = 0xFE0F;
    const uint16_t LDD_MASK             = 0xD200;
    const uint16_t IN_MASK              = 0xF800;    
    const uint16_t ADIW_MASK            = 0xFF00;
    const uint16_t LDI_MASK             = 0xF000;
    const uint16_t SBCI_MASK            = 0xF000;
    const uint16_t SUBI_MASK            = 0xF000;
    const uint16_t MOVW_MASK            = 0xFF00;
    const uint16_t SBRC_MASK            = 0xFE08;
    const uint16_t SBRS_MASK            = 0xFE08;
    const uint16_t STD_MASK             = 0xD200;

	if (	   (instruction & TWO_REG_OPERAND_MASK) == OPCODE_ADC          // 0001 11rd dddd rrrr
			|| (instruction & TWO_REG_OPERAND_MASK) == OPCODE_ADD          // 0000 11rd dddd rrrr
            || (instruction & TWO_REG_OPERAND_MASK) == OPCODE_AND          // 0010 00rd dddd rrrr
            || (instruction & TWO_REG_OPERAND_MASK) == OPCODE_CP           // 0001 01rd dddd rrrr
            || (instruction & TWO_REG_OPERAND_MASK) == OPCODE_CPC          // 0000 01rd dddd rrrr
			|| (instruction & TWO_REG_OPERAND_MASK) == OPCODE_EOR          // 0010 01rd dddd rrrr
			|| (instruction & TWO_REG_OPERAND_MASK) == OPCODE_MOV          // 0010 11rd dddd rrrr
			|| (instruction & TWO_REG_OPERAND_MASK) == OPCODE_OR           // 0010 10rd dddd rrrr
			|| (instruction & TWO_REG_OPERAND_MASK) == OPCODE_SBC          // 0000 10rd dddd rrrr
			|| (instruction & TWO_REG_OPERAND_MASK) == OPCODE_SUB          // 0001 10rd dddd rrrr
        )
    {
        return GET_DEST_REG_OPERAND(instruction) == target_reg
            || GET_SRC_REG_OPERAND(instruction) == target_reg;
    }

    if (       (instruction & ONE_REG_OPERAND_MASK) == OPCODE_ASR          // 1001 010d dddd 0101
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_COM          // 1001 010d dddd 0000
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_DEC          // 1001 010d dddd 1010
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_INC          // 1001 010d dddd 0011
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_LD_DECX      // 1001 000d dddd 1110
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_LD_XINC      // 1001 000d dddd 1101
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_LD_Z         // 1000 000d dddd 0000
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_LD_ZINC      // 1001 000d dddd 0001
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_LDS          // 1001 000d dddd 0000 (kkkk kkkk kkkk kkkk)
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_LSR          // 1001 010d dddd 0110
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_PUSH         // 1001 001d dddd 1111
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_POP          // 1001 000d dddd 1111
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_ROR          // 1001 010d dddd 0111
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_ST_DECX      // 1001 001r rrrr 1110
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_ST_XINC      // 1001 001r rrrr 1101
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_ST_Z         // 1000 001r rrrr 0000
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_ST_ZINC      // 1001 001r rrrr 0001
            || (instruction & ONE_REG_OPERAND_MASK) == OPCODE_STS          // 1001 001d dddd 0000
            || (instruction & IN_MASK)              == OPCODE_IN           // 1011 0AAd dddd AAAA
            || (instruction & LDD_MASK)             == OPCODE_LDD          // 10q0 qq0d dddd yqqq
            || (instruction & SBRC_MASK)            == OPCODE_SBRC         // 1111 110r rrrr 0bbb
            || (instruction & SBRS_MASK)            == OPCODE_SBRS         // 1111 111r rrrr 0bbb
            || (instruction & STD_MASK)             == OPCODE_STD          // 10q0 qq1r rrrr yqqq
        )
	{
		return GET_DEST_REG_OPERAND(instruction) == target_reg;
	}

    if (       (instruction & LDI_MASK)             == OPCODE_LDI          // 1110 KKKK dddd KKKK, with K a constant <= 255,d the destination register - 16
            || (instruction & SBCI_MASK)            == OPCODE_SBCI         // 0100 KKKK dddd KKKK, with K a constant <= 255,d the destination register - 16
            || (instruction & SUBI_MASK)            == OPCODE_SUBI         // 0101 KKKK dddd KKKK, with K a constant <= 255,d the destination register - 16
        )
    {
        // dest reg - 16 in                    0000 0000 dddd 0000
        // register we're looking for is in    0000 0000 000d dddd
        // highest bit in these instructions is implicit 1,
        // so we need to add it before comparing.
        return ((instruction & 0x00F0) >> 4) + 0x010 == target_reg;
    }

    if (       (instruction & ADIW_MASK)            == OPCODE_ADIW         // 1001 0110 KKdd KKKK, with d=r24, r26, r28, or r30
        )
    {
        // This one's a bit complicated. The two bits in the instruction
        // are second and third least significant. The high two bits are implicit 1,
        // the lowest implicit 0: 11dd0
        // two bits of dest reg                0000 0000 00dd 0000
        // register we're looking for is in    0000 0000 000d dddd
        // implicit meaning of dest bits,
        // alligned to match target_reg        0000 0000 0001 1dd0
        return ((instruction & 0x0030) >> 3) + 0x0018 == target_reg;
    }

    if (       (instruction & MOVW_MASK)            == OPCODE_MOVW         // 0000 0001 dddd rrrr, with d=dest register/2, r=source register/2
        )
    {
        // This will write both to the dest register, and the one after that.
        // The four bits in the instruction are the four most significant bits,
        // and both matching registers are written to, so we clear the lowest
        // bit on target_reg.
        return ((instruction & 0x00F0) >> 3) == (target_reg & 0x001E) // dest
               || ((instruction & 0x000F) << 1) == (target_reg & 0x001E); // src
    }

    if (       (instruction & TWO_REG_OPERAND_MASK) == OPCODE_MUL          // 1001 11rd dddd rrrr, with d=dest register, r=source register
        )
    {
        // MUL reads two source registers, and always writes to r0:r1
        return target_reg == 0x0000
            || target_reg == 0x0001
            || GET_DEST_REG_OPERAND(instruction) == target_reg
            || GET_SRC_REG_OPERAND(instruction) == target_reg;
    }

    return false;
}

bool rtc_maybe_optimise_push_pop(uint16_t *push_finger, uint16_t *pop_finger, uint16_t **code_end) {
    uint16_t push_reg = GET_1REG_OPERAND(*push_finger);
    uint16_t pop_reg  = GET_1REG_OPERAND(*pop_finger);

    // Can optimise if no instructions between the PUSH and POP write to the POP's target register.
    uint16_t *check_reg_write_finger = push_finger+1;
    while (check_reg_write_finger < pop_finger) {
        uint16_t check_reg_write_finger_instr = *check_reg_write_finger;

        #if defined (AOT_STRATEGY_BASELINE)
        if (rtc_instruction_fails_for_baseline_pushpop_optimisation(check_reg_write_finger_instr, pop_reg)) {
            return false;
        }
        #elif defined (AOT_STRATEGY_IMPROVEDPEEPHOLE)
        if (rtc_instruction_fails_for_improved_pushpop_optimisation(check_reg_write_finger_instr, pop_reg)) {
            // Some instruction inbetween the PUSH and POP writes to the target register, so we can't remove them or change them to a MOV.
            return false;
        }
        #else
        we should never get here
        #endif


        check_reg_write_finger += rtc_is_double_word_instruction(check_reg_write_finger_instr) ? 2 : 1;
    }

    // No conflicting writes inbetween the PUSH and POP, so we can optimise
    if (push_reg == pop_reg) {
        // PUSH and POP use the same register, so we can just remove both.
        rtc_optimise_drop_instruction(pop_finger, code_end); // Do pop first because after push it's address will change
        rtc_optimise_drop_instruction(push_finger, code_end);
    } else {
        // PUSH and POP use different register, so we can change the PUSH to a (cheaper) MOV, and remove the POP completely
        // Remove the POP
        rtc_optimise_drop_instruction(pop_finger, code_end);
        // Change the PUSH to a MOV
        *push_finger = asm_MOV(pop_reg, push_reg);
    }

    return true;
}

// MAKE SURE THIS ONLY GETS CALLED WITH A SINGLE BASIC BLOCK IN THE BUFFER
void rtc_optimise(uint16_t *buffer, uint16_t **code_end) {
    bool found;


    do {
        found = false;
        uint16_t *finger = buffer;

        while (finger < *code_end) {
            uint16_t finger_instr = *finger;

            // Go look for a INT PUSH
            if (IS_INT_PUSH(finger_instr)) {
                // We found a push, now look for the corresponding POP
                uint8_t extra_stack_depth = 0;
                uint16_t *pop_finger = finger+1;

                // Go look for a PUSH
                while (pop_finger < *code_end) {
                    uint16_t pop_finger_instr = *pop_finger;

                    if (IS_INT_PUSH(pop_finger_instr)) {
                        // Going one level deeper
                        extra_stack_depth++;
                    }

                    if (IS_INT_POP(pop_finger_instr)) {
                        if (extra_stack_depth > 0) {
                            // One level up again.
                            extra_stack_depth--;
                        } else {
                            // Corresponding POP found!
                            if (rtc_maybe_optimise_push_pop(finger, pop_finger, code_end)) {
                                found = true;
                            }
                            break; // No point to search further, since we found the corresponding POP already.
                        }
                    }

                    pop_finger += rtc_is_double_word_instruction(pop_finger_instr) ? 2 : 1;
                }
            }
            // Go look for a REF PUSH
            if (IS_REF_PUSH(finger_instr)) {
                // We found a push, now look for the corresponding POP
                uint8_t extra_stack_depth = 0;
                uint16_t *pop_finger = finger+1;

                // Go look for a PUSH
                while (pop_finger < *code_end) {
                    uint16_t pop_finger_instr = *pop_finger;

                    if (IS_REF_PUSH(pop_finger_instr)) {
                        // Going one level deeper
                        extra_stack_depth++;
                    }

                    if (IS_REF_POP(pop_finger_instr)) {
                        if (extra_stack_depth > 0) {
                            // One level up again.
                            extra_stack_depth--;
                        } else {
                            // Corresponding POP found!
                            if (rtc_maybe_optimise_push_pop(finger, pop_finger, code_end)) {
                                found = true;
                            }
                            break; // No point to search further, since we found the corresponding POP already.
                        }
                    }

                    pop_finger += rtc_is_double_word_instruction(pop_finger_instr) ? 2 : 1;
                }
            }
            // 2nd optimisation: MOV x, y; MOV x+1, y+1 -> MOVW x, y
            if (IS_MOV(finger_instr)) {
                if (finger+1 < *code_end) { // Check if there's another instruction in the buffer
                    uint16_t finger_next_instr = *(finger+1);
                    if (IS_MOV(finger_next_instr)) { // Two MOVs, now check the operands
                        uint8_t src1 = GET_SRC_REG_OPERAND(finger_instr);
                        uint8_t src2 = GET_SRC_REG_OPERAND(finger_next_instr);
                        uint8_t dest1 = GET_DEST_REG_OPERAND(finger_instr);
                        uint8_t dest2 = GET_DEST_REG_OPERAND(finger_next_instr);

                        if (src1 % 2 == 0 && dest1 % 2 == 0
                                && src1+1 == src2
                                && dest1+1 == dest2) {
                            // First instr operands are even, second operands are +1 from first. (ex: MOV r18, r20; MOV r19, r21  =>  MOVW r18, r20)
                            rtc_optimise_drop_instruction(finger+1, code_end); // Drop the second MOV
                            *finger = asm_MOVW(dest1, src1); // Change the first MOV to a MOVW to move both registers in 1 instruction
                            found = true;
                        } else if (
                            src2 % 2 == 0 && dest2 % 2 == 0
                                && src2+1 == src1
                                && dest2+1 == dest1) {
                            // Second instr operands are even, first operands are +1 from second. (ex: MOV r19, r21; MOV r18, r20  =>  MOVW r18, r20)
                            rtc_optimise_drop_instruction(finger+1, code_end); // Drop the second MOV
                            *finger = asm_MOVW(dest2, src2); // Change the first MOV to a MOVW to move both registers in 1 instruction
                            found = true;
                        }
                    }
                }
            }
            if (found) {
                break; // Break after optimising one instruction and start over again.
            }

            finger += rtc_is_double_word_instruction(finger_instr) ? 2 : 1;
        }
    } while (found);
}
#endif // defined (AOT_STRATEGY_BASELINE)  || defined (AOT_STRATEGY_IMPROVEDPEEPHOLE)