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

struct msgbuff
{
   long mtype;
   char mtext[64];
   int count;
};

int MASTER_UP = 2020;
int MASTER_DOWN = 2015;
int UP_QUEUE_ID ,DOWN_QUEUE_ID;
int DATA_COUNT_TYPE = 0;
int DATA_ADD_TYPE = 1;
int DATA_DELETE_TYPE = 2;
int COMMAND_DONE = 0;
int NO_COMMAND = -1;

int current_command_status = -1;
struct msgbuff command_data;
int clk;


char data_slots[10][64];

void delete_msg_queues(){
  printf("[Disk Process] Shutting Down Communication\n");
  msgctl(UP_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(DOWN_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  exit(0);
}
void clock_update(){
  printf("[Disk Process] Clock Tick : %d\n",++clk);
  // NO COMMAND , search the down queue for commands
  if(current_command_status == NO_COMMAND){
    printf("[Disk Process] No current command , searching in down queue");
    struct msgbuff message;
    int  rec_val = msgrcv(up_q_id, &message, sizeof(message.mtext), 0, !IPC_NOWAIT);
    if(rec_val == -1 )
      return;
    printf("[Disk Process] Found New Command ,Executing");
    if(message.mtype == DATA_ADD_TYPE){
      current_command_status = 3;
    }
    if(message.mtype == DATA_DELETE_TYPE){
      current_command_status = 1;
      int del_idx = message.count;
      for(int i=0;i<64;i++) data_slots[del_idx][i] = '\0';
    }

  }
  else{
    //command finished , send results to Kernel
    if(current_command_status == COMMAND_DONE){
      printf("[Disk Process] Finished Current Command, Sending Data to kernel at time %d \n",clk);
      int send_val = msgsnd(UP_QUEUE_ID, &command_data, sizeof(command_data.mtext) + sizeof(command_data.count), !IPC_NOWAIT);
      if(send_val == -1 )
        perror("[Disk Process] Failed to Send message (command results)\n");
    }
    //command in progress
    else{
      current_command_status--;
    }
  }
}

void count_free_slots(){
  int free_slots_count = 0;
  for(int i=0;i<10;i++) free_slots_count += (data_slots[i][0]=='\0');
  struct msgbuff message;
  message.count = free_slots_count;
  message.mtype = DATA_COUNT_TYPE;
  int send_val = msgsnd(UP_QUEUE_ID, &message, sizeof(message.mtext) + sizeof(message.count), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Disk Process] Failed to Send message (Count of Free Slots)\n");
  printf("[Disk Process] Sent Count of Free Slots at time %d \n", clk);
}


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
  signal (SIGUSR2 , clock_update);
  signal (SIGINT, delete_msg_queues);
  while(1){};

}
