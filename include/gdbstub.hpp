#include <unistd.h>
#include <cstdint>
#include "../include/cpu.h"

void GDB_Create(HART* hart);
void GDB_Stop();
void GDB_Loop();
void GDB_EBREAK();