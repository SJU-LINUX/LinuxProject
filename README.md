# **🚀 Linux IPC Distributed Data Processing Simulator**

- 이 프로젝트는 세종대학교 노재춘 교수님의 *리눅스프로그래밍및실습* 과목의 과제입니다.
- **리눅스 환경에서 대용량 데이터를 멀티 프로세스로 분산 처리(Scatter)하고, 프로세스 간 통신(IPC)을 통해 정렬(Gather/Sort)하는 시뮬레이션 프로젝트입니다.**

<br>

## **📖 목차 (Table of Contents)**

1. 프로젝트 소개 (Introduction)
2. 주요 기능 (Key Features)
3. 시스템 아키텍처 (Architecture)
4. 기술 스택 (Tech Stack)
5. 프로젝트 구조 (Directory Structure) 
6. 시작하기 (Getting Started)
7. 트러블슈팅 & 회고 (Retrospective)

<br>

## **📝 프로젝트 소개 (Introduction)**

이 프로젝트는 GPU나 분산 컴퓨팅 환경에서 발생하는 **데이터 분산 처리 모델**을 리눅스 시스템 프로그래밍으로 모사한 것입니다.

64x64 크기의 대형 매트릭스 데이터를 8개의 클라이언트 프로세스(SM)에 타일링(Tiling) 방식으로 분배하고, 각 클라이언트는 독립적으로 데이터를 수신한 뒤 \*\*프로세스 간 협업(Synchronization)\*\*을 통해 전체 데이터를 순차적으로 정렬하는 과정을 시뮬레이션합니다.

### ✅ **해결하고자 한 문제**

* **동시성 제어:** 다수의 프로세스가 동시에 실행될 때 발생하는 경쟁 상태(Race Condition) 해결  
* **IPC 효율성:** 시스템 커널의 자원 한계(Message Queue Size)를 고려한 데이터 전송 최적화  
* **데이터 일관성:** 분산된 데이터를 공유 메모리를 통해 다시 하나의 연속된 흐름으로 재구성

<br>

## **✨ 주요 기능 (Key Features)**

* **데이터 파티셔닝 (Data Partitioning):** \* 64x64 데이터를 8x8 또는 4x4 블록 단위로 쪼개어 8개의 클라이언트에 분배합니다.  
* **하이브리드 IPC 통신:**  
  * **Generator → Client:** 메시지 큐(Message Queue)를 사용하여 비동기 데이터 전송.  
  * **Client ↔ Client:** 공유 메모리(Shared Memory)를 사용하여 고속 데이터 정렬 및 교환.  
* **프로세스 동기화 (Synchronization):**  
  * 공유 메모리 내 플래그(Flag) 기반의 Barrier를 구현하여 모든 클라이언트의 작업 완료를 보장합니다.  
* **데드락 방지 (Deadlock Prevention):**  
  * 큐 용량 초과를 막기 위한 **배치(Batch) 전송 기법** 적용.  
* **데이터 검증:**  
  * od 명령어를 통해 바이너리 데이터의 무결성을 검증할 수 있습니다.

<br>

## **🏗 시스템 아키텍처 (Architecture)**

    GEN [Main Generator]
        -|Message Queue (Batch)| C1[Client 0]  
        -|Message Queue (Batch)| C2[Client 1...7]  
      
    Inter-Process Communication 
        C1 <-->|Shared Memory (Sorting)| SHM[(Shared Memory)]  
        C2 <-->|Shared Memory (Sorting)| SHM

    C1 -->|File Write| F1[client_sorted_0]  
    C2 -->|File Write| F2[client_sorted_X]

1. **Generator:** 데이터를 생성하고 파티셔닝하여 메시지 큐로 전송 (Batch 단위).  
2. **Client (Recv):** 메시지 큐에서 자신의 할당량을 수신.  
3. **Client (Sort):** 수신한 데이터를 공유 메모리의 절대 주소(Index)에 매핑 (Scatter).  
4. **Synchronization:** 모든 클라이언트가 쓰기를 마칠 때까지 대기 (Barrier).  
5. **Client (Save):** 정렬된 전체 데이터 중 자신의 담당 구간을 읽어(Gather) 파일로 저장.

<br>

## **🛠 기술 스택 (Tech Stack)**

* **Language:** C (Standard C99)  
* **OS:** Linux (Ubuntu/CentOS compatible)  
* **IPC:**  
  * System V Message Queue (msgget, msgsnd, msgrcv)  
  * System V Shared Memory (shmget, shmat)  
* **Process Management:** fork(), wait(), exit()  
* **Build Tool:** Makefile  
* **Debugging:** od (Octal Dump), gdb

<br>

## **📂 프로젝트 구조 (Directory Structure)**

.  
├── Makefile           \# 빌드 자동화 스크립트  
├── common.h           \# IPC 키, 상수, 구조체 정의 (공통 헤더)  
├── main.c             \# 메인 프로세스 (Generator & Process Spawning)  
├── client.h           \# 클라이언트 로직 헤더  
├── client.c           \# 클라이언트 로직 구현 (MQ 수신 및 SHM 정렬)  
├── data               \# (실행 시 생성) 원본 데이터 파일  
└── client\_sorted\_\* \# (실행 시 생성) 결과 데이터 파일

<br>

## **🚀 시작하기 (Getting Started)**

### **1\. 프로젝트 클론 및 이동**
```
git clone \[repository-url\]  
cd \[project-folder\]
```

### **2\. 컴파일 (Build)**

make 명령어를 통해 실행 파일(App)을 생성합니다.
```
make
```

### **3\. 실행 (Run)**

파티션 크기(8 또는 4)를 인자로 주어 실행합니다.
```
./App 8  
\# 또는  
./App 4
```

### **4\. 결과 검증 (Verification)**

생성된 바이너리 파일을 od 명령어로 확인하여 데이터가 정렬되었는지 확인합니다. (-v 옵션 필수)
```
# Client 0이 0\~511까지의 값을 가지고 있는지 확인  
od -t d4 -v client_sorted_0

# Client 1이 512\~1023까지의 값을 가지고 있는지 확인  
od -t d4 -v client_sorted_1
```

### **5\. 청소 (Clean)**
```
make clean  
```