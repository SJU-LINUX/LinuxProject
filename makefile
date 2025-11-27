all: project_run

project_run: main.c common.h
	gcc -o project_run main.c

clean:
	rm -f project_run *.dat

# 데이터 확인을 위한 od 명령어 단축키
check:
	@echo "--- Checking Input Data (First 16 ints) ---"
	od -t d4 -N 64 input_matrix.dat
	@echo "\n--- Checking Client 0 Data (First 16 ints) ---"
	od -t d4 -N 64 client_0_gathered.dat
	@echo "\n--- Checking Server 1 Data (First 16 ints) ---"
	od -t d4 -N 64 server_1_storage.dat