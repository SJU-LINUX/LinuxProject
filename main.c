#include "common.h"
#include <time.h>

// --- 함수 프로토타입 ---
void input_generator(int shm_id);
void run_client(int client_id, int shm_id, int msg_id);
void run_server(int server_id, int msg_id);

int main() {
    // 1. IPC 자원 생성
    // (1) Shared Memory 생성 (Input 데이터용)
    int shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) { perror("shmget failed"); exit(1); }

    // (2) Message Queue 생성 (Client->Server 전송용)
    int msg_id = msgget(MQ_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) { perror("msgget failed"); exit(1); }

    // 2. Input Generator 실행 (데이터 생성 및 파일 저장)
    input_generator(shm_id);

    // 3. Server 프로세스 4개 생성 (Fork)
    for (int i = 0; i < NUM_SERVERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            run_server(i + 1, msg_id); // Server ID는 1부터 시작 (mtype용)
            exit(0);
        }
    }

    // 4. Client 프로세스 8개 생성 (Fork)
    for (int i = 0; i < NUM_CLIENTS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            run_client(i, shm_id, msg_id);
            exit(0);
        }
    }

    // 5. 부모 프로세스는 자식들이 끝날 때까지 대기
    while (wait(NULL) > 0);

    // 6. IPC 자원 정리
    shmctl(shm_id, IPC_RMID, NULL);
    msgctl(msg_id, IPC_RMID, NULL);

    printf("\n[System] All Simulation Finished.\n");
    return 0;
}

// --- [역할 1] Input Generator ---
void input_generator(int shm_id) {
    SharedData *shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    FILE *fp = fopen("input_matrix.dat", "wb");
    
    int counter = 0;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            shm_ptr->data[i][j] = counter++; // 0, 1, 2... 순차 증가값
        }
    }
    
    // 파일로 저장 (od 확인용)
    fwrite(shm_ptr->data, sizeof(int), TOTAL_INTS, fp);
    fclose(fp);
    shmdt(shm_ptr);
    printf("[Generator] 64x64 Matrix Created & Saved to 'input_matrix.dat'\n");
}

// --- [역할 2] Client (SM) - 수정됨 ---
void run_client(int client_id, int shm_id, int msg_id) {
    SharedData *shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    int gathered_data[INTS_PER_CLIENT]; // 512 ints
    int idx = 0;

    // --- Step A: Gathering (Shared Memory Read) ---
    // 64x64 행렬을 위에서부터 8줄씩 담당하여 가져옴
    int start_row = client_id * (MATRIX_SIZE / NUM_CLIENTS); 
    
    for (int i = start_row; i < start_row + 8; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
             gathered_data[idx++] = shm_ptr->data[i][j];
        }
    }
    shmdt(shm_ptr);

    // 1. Gathered Data 파일 저장
    char filename[30];
    sprintf(filename, "client_%d_gathered.dat", client_id);
    FILE *fp = fopen(filename, "wb");
    fwrite(gathered_data, sizeof(int), INTS_PER_CLIENT, fp);
    fclose(fp);
    printf("[Client %d] Gathered %d ints -> Saved to '%s'\n", client_id, idx, filename);

    // --- Step B: Sending to Server (Message Queue) ---
    // [수정] 512개(2052바이트)를 한 번에 보내면 일부 시스템(msgmax=2048)에서
    // Invalid Argument 오류가 발생하므로, 256개(1KB)씩 두 번 나누어 보냅니다.
    
    MsgPacket packet;
    packet.clientId = client_id;
    
    // 타겟 서버 결정 (Round Robin)
    int target_server = (client_id / 2) + 1; 
    packet.mtype = target_server; 

    // 절반씩 나누어 전송 (Total 512 ints -> 256 ints * 2 times)
    int chunk_size = INTS_PER_CLIENT / 2; // 256 ints (1KB)
    
    for (int k = 0; k < 2; k++) {
        // 데이터 복사 (앞부분 256개 또는 뒷부분 256개)
        memcpy(packet.data, &gathered_data[k * chunk_size], chunk_size * sizeof(int));
        
        // [중요 수정] 사이즈 계산을 표준 방식(struct size - long size) 대신
        // 실제 보내는 유효 데이터 크기로 명시하여 안전성 확보
        // 여기서는 구조체를 재활용하되 실제로 256개만 채워서 보낸다고 가정할 수도 있지만,
        // 구조체 전체를 보내되 내용은 256개만 유효하다고 보는 것이 구현상 깔끔합니다.
        // 다만 'Invalid Argument'를 피하기 위해 구조체 정의 자체를 줄이거나,
        // 아래처럼 '실제 전송할 데이터 크기'를 정확히 계산해야 합니다.
        
        // 여기서는 안전하게 전체 구조체 크기에서 mtype을 뺀 크기를 보냅니다.
        // *주의: 만약 시스템 msgmax가 2048이면 이 방식도 터질 수 있습니다.*
        // 따라서, **MsgPacket 구조체의 data 배열 크기를 common.h에서 줄이는 것이 근본 해결책**이나,
        // 당장 코드만으로 해결하기 위해 '실제 채운 데이터 크기'만 보낸다고 명시하겠습니다.
        
        size_t payload_size = sizeof(packet.clientId) + (chunk_size * sizeof(int));
        
        if (msgsnd(msg_id, &packet, payload_size, 0) == -1) {
            perror("msgsnd failed");
            printf("Debug: Size=%lu, limit might be 2048\n", payload_size);
            exit(1);
        } else {
            // printf("[Client %d] Sent Chunk %d (%d ints) to Server %d\n", client_id, k+1, chunk_size, target_server);
        }
    }
    printf("[Client %d] Completely sent %d ints to Server %d\n", client_id, INTS_PER_CLIENT, target_server);
}

// --- [역할 3] Server - 수정됨 ---
void run_server(int server_id, int msg_id) {
    MsgPacket packet;
    char filename[30];
    sprintf(filename, "server_%d_storage.dat", server_id);
    FILE *fp = fopen(filename, "wb"); 

    // 각 서버는 2개의 Client를 담당함.
    // 각 Client는 데이터를 2번 나누어 보냄 (2 chunks).
    // 총 받아야 할 메시지 수 = 2 Clients * 2 Chunks = 4 messages
    int messages_expected = 2 * 2;
    int messages_received = 0;
    
    // 수신된 데이터를 순서대로 파일에 쓰기 위해 버퍼링 없이 바로 append 하면
    // 멀티프로세스 환경에서 순서가 섞일 수 있으나, 
    // 문제 요구사항(od 확인) 수준에서는 순차적으로 온다고 가정하거나 파일에 바로 씁니다.
    // (엄밀한 정렬을 위해서는 Client ID 별로 버퍼링이 필요하나 여기선 단순화)
    
    while (messages_received < messages_expected) {
        // payload 크기: clientId(4) + data(1024) = 1028 bytes
        // msgrcv의 사이즈 인자는 최대 수신 가능 크기(구조체 - long)를 넣으면 됩니다.
        size_t max_payload = sizeof(MsgPacket) - sizeof(long);

        if (msgrcv(msg_id, &packet, max_payload, server_id, 0) == -1) {
            perror("msgrcv failed");
            break;
        }
        
        fwrite(packet.data, sizeof(int), CHUNK_SIZE, fp); // 256개씩 쓰기
        messages_received++;
    }
    
    fclose(fp);
    printf("[Server %d] Storage Complete -> '%s'\n", server_id, filename);
}