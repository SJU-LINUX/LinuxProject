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
#define TOTAL_INTS (MATRIX_SIZE * MATRIX_SIZE) // 4096
#define INTS_PER_CLIENT (TOTAL_INTS / NUM_CLIENTS) // 512

// [안전장치] 메시지 큐 전송 단위 (32 ints = 128 bytes)
// 시스템별 msgmax 제한(2048~8192)을 안전하게 우회하기 위함
#define CHUNK_SIZE 32
#define NUM_CHUNKS (INTS_PER_CLIENT / CHUNK_SIZE) // 16번 전송

// IPC 키값 (충돌 방지를 위한 고유 키)
#define KEY_MQ_GEN_CLIENT  0x1234
#define KEY_SHM_SORT       0x5678
#define KEY_MQ_CLI_SERVER  0x9ABC

// --- 데이터 구조 ---

// 1. Generator -> Client 메시지
struct msg_gen_client {
    long mtype;                 // Target Client ID + 1
    int data[CHUNK_SIZE];       // 조각난 데이터
};

// 2. Client -> Server 메시지
struct msg_cli_server {
    long mtype;                 // Target Server ID
    int src_client_id;          // 보낸 Client ID
    int data[CHUNK_SIZE];       // 조각난 데이터
};

// 3. Client 간 정렬용 공유 메모리
typedef struct {
    int full_data[TOTAL_INTS];  // 전체 데이터 공간 (인덱스 = 값)
    int ready_flags[NUM_CLIENTS]; // 동기화 플래그 (0:진행중, 1:완료)
} SharedSortBuffer;

#endif