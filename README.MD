
## 🚀 실행 및 검증 방법
### 1. 컴파일 및 실행:
```Bash
make
./project_run
```
- 화면에 [Generator], [Client], [Server] 로그가 찍히며 파일들이 생성됩니다.

### 2.  데이터 검증 (od 명령어):
요구사항에 있는 od 확인을 쉽게 하기 위해 Makefile에 타겟을 넣어뒀습니다.
```Bash
make check
```
- `input_matrix.dat`: 0, 1, 2... 순서대로 보일 것입니다.
- `client_0_gathered.dat`: Client 0이 가져간 데이터(0~511)가 연속되어 있는지 확인합니다.
- `server_1_storage.dat`: Server 1이 저장한 데이터가 정상적으로 들어있는지 확인합니다.

## 💡 교수님 요구사항 충족 포인트 요약
### 1. 통신기법 이원화:
- Client 내부 수집(Gather): shmget/shmat (Shared Memory)
- Client $\rightarrow$ Server 전송: msgget/msgsnd (Message Queue)
### 2. 데이터 크기:
- Input: 64x64
- Client 처리: 512 Ints (8 * 64)
### 3. 결과물:
-  *.dat 파일로 모두 저장되며 od로 확인 가능.




## ✅ 검증 방법
### 1. Client 데이터 검증 (요구사항 1)
각 Client가 흩어진 데이터를 잘 모았는지(Gather) 확인합니다.
- 검증 포인트: client_0은 0부터 숫자가 시작되어야 하고, client_1은 그 뒷번호(512)부터 시작되어야 합니다.

    ```Bash

    # Client 0의 데이터 확인 (처음 32개 정수 출력) -> 0, 1, 2... 순서여야 함
    echo "[Client 0 Data Head]"
    od -t d4 -N 128 -v client_0_gathered.dat

    # Client 1의 데이터 확인 (처음 32개 정수 출력) -> 512, 513... 순서여야 함
    echo "[Client 1 Data Head]"
    od -t d4 -N 128 -v client_1_gathered.dat
    ```
    - -t d4: 4바이트 10진수 정수로 출력
    - -N 128: 파일의 처음 128바이트(정수 32개)만 출력 (화면 도배 방지)
    - -v: 중복된 데이터가 있어도 생략하지 않고 모두 출력

### 2. Server 데이터 검증 (요구사항 2)
서버가 데이터를 잘 받아서 파일로 저장했는지 확인합니다.
- 검증 포인트: Server 1은 Client 0과 Client 1의 데이터를 저장하므로, 총 1024개의 정수가 있어야 하며, 0~1023까지의 숫자가 보여야 합니다.
- 주의: 멀티 프로세스 환경이라 Client 0과 1 중 누가 먼저 도착하느냐에 따라 저장 순서가 [0~511, 512~1023] 일 수도 있고 [512~1023, 0~511] 일 수도 있습니다.

    ```Bash
    # Server 1의 데이터 확인 (앞부분) -> 0 또는 512로 시작
    echo "[Server 1 Storage Head]"
    od -t d4 -N 128 -v server_1_storage.dat

    # Server 1의 데이터 확인 (뒷부분 확인을 위해 전체 덤프 후 tail)
    echo "[Server 1 Storage Tail]"
    od -t d4 -v server_1_storage.dat | tail -n 5
    ```