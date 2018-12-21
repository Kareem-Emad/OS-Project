#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>



int MASTER_UP = 2020;
int MASTER_DOWN = 2015;
int UP_QUEUE_ID ,DOWN_QUEUE_ID;
int clk;


void delete_msg_queues(){
  printf("[Disk Process] Shutting Down Communication\n");
  msgctl(UP_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(DOWN_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  exit(0);
}
void clock_update(){
  printf("[Disk Process] Clock Tick : %d",++clk);
  resume_work();
}

void resume_work(){

}
char data_slots[10][64];

int main(){

  UP_QUEUE_ID = msgget(MASTER_UP, IPC_CREAT|0644);
  DOWN_QUEUE_ID = msgget(MASTER_DOWN, IPC_CREAT|0644);
  if(UP_QUEUE_ID == -1 ||  DOWN_QUEUE_ID == -1){
    perror("[Disk Process] Fatal Error:Failed to Create up/down queues \n");
  }
  printf("[Disk Process] Successfully Created/Joined Channels\n");

  for(int i=0;i<10;i++){
    for(int j=0;j<64;j++) data_slots[i][j] = '\0';
  }



  signal (SIGUSR2 , clock_update);
  signal (SIGINT, delete_msg_queues);
  while(1){};

}
