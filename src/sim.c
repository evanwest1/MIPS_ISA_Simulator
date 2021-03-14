//Evan West (emw2zf)

// This is the only file that we are allowed to change!

#include <stdio.h>
#include "shell.h"
#define OP_SPECIAL    0x00 //Must be for R-type instructions
#define SUBOP_ADD     0x20
#define SUBOP_ADDU    0x21
#define OP_ADDI       0x08
#define OP_ADDIU      0x09
#define SUBOP_SYSCALL 0xc
uint32_t dcd_op;     /* decoded opcode */
uint32_t dcd_rs;     /* decoded rs operand */
uint32_t dcd_rt;     /* decoded rt operand */
uint32_t dcd_rd;     /* decoded rd operand */
uint32_t dcd_shamt;  /* decoded shift amount */
uint32_t dcd_funct;  /* decoded function */
uint32_t dcd_imm;    /* decoded immediate value */
uint32_t dcd_target; /* decoded target address */
int      dcd_se_imm; /* decoded sign-extended immediate value */
uint32_t inst;       /* machine instruction */

uint32_t sign_extend_h2w(uint16_t c) // Is this an arithmetic or zero sign-extension?
{
    return (c & 0x8000) ? (c | 0xffff8000) : c;
}
void fetch()
{
    /* fetch the 4 bytes of the current instruction */
    inst = mem_read_32(CURRENT_STATE.PC);
}

void decode()
{
    /* decoding an instruction */
    dcd_op     = (inst >> 26) & 0x3F; /* The hex values mask the unneccesary bits and extract the specified part of the instruction*/
    dcd_rs     = (inst >> 21) & 0x1F;
    dcd_rt     = (inst >> 16) & 0x1F;
    dcd_rd     = (inst >> 11) & 0x1F;
    dcd_shamt  = (inst >> 6) & 0x1F;
    dcd_funct  = (inst >> 0) & 0x3F;
    dcd_imm    = (inst >> 0) & 0xFFFF;
    dcd_se_imm = sign_extend_h2w(dcd_imm); //se means "sign-extended immediate"
    dcd_target = (inst >> 0) & ((1UL << 26) - 1);
}

void execute()
{
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    CURRENT_STATE.REGS[0] = 0; // Not sure why this line is necessary. Can't think of what instructions would ever change R0

    switch (dcd_op) //Only R-type instructions have a func field
    {
        case OP_SPECIAL: // This must be all the R-type instructions since all R-type operations have an opcode of 0x00(OP_SPECIAL) 
            switch (dcd_funct) //Kind of chooses a sub operation of the R-type instructions
            {
				case SUBOP_ADD:
				case SUBOP_ADDU:
					NEXT_STATE.REGS[dcd_rd] = CURRENT_STATE.REGS[dcd_rs] + CURRENT_STATE.REGS[dcd_rt];
					break;

				case 0x22: // SUB - 100010 - 0x22 - R-type
				/*The contents of general register rt are subtracted from the contents of
				general register rs to form a result. The result is placed into general
				register rd. In 64-bit mode, the operands must be valid sign-extended, 32-
				bit values.*/
					NEXT_STATE.REGS[dcd_rd] = CURRENT_STATE.REGS[dcd_rs] - CURRENT_STATE.REGS[dcd_rt];
					break;

				case 0x23: // SUBU - 100011 - 0x23 - R-type
				/*The contents of general register rt are subtracted from the contents of
				general register rs to form a result.  
				The result is placed into general register rd. The only difference between SUB is SUBU never traps on overflow*/
					NEXT_STATE.REGS[dcd_rd] = CURRENT_STATE.REGS[dcd_rs] - CURRENT_STATE.REGS[dcd_rt];
					break;

				case 0x00: // SLL - 000000 - 0x00 - R-type
				/*The contents of general register rt are shifted left by sa bits, inserting zeros
				into the low-order bits.
				The result is placed in register rd*/
					NEXT_STATE.REGS[dcd_rd] = (CURRENT_STATE.REGS[dcd_rt] << CURRENT_STATE.REGS[dcd_shamt]);
					break;

				case 0x02: // SRL - 000010 - 0x02 - R-type
				/*The contents of general register rt are shifted right by sa bits, inserting
				zeros into the high-order bits.
				The result is placed in register rd.*/
					NEXT_STATE.REGS[dcd_rd] = (CURRENT_STATE.REGS[dcd_rt] >> CURRENT_STATE.REGS[dcd_shamt]); // When value of dcd_rt is unsigned, >> performs a logical right shift
					break;

				case 0x03: // SRA - 000011 - 0x03 - R-type
				/*The contents of general register rt are shifted right by sa bits, signextending the high-order bits.
				The result is placed in register rd.*/
					NEXT_STATE.REGS[dcd_rd] = (CURRENT_STATE.REGS[dcd_rt] >> CURRENT_STATE.REGS[dcd_shamt]); // When value of dcd_rt is signed, >> performs an arithmetic right shift
					break;

				case 0x19: // MULTU - 011001 - 0x19 - R-type
				/*The contents of general register rs and the contents of general register rt
				are multiplied, treating both operands as unsigned values. No overflow
				exception occurs under any circumstances

				When the operation completes, the low-order word of the double result is
				loaded into special register LO, and the high-order word of the double
				result is loaded into special register HI.*/
					NEXT_STATE.HI = (CURRENT_STATE.REGS[dcd_rs] * CURRENT_STATE.REGS[dcd_rt]) >> 32; //HOW TO ISOLATE THE TOP-ORDER BITS IN HI AND LOW-ORDER BITS IN LO? SHIFTS AND MASKS MAYBE?
					NEXT_STATE.LO = CURRENT_STATE.REGS[dcd_rs] * CURRENT_STATE.REGS[dcd_rt];
					break;

				case 0x: // DIVU - 011011 -0x1B - R-type
				/*The contents of general register rs are divided by the contents of general
				register rt, treating both operands as unsigned values. No integer
				overflow exception occurs under any circumstances, and the result of this
				operation is undefined when the divisor is zero.*/
					NEXT_STATE.HI = (CURRENT_STATE.REGS[dcd_rs] / CURRENT_STATE.REGS[dcd_rt]) >> 32; //HOW TO ISOLATE THE TOP-ORDER BITS IN HI AND LOW-ORDER BITS IN LO? SHIFTS AND MASKS MAYBE?
					NEXT_STATE.LO = CURRENT_STATE.REGS[dcd_rs] / CURRENT_STATE.REGS[dcd_rt];
					break;

				case 0x0B: // SLTU - 101011 - 0x0B - R-type
				/*The contents of general register rt are subtracted from the contents of
				general registerrs. Considering both quantities as unsigned integers, if the
				contents of general register rs are less than the contents of general register
				rt, the result is set to one; otherwise the result is set to zero.

				The result is placed into general register rd. No integer overflow exception occurs
				under any circumstances. The comparison is valid even if the subtraction used during the comparison
				overflows*/
					NEXT_STATE.REGS[dcd_rd] = (CURRENT_STATE.REGS[dcd_rs] - CURRENT_STATE.REGS[dcd_rt]) < 0 ? 1 : 0;
					break;

				case 0x10: //MFHI - "Move from Hi" - 010000 - 0x10 - R-type
				/*The contents of special register HI are loaded into general register rd*/
					NEXT_STATE.REGS[dcd_rd] = CURRENT_STATE.HI;
					break;
            }
		
        case 0x0D: // ORI - 001101 - 0x0D - I-type
            /*The 16-bit immediate is zero-extended and combined with the contents of
            general register rs in a bit-wise logical OR operation. The result is placed
            into general register rt*/
            NEXT_STATE.REGS[dcd_rt] = CURRENT_STATE.REGS[dcd_rs] | dcd_se_imm;
            break;
		
		case SUBOP_SYSCALL:
			if (CURRENT_STATE.REGS[2] == 10) // If a SYSCALL instruction is executed when the value in $v0 (register R2) == 0x0A (10 in decimal)
				RUN_BIT = 0; // This line halts the simulator
			break;

        case OP_ADDI: //Not sure why the skeleton code didn't come with an implementation for this case...
        case OP_ADDIU:
            NEXT_STATE.REGS[dcd_rt] = CURRENT_STATE.REGS[dcd_rs] + dcd_se_imm; //This line is probably helpful in process_instruction(). Does dcd_r* hold reg value or REG index?
            break;
    }

    CURRENT_STATE.REGS[0] = 0;
}


void process_instruction()
{
   fetch();
   decode();
   execute();
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
   
}
