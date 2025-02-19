#include <signal.h>
#include <stdint.h>
#include <stdio.h>
/* unix only */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
/* LC-3 has 2^16 memory locations having 16 bits each */
#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; /* Represents the Memory */

/* There are 10 registers with  */
/* 8 general purpose -> R0 to R7 */
/* 1 Program Counter */
/* 1 Conditional Flag */
enum {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,   /* Program Counter */
    R_COND, /* Conditional Flag */
    R_COUNT /* Number of Registers */
};
uint16_t reg[R_COUNT];

/* LC-3 has 16 opcodes */
/* each instruction is 16 bit long */
/* the first 4 bits (log2(16)) is the opcode */
/* the other 12 bits are the parameters */
enum {
    OP_BR = 0, /* Branch Operation */
    OP_ADD,    /* Add Operations */
    OP_LD,     /* Load Operations */
    OP_ST,     /* Store Operations */
    OP_JSR,    /* Jump Register Operations */
    OP_AND,    /* Bitwise And Operations */
    OP_LDR,    /*  Load Register Operations */
    OP_STR,    /*  Store Register Operations */
    OP_RTI,    /*  Unused Operations */
    OP_NOT,    /* Bitwise Not Operation */
    OP_LDI,    /* Load Indirect */
    OP_STI,    /* Store Indirect */
    OP_JMP,    /* Jump */
    OP_RES,    /* Reserved (unused) */
    OP_LEA,    /* Load Effective Address */
    OP_TRAP,   /* Execute Trap */
};

/* LC-3 uses flags to store result of previous calculation */
/* it has 3 flags to store the sign of the previous result */
enum {
    FL_POS = (1 << 0), /* Positive */
    FL_ZRO = (1 << 1), /* Zero */
    FL_NEG = (1 << 2)  /* Negative */
};

/* The different type of trap routines available on LC-3 */
enum {
    TRAP_GETC  = 0x20, /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT   = 0x21, /* output a character */
    TRAP_PUTS  = 0x22, /* output a word string */
    TRAP_IN    = 0x23, /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT  = 0x25  /* halt the program */
};

/* Sign extend function */
/* Used to extend lesser bit numbers to higher bits (16) */
/* Achieved by padding Positives with 0 and Negatives with 1 to their left */
uint16_t sign_extend(uint16_t x, int bit_count) {
    if (x >> (bit_count - 1) & 1) {
        // If sign bit is 1 -> negative
        x |= (0xFFFF << bit_count);
    }
    return x;
}

/* Function to update flag based on the register */
void update_flags(uint16_t r) {
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    } else if (reg[r] >> 15) {
        // Sign bit is 1
        reg[R_COND] = FL_NEG;
    } else {
        reg[R_COND] = FL_POS;
    }
}

int main(int argc, char* argv[]) {
    /* Handle command line inputs */
    if (argc < 2) {
        printf("lc3 [image-file1]...\n");
        exit(2);
    }

    for (int j = 1; j < argc; j++) {
        if (!read_image(argv[j])) {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    /* One flag should always be set, so set FL_ZRO */
    reg[R_COND] = FL_ZRO;

    /* Set PC to start position, usually 0x3000 */
    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;

    int running = 1;
    while (running) {
        /* FETCH */
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op    = instr >> 12;

        /* Implement all the OPCODES */
        switch (op) {
            case OP_ADD:
                /* ADD */
                /* Produces the sum of two SR or one SR and one indirect address value and stores it
                 * in DR */
                /* IF bit 5 if 0 in the instruction, second operand is from SR2  */
                /* IF bit 5 is 1, the operand is obtained from immediate address */
                {
                    // Destination register
                    uint16_t r0 = (instr >> 9) & 0x7; // 9 till 11 bit
                    // first operand (SR1)
                    uint16_t r1 = (instr >> 6) & 0x7; // 6 till 8 bit
                    // Check if mode is immediate or register
                    uint16_t imm_flag = (instr >> 5) & 0x1; // 5th bit

                    if (imm_flag) {
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0]       = reg[r1] + imm5;
                    } else {
                        uint16_t r2 = (instr & 0x7);
                        reg[r0]     = reg[r1] + reg[r2];
                    }
                    update_flags(r0);
                }
                break;
            case OP_AND:
                /* BITWISE AND */
                /* Produces the sum of two SR or one SR and one indirect address into DR */
                /* Similar to ADD, bit 5 is checked */
                {
                    // Destination Register
                    uint16_t r0 = (instr >> 9) & 0x7;
                    // first operand
                    uint16_t r1 = (instr >> 6) & 0x7;
                    // Check if mode if register or indirect
                    uint16_t imm_flag = (instr >> 5) & 0x1;
                    if (imm_flag) {
                        uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                        reg[r0]       = reg[r1] + imm5;
                    } else {
                        uint16_t r2 = (instr & 0x7);
                        reg[r0]     = reg[r1] + reg[r2];
                    }
                    update_flags(r0);
                }
                break;
            case OP_NOT:
                /* BITWISE NOT */
                /* Produces the 1's complement to imput from SR */
                {
                    // Destination Register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // Source Register
                    uint16_t r1 = (instr >> 6) & 0x7;

                    reg[r0] = ~reg[r1];
                    update_flags(r0);
                }

                break;
            case OP_BR:
                /* Branch Operation */
                /* Takes a n, p and z bit signifying the branch condition */
                /* If the branch logic is correct, moves the PC to the PC + PCoffset9 */
                {
                    // The 9 bit PC Offset
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    // The Conditional Flag result of 3 bit as npz
                    uint16_t cond = (instr >> 9) & 0x7;

                    if (cond & reg[R_COND])
                        reg[R_PC] = reg[R_PC] + pc_offset;
                }
                break;
            case OP_JMP:
                /* Jump Instruction */
                /* Moves the PC to the given value in the base register number bit 6 to 8 */
                /* Acts as RETURN if the given bits are 111 */
                {
                    uint16_t r0 = (instr >> 6) & 0x7;
                    reg[R_PC]   = reg[r0];
                }
                break;
            case OP_JSR:
                /* Jump Register Instruction */
                /* First store the value of PC in R7; */
                /* Based on a flag value, either increment the value in PC by a PCoffset11
                 * or replace it with value in another register */
                {
                    uint16_t flag = (instr >> 11) & 1;
                    reg[R_R7]     = reg[R_PC];
                    if (flag) {
                        reg[R_PC] += sign_extend(instr & 0x7FF, 11);
                    } else {
                        uint16_t r0 = (instr >> 6) & 0x7;
                        reg[R_PC]   = reg[r0];
                    }
                }
                break;
            case OP_LD:
                /* Load Operation Takes a DR and offset */
                /* Loads into DR from MEMORY[PC_VALUE + offset] */
                {
                    uint16_t r0        = (instr >> 9) & 0x7;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    reg[r0] = mem_read(reg[R_PC] + pc_offset);
                    update_flags(r0);
                }
                break;
            case OP_LDI:
                /* Load Indirect */
                /* Takes a DR and a PCoffset9 of 9 bits */
                /* The value is taken from make memory location stored in register number R_PC +
                 * PCoffset9 and put into DR
                 */
                {
                    uint16_t r0        = (instr >> 9) & 0x7; // Bits 9 till 11;
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                    update_flags(r0);
                }
                break;
            case OP_LDR:
                /* Load Register */
                /* Loads the value from a loction in memory into a DR */
                /* The loction is calculated as MEMORY[SOME_BASE_REGISTER + some_offset] */
                {
                    // Destination Register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // Source Register
                    uint16_t r1 = (instr >> 6) & 0x7;

                    // the offset of 6 bits
                    uint16_t offset = sign_extend(instr & 0x3F, 6);

                    reg[r0] = mem_read(reg[r1] + offset);
                    update_flags(r0);
                }
                break;
            case OP_LEA:
                /* Load effective address */
                /* Loads value from memory in DR from loaction PCoffset9 */
                {
                    // Destination Register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // offset
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    reg[r0] = pc_offset;
                    update_flags(r0);
                }
                break;
            case OP_ST:
                /* Store instruction */
                /* Takes value from a SR and stores it directly in memory location PC + offset */
                {
                    // Source Register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // PCoffset9
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    mem_write(reg[R_PC] + pc_offset, reg[r0]);
                }
                break;
            case OP_STI:
                /* Store Indirect Instruction */
                /* Gains the address from other  address and stores the SR there */
                {
                    // Source Register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // PCoffset9
                    uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);

                    mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
                }
                break;
            case OP_STR:
                /* Store Register Instruction */
                /* Takes value from a Source register and stores it in memory location  */
                /* BASE_REGISTER + offset6 */
                {
                    // Source Register
                    uint16_t r0 = (instr >> 9) & 0x7;

                    // Base register
                    uint16_t r1 = (instr >> 6) & 0x7;

                    // offset6
                    uint16_t offset = sign_extend(instr & 0x3F, 6);

                    mem_write(reg[r1] + offset, reg[r0]);
                }
                break;
            case OP_TRAP:
                /* The TRAP operation or the TRAP routine allows the program to  */
                /* stop what it is doing and go for another work like user input */
                /* The content of PC are saved to Register 7 so the RET operation can be used */
                {
                    reg[R_R7] = reg[R_PC];
                    // The trap opcode has a 8 bit (0 to 7) trapvect8 that
                    // specifies the type of TRAP routine, so we make a switch case for that too

                    switch (instr & 0xFF) {
                        case TRAP_GETC:
                            break;
                        case TRAP_OUT:
                            break;
                        case TRAP_PUTS:
                            break;
                        case TRAP_IN:
                            break;
                        case TRAP_PUTSP:
                            break;
                        case TRAP_HALT:
                            break;
                    }
                }
                break;
            case OP_RES:
            case OP_RTI:
            default:
                /* bad opcode */
                abort();
                break;
        }
    }
}
