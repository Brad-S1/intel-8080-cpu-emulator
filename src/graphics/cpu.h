#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEMORY_SIZE 0x10000 // 64KB (8080 has 16-bit memory bus)

// structure for 8080 processor condition flags (status bits)
typedef struct ConditionCodes {
  uint8_t   z:1;    // zero
  uint8_t   s:1;    // sign
  uint8_t   p:1;    // parity
  uint8_t   cy:1;   // carry
  uint8_t   ac:1;   // auxillary carry
  uint8_t   pad:3;  // unused bits 
} ConditionCodes;

// structure for 8080 CPU register state
typedef struct State8080 {
  uint8_t   a;                    // accumulator
  uint8_t   b;                    // general purpose registers (b through l)
  uint8_t   c;
  uint8_t   d;
  uint8_t   e;
  uint8_t   h;
  uint8_t   l;
  uint16_t  sp;                   // stack pointer
  uint16_t  pc;                   // program counter
  uint8_t   *memory;              // pointer to memory
  struct    ConditionCodes  cc;   // flag register
  uint8_t   int_enable;           // interrupt enable 
} State8080;

#endif  // CPU_H