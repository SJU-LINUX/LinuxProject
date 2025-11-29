#include "common.h"

int shm_ids[4];

void init_data() {
    int fd;
    int i, j;
    int data[64][64];
    int n = 0;

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) {
            data[i][j] = n++;
        }
    }

    fd = open("data", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    write(fd, data, sizeof(data));
    close(fd);

    printf("[main] data created\n");
}

int create_msg_queue() {
    int msg_qid;

    msg_qid = msgget(0557, IPC_CREAT | 0666);

    if (msg_qid == -1) {
        perror("msgget");
        exit(1);
    }

    printf("[main] message queue created\n");
    return msg_qid;
}

void create_shm() {
    int i;

    for (i = 0; i < 4; i++) {
        int shm_key = 0600 + i;
        int shm_id = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);

        if (shm_id == -1) {
            perror("shmget");
            exit(1);
        }

        shm_ids[i] = shm_id;

        printf("[main] shm created\n");
    }
}

void partitioning_data(int msg_qid, int partition) {
    int r, c;
    int i;
    struct msgbuf msg;
    int data[64][64];

    FILE* fp = fopen("data", "rb");
    int sm_fd[8];

    for (i = 0; i < 8; i++) {
        char name[32];
        sprintf(name, "sm%d_%dx%d", i, partition, partition);
        sm_fd[i] = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (sm_fd[i] == -1) {
            perror("open sm file");
            exit(1);
        }
    }

    if (!fp) {
        perror("fopen data");
        exit(1);
    }

    fread(data, sizeof(int), 64 * 64, fp);
    fclose(fp);

    if (partition == 8) {
        for (r = 0; r < 64; r++) {
            for (c = 0; c < 64; c++) {
                int sm_id = c / 8;
                msg.mtype = sm_id + 1;
                msg.value = data[r][c];

                if (msgsnd(msg_qid, &msg, sizeof(int), 0) == -1) {
                    perror("msgsnd");
                    exit(1);
                }

                write(sm_fd[sm_id], &data[r][c], sizeof(int));
            }
        }

        printf("[main] 8*8 partitioning data to SM\n");
    }

    else if (partition == 4) {
        for (r = 0; r < 64; r++) {
            for (c = 0; c < 64; c++) {
                int block_row = r / 4;
                int block_col = c / 4;
                int block_id = block_row * 16 + block_col;

                int sm_id = block_id % 8;

                msg.mtype = sm_id + 1;
                msg.value = data[r][c];

                if (msgsnd(msg_qid, &msg, sizeof(int), 0) == -1) {
                    perror("msgsnd");
                    exit(1);
                }

                write(sm_fd[sm_id], &data[r][c], sizeof(int));
            }
        }

        printf("[main] 4*4 partitioning data to SM\n");
    }

    for (i = 0; i < 8; i++) {
        close(sm_fd[i]);
    }
}

void create_servers() {
    int i;

    for (i = 0; i < 4; i++) {
        int pid = fork();

        if (pid == 0) {
            //server_main(i);
            exit(0);
        } else if (pid < 0) {
            perror("fork server");
            exit(1);
        }
    }

    printf("[main] create 4 servers\n");
}

void create_clients(int msg_qid) {
    int i;

    for (i = 0; i < 8; i++) {
        int pid = fork();
        if (pid == 0) {
            //client_main(i, msg_qid);
            exit(0);
        } else if (pid < 0) {
            perror("fork client");
            exit(1);
        }
    }

    printf("[main] create 8 clients(SM)\n");
}

void clean(int msg_qid) {
    int i;

    msgctl(msg_qid, IPC_RMID, NULL);

    for (i = 0; i < 4; i++) {
        shmctl(shm_ids[i], IPC_RMID, NULL);
    }

    printf("[main] all IPC removed\n");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage : %s [8 or 4]\n", argv[0]);
        exit(1);
    }

    /* 8x8 분할, 4x4 분할 결정 */
    int partition;
    partition = atoi(argv[1]);
    if (partition != 8 && partition != 4) {
        printf("partition size can be only 8 or 4\n");
        exit(1);
    }

    int i;

    /* 데이터 생성 */
    init_data();


    /* IPC 자원 생성 */
    // (1) 메시지큐 (Generator -> Client)
    int msg_qid = create_msg_queue();
    create_shm();

    // (2) 공유 메모리 (Client -> Client)


    /* 서버 생성 및 실행 */
    create_servers();


    /* 클라이언트 생성 및 실행 */
    create_clients(msg_qid);


    /* 데이터 분배 */
    partitioning_data(msg_qid, partition);

    for (i = 0; i < 12; i++) {
        wait(NULL);
    }


    /* IPC 자원 할당 해제 */
    clean(msg_qid);

    printf("[main] all done\n");

    return 0;
}