#include "client.h"

void client_main(int client_id, int mq_gen_id, int mq_srv_id) {
    struct msg_gen_client msg_recv;
    struct msg_cli_server msg_send;
    int local_data[INTS_PER_CLIENT];
    int sorted_data[INTS_PER_CLIENT];
    int i;
    
    // 시간 측정 변수
    struct timeval t_start, t_end;
    double comm_time = 0.0;

    // ---------------------------------------------------
    // Step 1. Generator로부터 데이터 수신 (MQ)
    // ---------------------------------------------------
    for (i = 0; i < NUM_CHUNKS; i++) {
        if (msgrcv(mq_gen_id, &msg_recv, sizeof(int)*CHUNK_SIZE, client_id + 1, 0) == -1) {
            perror("Client msgrcv failed");
            exit(1);
        }
        memcpy(&local_data[i * CHUNK_SIZE], msg_recv.data, sizeof(int)*CHUNK_SIZE);
    }
    printf("[Client %d] Received data from Generator.\n", client_id);


    // ---------------------------------------------------
    // Step 2. Client 간 정렬 (Shared Memory)
    // ---------------------------------------------------
    gettimeofday(&t_start, NULL); // 통신 시간 측정 시작

    // SHM 연결
    int shm_id = shmget(KEY_SHM_SORT, sizeof(SharedSortBuffer), 0666);
    if (shm_id == -1) { perror("Client shmget failed"); exit(1); }
    SharedSortBuffer *sb = (SharedSortBuffer *)shmat(shm_id, NULL, 0);

    // (A) Scattering: 값을 인덱스로 사용하여 배치
    for (i = 0; i < INTS_PER_CLIENT; i++) {
        int val = local_data[i];
        sb->full_data[val] = val;
    }

    // (B) Barrier: 모든 클라이언트가 쓸 때까지 대기
    sb->ready_flags[client_id] = 1;
    while(1) {
        int count = 0;
        for(i=0; i<NUM_CLIENTS; i++) count += sb->ready_flags[i];
        if(count == NUM_CLIENTS) break;
        usleep(100); // CPU 양보
    }

    // (C) Gathering: 내 담당 구역 가져오기
    int start_idx = client_id * INTS_PER_CLIENT;
    for (i = 0; i < INTS_PER_CLIENT; i++) {
        sorted_data[i] = sb->full_data[start_idx + i];
    }
    
    shmdt(sb); // SHM 분리

    gettimeofday(&t_end, NULL); // 통신 시간 측정 종료
    comm_time = (double)(t_end.tv_sec - t_start.tv_sec) + 
                (double)(t_end.tv_usec - t_start.tv_usec) / 1000000.0;


    // ---------------------------------------------------
    // Step 3. Server로 데이터 전송 (MQ)
    // ---------------------------------------------------
    int target_server = client_id % 4 + 1; // 0,4->1, 1,5->2 ...
    
    msg_send.mtype = target_server;
    msg_send.src_client_id = client_id;

    for (i = 0; i < NUM_CHUNKS; i++) {
        memcpy(msg_send.data, &sorted_data[i * CHUNK_SIZE], sizeof(int)*CHUNK_SIZE);
        if (msgsnd(mq_srv_id, &msg_send, sizeof(int)*CHUNK_SIZE + sizeof(int), 0) == -1) {
            perror("Client msgsnd failed");
            exit(1);
        }
    }

    printf("--------------------------------------\n");
    printf("[Client %d] Done. Comm Time: %.6f sec\n", client_id, comm_time);
    printf("--------------------------------------\n");
    

    // 검증용 파일 저장
    char fname[32];
    sprintf(fname, "client_sorted_%d", client_id);
    FILE *fp = fopen(fname, "wb");
    fwrite(sorted_data, sizeof(int), INTS_PER_CLIENT, fp);
    fclose(fp);

    exit(0);
}