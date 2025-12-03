#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

// --- 상수 정의 ---
#define NUM_CLIENTS 8
#define NUM_SERVERS 4
#define MATRIX_SIZE 64
#define TOTAL_INTS (MATRIX_SIZE * MATRIX_SIZE)
#define INTS_PER_CLIENT (TOTAL_INTS / NUM_CLIENTS) // 512

// 요구사항: Server는 256개씩 받음
#define BLOCK_SIZE 256 
#define BLOCKS_PER_CLIENT (INTS_PER_CLIENT / BLOCK_SIZE) // 2 blocks

// Generator -> Client 전송용 작은 청크 (안전성 유지)
#define CHUNK_SIZE 32
#define NUM_CHUNKS (INTS_PER_CLIENT / CHUNK_SIZE) 

// IPC Keys
#define KEY_MQ_GEN_CLIENT  0x1234
#define KEY_SHM_SORT       0x5678
#define KEY_MQ_CLI_SERVER  0x9ABC

// --- 데이터 구조 ---

// 1. Generator -> Client 메시지 (기존 유지: 작은 청크로 안전하게 전송)
struct msg_gen_client {
    long mtype;
    int data[CHUNK_SIZE];
};

// 2. Client -> Server 메시지 (수정: 256개 블록 단위 전송)
struct msg_cli_server {
    long mtype;                 
    int src_client_id;          
    int block_id;               
    int data[BLOCK_SIZE]; // [수정] 32 -> 256 (1KB 데이터를 한 번에 담음)
};

// 3. Client 간 정렬용 공유 메모리
typedef struct {
    int full_data[TOTAL_INTS];
    int ready_flags[NUM_CLIENTS];
} SharedSortBuffer;

#endif