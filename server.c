#include "server.h"
#include <sys/time.h>
#include "common.h"

void server_main(int server_id){
        int msgid,clients_done=0;
        char filename[32];
        FILE *fp;
        struct msgbuf mesg;
        struct timeval start,end;
        double current_time,total_time=0;//경과 시간

        if((msgid=msgget(Client_to_Server_MSG_KEY,0666))==-1){
                perror("server msgget");
                exit(1);
        }
        //server_x.bin파일 열기
        sprintf(filename,"server_%d.bin",server_id);
        if(!(fp=fopen(filename,"wb"))){
                perror("server file fopen");
                exit(1);
        }


        //담당하는 2개의 클라이언트로부터 메세지다  받을 동안 루프
        while(clients_done<2){
                //mtype=server_id+1
                if (msgrcv(msgid,&mesg,sizeof(int)*INTS_PER_CLIENT,server_id+1,0)==-1){
                        perror("server msgrcv");
                        exit(1);
                }
                //server I/O 시간 측정
                gettimeofday(&start,NULL);
                fwrite(mesg.data,sizeof(int),INTS_PER_CLIENT,fp);
                gettimeofday(&end,NULL);
                current_time=(double)(end.tv_sec-start.tv_sec)+(double)(end.tv_usec-start.tv_usec)/1000000.0;
                total_time+=current_time;

                clients_done++;
        }

        printf("Server_%d I/O Time: %.6f sec\n",server_id,total_time);

        fclose(fp);

}
