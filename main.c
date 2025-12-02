#include "common.h"
#include "client.h"
#include "server.h"

// IPC IDs
int id_mq_gen;
int id_mq_srv;
int id_shm_sort;

// 데이터 생성
void init_data() {
    int data[64][64];
    int n = 0;
    for(int i=0; i<64; i++) 
        for(int j=0; j<64; j++) data[i][j] = n++;
    
    int fd = open("data", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd != -1) {
        write(fd, data, sizeof(data));
        close(fd);
    }
    printf("[Main] Data file created.\n");
}

// IPC 초기화 (안전한 제거 후 생성)
void init_ipc() {
    // 1. Generator MQ (main -> Client)
    int temp = msgget(KEY_MQ_GEN_CLIENT, 0666);
    if(temp != -1) msgctl(temp, IPC_RMID, NULL);
    id_mq_gen = msgget(KEY_MQ_GEN_CLIENT, IPC_CREAT | 0666);


    // 2. Server MQ (Client -> Server)
    temp = msgget(KEY_MQ_CLI_SERVER, 0666);
    if(temp != -1) msgctl(temp, IPC_RMID, NULL);
    id_mq_srv = msgget(KEY_MQ_CLI_SERVER, IPC_CREAT | 0666);


    // 3. Sort SHM (Client <-> Client)
    temp = shmget(KEY_SHM_SORT, sizeof(SharedSortBuffer), 0666);
    if(temp != -1) shmctl(temp, IPC_RMID, NULL);
    id_shm_sort = shmget(KEY_SHM_SORT, sizeof(SharedSortBuffer), IPC_CREAT | 0666);
    
    // SHM 초기화 (0으로)
    void *ptr = shmat(id_shm_sort, NULL, 0);
    memset(ptr, 0, sizeof(SharedSortBuffer));
    shmdt(ptr);



    if(id_mq_gen == -1 || id_mq_srv == -1 || id_shm_sort == -1) {
        perror("IPC Init Failed");
        exit(1);
    }
    printf("[Main] IPC Initialized.\n");
}

void clean_ipc() {
    msgctl(id_mq_gen, IPC_RMID, NULL);
    msgctl(id_mq_srv, IPC_RMID, NULL);
    shmctl(id_shm_sort, IPC_RMID, NULL);
    printf("[Main] IPC Cleaned.\n");
}

// Generator 로직 (Partitioning & Sending)
void run_generator(int partition) {
    int data[64][64];
    int buffers[NUM_CLIENTS][INTS_PER_CLIENT];
    int counts[NUM_CLIENTS] = {0,};
    struct msg_gen_client msg;

    // 데이터 읽기
    FILE *fp = fopen("data", "rb");
    if(!fp) { perror("fopen data"); exit(1); }
    fread(data, sizeof(int), 64*64, fp);
    fclose(fp);

    // SM 확인용 파일 열기
    int sm_fd[NUM_CLIENTS];
    for(int i=0; i<NUM_CLIENTS; i++) {
        char name[32];
        sprintf(name, "sm%d_%dx%d", i, partition, partition);
        sm_fd[i] = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }

    // Partitioning
    for(int r=0; r<64; r++) {
        for(int c=0; c<64; c++) {
            int client_id;
            if (partition == 8) client_id = c / 8;
            else client_id = ((r/4)*16 + (c/4)) % 8; // 4x4 logic
            
            int idx = counts[client_id]++;
            buffers[client_id][idx] = data[r][c];
            write(sm_fd[client_id], &data[r][c], sizeof(int));
        }
    }

    // Sending (Chunked)
    printf("[Main] Sending data to clients...\n");
    for(int i=0; i<NUM_CLIENTS; i++) {
        msg.mtype = i + 1;
        for(int k=0; k<NUM_CHUNKS; k++) {
            memcpy(msg.data, &buffers[i][k*CHUNK_SIZE], sizeof(int)*CHUNK_SIZE);
            if(msgsnd(id_mq_gen, &msg, sizeof(int)*CHUNK_SIZE, 0) == -1) {
                perror("Gen msgsnd"); exit(1);
            }
        }
    }
    
    for(int i=0; i<NUM_CLIENTS; i++) close(sm_fd[i]);
}

int main(int argc, char** argv) {
    if(argc != 2) { printf("Usage: %s [8 or 4]\n", argv[0]); exit(1); }
    int partition = atoi(argv[1]);

    init_data();    // 데이터 생성 (64x64)
    init_ipc();     // 통신 기법 설정

    // 1. Server Fork
    for(int i=0; i<NUM_SERVERS; i++) {
        if(fork() == 0) {
            server_main(i+1, id_mq_srv);
            exit(0);
        }
    }

    // 2. Client Fork
    for(int i=0; i<NUM_CLIENTS; i++) {
        if(fork() == 0) {
            client_main(i, id_mq_gen, id_mq_srv);
            exit(0);
        }
    }

    // 3. Generator Start
    usleep(100000); // Wait for clients ready
    run_generator(partition);

    // 4. Wait & Clean
    for(int i=0; i<NUM_CLIENTS + NUM_SERVERS; i++) wait(NULL);
    clean_ipc();
    
    printf("[Main] All Done.\n");
    return 0;
}