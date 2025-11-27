#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

// --- 상수 정의 ---
#define NUM_CLIENTS 8
#define NUM_SERVERS 4
#define MATRIX_SIZE 64 // 64x64
#define TOTAL_INTS (MATRIX_SIZE * MATRIX_SIZE) // 4096
#define INTS_PER_CLIENT (TOTAL_INTS / NUM_CLIENTS) // 512 ints
#define CHUNK_SIZE (INTS_PER_CLIENT / 2)

// Shared Memory & Message Queue Keys (임의 설정)
#define SHM_KEY 0x1234
#define MQ_KEY 0x5678

// --- 데이터 구조 ---

// 1. Shared Memory에 올라갈 원본 데이터 (64x64)
typedef struct {
    int data[MATRIX_SIZE][MATRIX_SIZE];
} SharedData;

// 2. Message Queue로 보낼 메시지 구조체
// Client -> Server 전송용 (Chunk 단위)
typedef struct {
    long mtype; 
    int clientId; 
    int data[CHUNK_SIZE]; // [수정] 512 -> 256
} MsgPacket;

#endif