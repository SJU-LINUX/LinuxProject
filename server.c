#include "server.h"

void server_main(int server_id, int mq_srv_id) {
    struct msg_cli_server msg;
    char fname[32];
    
    // 시간 측정
    struct timeval t_start, t_recv, t_end;
    double comm_time = 0.0;
    double io_time = 0.0;

    // Server당 2명의 Client * 16 chunks = 32 messages 수신 대기
    int msg_expected = 2 * NUM_CHUNKS;
    int msg_received = 0;

    sprintf(fname, "server_%d.bin", server_id);
    FILE *fp = fopen(fname, "wb");
    if(!fp) { perror("Server fopen failed"); exit(1); }

    printf("[Server %d] Ready.\n", server_id);

    while(msg_received < msg_expected) {
        gettimeofday(&t_start, NULL);

        // 메시지 수신 (Payload size 주의: 데이터 + client_id)
        size_t payload_size = sizeof(int)*CHUNK_SIZE + sizeof(int);
        if (msgrcv(mq_srv_id, &msg, payload_size, server_id, 0) == -1) {
            perror("Server msgrcv failed");
            exit(1);
        }

        gettimeofday(&t_recv, NULL);
        comm_time += (double)(t_recv.tv_sec - t_start.tv_sec) + 
                     (double)(t_recv.tv_usec - t_start.tv_usec) / 1000000.0;

        // 파일 저장
        fwrite(msg.data, sizeof(int), CHUNK_SIZE, fp);

        gettimeofday(&t_end, NULL);
        io_time += (double)(t_end.tv_sec - t_recv.tv_sec) + 
                   (double)(t_end.tv_usec - t_recv.tv_usec) / 1000000.0;

        msg_received++;
    }

    fclose(fp);
    printf("======================================\n");
    printf("[Server %d] Comm Time : %.6f sec\n", server_id, comm_time);
    printf("[Server %d] I/O Time  : %.6f sec\n", server_id, io_time);
    printf("======================================\n");
    exit(0);
}