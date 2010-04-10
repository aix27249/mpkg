#ifndef SYSCOMMANDS_H_
#define SYSCOMMANDS_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

void runShellCommand(std::string cmd);
void spawn(char* proc, char* args);

#endif

