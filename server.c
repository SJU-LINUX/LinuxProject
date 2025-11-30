#include "server.h"
#include <sys/time.h>
#include "common.h"

void server_main(int server_id) {
    // 변수 선언
    int shm_key, shm_id;
    ServerShm *shm_ptr;
    char filename[32];
    FILE *fp;
    int clients_done;
    struct timeval start, end;
    double IO_time;

    // 1. 공유 메모리 연결 (Key: 0600 + server_id)
    shm_key = 0600 + server_id;
    shm_id = shmget(shm_key, SHM_SIZE, 0666);
    if (shm_id == -1) {
        perror("server shmget");
        exit(1);
    }

    shm_ptr = (ServerShm *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("server shmat");
        exit(1);
    }

    // 공유 메모리 플래그 초기화
    shm_ptr->flags[0] = 0;
    shm_ptr->flags[1] = 0;

    // 2. 저장 파일 열기
    sprintf(filename, "server_%d.bin", server_id);
    fp = fopen(filename, "wb");
    if (!fp) {
        perror("server file");
        exit(1);
    }

    // 3. 데이터 수신 및 저장
    clients_done = 0;
    gettimeofday(&start, NULL);

    while (clients_done < 2) {
        for (int i = 0; i < 2; i++) {
            if (shm_ptr->flags[i] == 1) {
                // 데이터 저장
                fwrite(shm_ptr->data[i], sizeof(int), INTS_PER_CLIENT, fp);

                // 처리 완료 표시
                shm_ptr->flags[i] = 2;
                clients_done++;
            }
        }

        // CPU 과부하 방지
        usleep(1000);
    }

    gettimeofday(&end, NULL);
    //server io time
    IO_time = (double)(end.tv_sec - start.tv_sec) +
              (double)(end.tv_usec - start.tv_usec) / 1000000.0;

    printf("[Server %d] Storage I/O Time: %.6f sec\n",
           server_id, IO_time);


    fclose(fp);
    shmdt(shm_ptr);
}
