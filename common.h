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

// --- 상수 정의 ---
#define SHM_SIZE (4096 * sizeof(int))
#define INTS_PER_CLIENT 512  // 64*64 / 8 clients = 512
#define TOTAL_INTS (64 * 64)      // 전체 데이터 크기

#define INTER_CLIENT_SHM_KEY 0x7777

// --- 데이터 구조 ---
// [수정] 데드락 방지를 위해 데이터를 한 번에(Batch) 전송하도록 변경
struct msgbuf {
    long mtype;                 // 메시지 타입 (Client ID + 1)
    int data[INTS_PER_CLIENT];  // 512개 정수 데이터 배열
};

// 클라이언트 간 데이터 교환 및 동기화를 위한 구조체
typedef struct {
    int full_data[TOTAL_INTS];  // 0~4095까지 정렬될 전체 공간
    int ready_flags[8];         // 클라이언트 8명의 작업 완료 여부 (동기화용)
} SharedSortBuffer;

#endif