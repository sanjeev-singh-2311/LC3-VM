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
