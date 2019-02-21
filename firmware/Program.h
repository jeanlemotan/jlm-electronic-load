#pragma once

#include <cstdint>
#include <cstring>

void compileProgram(const char* code);
void runProgram(size_t index);
void stopProgram();
void updateProgram();
bool isRunningProgram();