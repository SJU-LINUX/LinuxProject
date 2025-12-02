#include "server.h"

void server_main(int server_id, int mq_srv_id) {
    struct msg_cli_server msg;
    char fname[32];
    
    // 시간 측정
    struct timeval t_start, t_recv, t_end;
    double comm_time = 0.0;
    double io_time = 0.0;

    // 각 Client(2명)가 2개의 Block(256개)을 보내고, 각 Block은 8개의 Chunk로 구성됨
    // 총 Chunk 수 = 2 Clients * 2 Blocks * 8 Chunks = 32 Chunks
    int total_chunks = 2 * BLOCKS_PER_CLIENT * CHUNKS_PER_BLOCK;
    int received_chunks = 0;

    sprintf(fname, "server_%d.bin", server_id);
    FILE *fp = fopen(fname, "wb");
    if(!fp) { perror("Server fopen failed"); exit(1); }

    printf("[Server %d] Ready.\n", server_id);

    while(received_chunks < total_chunks) {
        gettimeofday(&t_start, NULL);

        // 메시지 수신 (Payload size 주의: 데이터 + client_id)
        size_t payload_size = sizeof(struct msg_cli_server) - sizeof(long);
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

        received_chunks++;
    }

    fclose(fp);
    printf("======================================\n");
    printf("[Server %d] Comm Time : %.6f sec\n", server_id, comm_time);
    printf("[Server %d] I/O Time  : %.6f sec\n", server_id, io_time);
    printf("======================================\n");
    exit(0);
}