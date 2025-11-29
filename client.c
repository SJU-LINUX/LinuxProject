#include "client.h"

void client_main(int client_id, int msg_qid) {
    struct msgbuf msg;
    int received_data[INTS_PER_CLIENT]; // 섞인 데이터
    int sorted_data[INTS_PER_CLIENT];   // 정렬되어 최종적으로 가질 데이터
    int i;

    // 1. 메시지 큐 수신 (기존 단계)
    if (msgrcv(msg_qid, &msg, sizeof(int) * INTS_PER_CLIENT, client_id + 1, 0) == -1) {
        perror("client msgrcv");
        exit(1);
    }
    for (i = 0; i < INTS_PER_CLIENT; i++) {
        received_data[i] = msg.data[i];
    }
    printf("[Client %d] Received data from MQ.\n", client_id);


    // 2. [추가] 클라이언트 간 통신 (Shared Memory Sorting)
    
    // (A) 정렬용 공유 메모리 접근
    int sort_shm_id = shmget(INTER_CLIENT_SHM_KEY, sizeof(SharedSortBuffer), 0666);
    if (sort_shm_id == -1) { perror("client shmget"); exit(1); }
    
    SharedSortBuffer *sb = (SharedSortBuffer *)shmat(sort_shm_id, NULL, 0);
    if (sb == (void *)-1) { perror("client shmat"); exit(1); }

    // (B) 데이터 재배치 (Scattering to Global Index)
    // 값 자체가 곧 인덱스(0~4095)이므로, 해당 위치에 값을 기록합니다.
    for (i = 0; i < INTS_PER_CLIENT; i++) {
        int val = received_data[i];
        sb->full_data[val] = val; 
    }

    // (C) 완료 플래그 설정 (나는 쓰기 끝났다)
    sb->ready_flags[client_id] = 1;

    // (D) 동기화 (Barrier) - 모든 클라이언트(8명)가 쓸 때까지 대기
    while (1) {
        int ready_count = 0;
        for (int k = 0; k < 8; k++) {
            ready_count += sb->ready_flags[k];
        }
        if (ready_count == 8) break; // 모두 완료되면 탈출
        usleep(1000); // 1ms 대기 (CPU 점유율 방지)
    }

    // (E) 연속된 데이터 가져오기 (Gathering Continuous Block)
    // Client 0: 0~511, Client 1: 512~1023 ...
    int start_index = client_id * INTS_PER_CLIENT;
    for (i = 0; i < INTS_PER_CLIENT; i++) {
        sorted_data[i] = sb->full_data[start_index + i];
    }

    printf("[Client %d] Sorting complete via SHM. Holding %d ~ %d.\n", 
           client_id, start_index, start_index + INTS_PER_CLIENT - 1);

    // 공유 메모리 분리
    shmdt(sb);


    // 3. 파일 저장 (최종 결과)
    char filename[32];
    sprintf(filename, "client_sorted_%d", client_id);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { perror("client file open"); exit(1); }
    
    write(fd, sorted_data, sizeof(int) * INTS_PER_CLIENT);
    close(fd);

    exit(0);
}