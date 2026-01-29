#ifndef COMMAND_H
#define COMMAND_H

#include "common.h"

typedef void (*command_func_t)(const char* args);

void command_init(void);
void command_execute(const char* line);

#endif
