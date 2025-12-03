#include "client.h"

void client_main(int client_id, int mq_gen_id, int mq_srv_id) {
    struct msg_gen_client msg_recv;
    struct msg_cli_server msg_send;
    int local_data[INTS_PER_CLIENT];
    int sorted_data[INTS_PER_CLIENT];
    struct timeval t_start, t_end;
    double comm_time = 0.0;
    double send_time = 0.0;

    // 1. Receive from Generator (기존 유지: 32개씩 수신)
    int total_chunks = INTS_PER_CLIENT / CHUNK_SIZE;
    for (int i = 0; i < total_chunks; i++) {
        if (msgrcv(mq_gen_id, &msg_recv, sizeof(struct msg_gen_client) - sizeof(long), client_id + 1, 0) == -1) {
            perror("Client msgrcv"); exit(1);
        }
        memcpy(&local_data[i * CHUNK_SIZE], msg_recv.data, sizeof(int)*CHUNK_SIZE);
    }

    // 2. Sort (SHM) - 기존 유지
    gettimeofday(&t_start, NULL);
    
    int shm_id = shmget(KEY_SHM_SORT, sizeof(SharedSortBuffer), 0666);
    if (shm_id == -1) { perror("Client shmget"); exit(1); }
    SharedSortBuffer *sb = (SharedSortBuffer *)shmat(shm_id, NULL, 0);

    for (int i = 0; i < INTS_PER_CLIENT; i++) {
        int val = local_data[i];
        sb->full_data[val] = val;
    }
    sb->ready_flags[client_id] = 1;

    while(1) {
        int count = 0;
        for(int k=0; k<NUM_CLIENTS; k++) count += sb->ready_flags[k];
        if(count == NUM_CLIENTS) break;
        usleep(100);
    }

    int start_idx = client_id * INTS_PER_CLIENT;
    for (int i = 0; i < INTS_PER_CLIENT; i++) {
        sorted_data[i] = sb->full_data[start_idx + i];
    }
    shmdt(sb);

    gettimeofday(&t_end, NULL);
    comm_time = (double)(t_end.tv_sec - t_start.tv_sec) + 
                (double)(t_end.tv_usec - t_start.tv_usec) / 1000000.0;

    // ---------------------------------------------------
    // Step 3. Server로 데이터 전송 (수정됨)
    // ---------------------------------------------------
    // 256개(1KB)씩 쪼개지 않고 통째로 보냅니다.
    int target_server = (client_id / 2) + 1;
    msg_send.mtype = target_server;
    msg_send.src_client_id = client_id;

    gettimeofday(&t_start, NULL);

    // Block 0 (First 256 ints) & Block 1 (Second 256 ints)
    for (int b = 0; b < BLOCKS_PER_CLIENT; b++) {
        msg_send.block_id = b; 
        
        // [수정] 작은 청크 루프 제거 -> 블록 전체 복사
        int offset = b * BLOCK_SIZE;
        memcpy(msg_send.data, &sorted_data[offset], sizeof(int) * BLOCK_SIZE);
        
        // Payload size: src_client_id + block_id + data[256]
        size_t payload_size = sizeof(struct msg_cli_server) - sizeof(long);
        
        if (msgsnd(mq_srv_id, &msg_send, payload_size, 0) == -1) {
            perror("Client msgsnd"); 
            printf("Error Detail: Size=%lu\n", payload_size);
            exit(1);
        }
        printf("[Client %d] Sent Block %d (256 ints) to Server %d.\n", client_id, b, target_server);
    }

    gettimeofday(&t_end, NULL);
    send_time = (double)(t_end.tv_sec - t_start.tv_sec) + 
                (double)(t_end.tv_usec - t_start.tv_usec) / 1000000.0;



    // 파일 저장
    char fname[32];
    sprintf(fname, "client_sorted_%d", client_id);
    FILE *fp = fopen(fname, "wb");
    fwrite(sorted_data, sizeof(int), INTS_PER_CLIENT, fp);
    fclose(fp);

    printf("[Client %d] Done. Send Time: %.6f sec\n", client_id, send_time);
    printf("[Client %d] Done. Comm Time: %.6f sec\n", client_id, comm_time);
    exit(0);
}