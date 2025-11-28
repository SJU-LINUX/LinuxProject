#ifndef COMMON_H
#define COMMON_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// constant

#define SHM_SIZE (4096 * sizeof(int))

// data structure

struct msgbuf {
    long mtype;
    int value;
};

#endif