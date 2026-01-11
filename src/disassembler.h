#ifndef DISASM_INCLUDE
#define DISASM_INCLUDE
#include <stdint.h>
#include <stdbool.h>

// buf_len should be 32 bytes
bool disasm_instr(uint32_t instr, char *buf, size_t buf_len, uint16_t pc);

#endif // DISASM_INCLUDE
