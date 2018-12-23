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
   int data;
   int pid;
};

int MASTER_UP = 2020;
int MASTER_DOWN = 2015;
int UP_QUEUE_ID ,DOWN_QUEUE_ID;
int DATA_ADD_TYPE = 2;
int DATA_DELETE_TYPE = 4;
int DISK_PID_EXCHANGE_TYPE = 3;
int DATA_COUNT_TYPE = 15;
int DISK_DONE_TYPE = 16;
struct msgbuff command_data;
int clk;


char data_slots[10][64];


void clock_update(int s){
  printf("[Disk Process] Clock Tick : %d\n",++clk);
}

void count_free_slots(int s){
  int free_slots_count = 0;
  struct msgbuff message;

  for(int i=0;i<10;i++) {
    free_slots_count += (data_slots[i][0]=='\0');
    message.mtext[i] = (data_slots[i][0]=='\0');
  }

  message.mtype = DATA_COUNT_TYPE;
  message.data = free_slots_count;
  int send_val = msgsnd(UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Disk Process] Failed to Send message (Count of Free Slots)\n");
  printf("[Disk Process] Sent Count of Free Slots at time %d ,count is %d \n", clk,free_slots_count);
}

void add_new_data(struct msgbuff message){
  int starting_time = clk;
  for(int i=0;i<10;i++){
    if(data_slots[i][0] == '\0' ){
        for(int j=0;j<64;j++) data_slots[i][j] = message.mtext[j];
        //sleep(3);
        while(clk < starting_time +3){}
        printf("[Disk Process] Finished Current Command, Sending Data to kernel at time %d \n",clk);
        command_data.mtype = DISK_DONE_TYPE;
        command_data.pid = DATA_ADD_TYPE;
        int send_val = msgsnd(UP_QUEUE_ID, &command_data, sizeof(command_data) - sizeof(command_data.mtype), !IPC_NOWAIT);
        if(send_val == -1 )
          perror("[Disk Process] Failed to Send message (command results)\n");
        return;
    }
  }
}

void delete_data(struct msgbuff message){
  int starting_time = clk;
  int del_idx = message.data;
  for(int i=0;i<64;i++) data_slots[del_idx][i] = '\0';
  //sleep(1);
  while(clk < starting_time +1){}

  printf("[Disk Process] Finished Current Command, Sending Data to kernel at time %d \n",clk);
  command_data.mtype = DISK_DONE_TYPE;
  command_data.pid = DATA_DELETE_TYPE;

  int send_val = msgsnd(UP_QUEUE_ID, &command_data, sizeof(command_data) - sizeof(command_data.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Disk Process] Failed to Send message (command results)\n");
  return;
}

void search_for_command(){
  printf("[Disk Process] No current command , searching in down queue\n");
  struct msgbuff message;
  int  rec_val = msgrcv(DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
  if(rec_val == -1 )
    return;
  printf("[Disk Process] Found New Command ,Executing\n");
  if(message.mtype == DATA_ADD_TYPE){
    add_new_data(message);
  }
  if(message.mtype == DATA_DELETE_TYPE){
    delete_data(message);
  }
}

void send_pid_to_kernel(){
  printf("[Disk Process] intializing Kernel with Disk process PID\n");
  struct msgbuff message;
  message.mtype = DISK_PID_EXCHANGE_TYPE;
  message.pid =  getpid();
  int send_val = msgsnd(UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Disk Process] Failed to Send message (pid exchange)\n");
}

int main(){
  printf("[Disk Process] Process Created with process id %d\n" , getpid());
  UP_QUEUE_ID = msgget(MASTER_UP, IPC_CREAT|0644);
  DOWN_QUEUE_ID = msgget(MASTER_DOWN, IPC_CREAT|0644);
  if(UP_QUEUE_ID == -1 ||  DOWN_QUEUE_ID == -1){
    perror("[Disk Process] Fatal Error:Failed to Create up/down queues \n");
  }
  printf("[Disk Process] Successfully Created/Joined Channels\n");

  for(int i=0;i<10;i++){
    for(int j=0;j<64;j++) data_slots[i][j] = '\0';
  }

  send_pid_to_kernel();

  signal (SIGUSR1 , count_free_slots);
  signal (SIGUSR2 , clock_update);
  while(1){
    search_for_command();
  };

}
