#include "server.h"
int compare(const void *a,const void *b){
	return (*(int*)a-*(int*)b);
}
void server_main(int server_id, int mq_srv_id) {
    struct msg_cli_server msg;
    char fname[32];
    struct timeval t_start, t_recv, t_end;
    double comm_time = 0.0;
    double io_time = 0.0;
 
    // Server당 2명의 Client * 2 Blocks = 4 Messages 수신 대기
    int total_blocks = 2 * BLOCKS_PER_CLIENT;
    int received_blocks = 0;

    sprintf(fname, "server_%d.bin", server_id);
    FILE *fp = fopen(fname, "wb+");
    if(!fp) { perror("Server fopen"); exit(1); }

    printf("[Server %d] Ready.\n", server_id);

    while(received_blocks < total_blocks) {
        gettimeofday(&t_start, NULL);

        // [수정] Payload size 계산 (256개 정수 포함 크기)
        size_t payload_size = sizeof(struct msg_cli_server) - sizeof(long);

        if (msgrcv(mq_srv_id, &msg, payload_size, server_id, 0) == -1) {
            perror("Server msgrcv failed");
            exit(1);
        }

        gettimeofday(&t_recv, NULL);
        comm_time += (double)(t_recv.tv_sec - t_start.tv_sec) + 
                     (double)(t_recv.tv_usec - t_start.tv_usec) / 1000000.0;

        // [수정] 256개(BLOCK_SIZE) 한번에 쓰기
        fwrite(msg.data, sizeof(int), BLOCK_SIZE, fp);

        gettimeofday(&t_end, NULL);
        io_time += (double)(t_end.tv_sec - t_recv.tv_sec) + 
                   (double)(t_end.tv_usec - t_recv.tv_usec) / 1000000.0;

        received_blocks++;

        printf("[Server %d] Received 256 integers (1KB) from Client %d (Block %d)\n", 
               server_id, msg.src_client_id, msg.block_id);
    }
    //정렬
    int total_ints=INTS_PER_CLIENT*2;
    int *sort_buffer=(int*)malloc(sizeof(int)*total_ints);

    rewind(fp);
    fread(sort_buffer,sizeof(int),total_ints,fp);

    qsort(sort_buffer,total_ints,sizeof(int),compare);
    
    rewind(fp);
    fwrite(sort_buffer,sizeof(int),total_ints,fp);

    free(sort_buffer);    

    fclose(fp);

    printf("======================================\n");
    printf("[Server %d] Comm Time : %.6f sec\n", server_id, comm_time);
    printf("[Server %d] I/O Time  : %.6f sec\n", server_id, io_time);
    printf("======================================\n");
    exit(0);
}


/*
1. 받을 때 데잍터들 정렬될 것
2. 시간 계산 ()
3. 서버가 받아서 정렬  send()+recv()
*/
