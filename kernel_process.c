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



int CLIENT_MASTER_UP = 1997;
int CLIENT_MASTER_DOWN = 2018;
int DISK_MASTER_UP = 2020;
int DISK_MASTER_DOWN = 2015;
int DISK_PID;

int DISK_UP_QUEUE_ID ,DISK_DOWN_QUEUE_ID;
int CLIENT_UP_QUEUE_ID ,CLIENT_DOWN_QUEUE_ID;

int DISK_PID_EXCHANGE_TYPE = 3;
int CLIENT_PID_EXCHANGE_TYPE = 1;
int ADD_DATA_TYPE = 2;
int DEL_DATA_TYPE = 4;
int DATA_COUNT_TYPE = 15;
int DISK_DONE_TYPE = 16;


int clients[1000];
int clients_count;
int waiting_for_disk_response;

void delete_msg_queues(){
  printf("[Kernel Process] Shutting Down Communication\n");
  msgctl(DISK_UP_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(DISK_DOWN_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(CLIENT_UP_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(CLIENT_DOWN_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  exit(0);
}

void trigger_clk(){
    printf("[Kernel Process] clk Trigger \n");
    kill(DISK_PID, SIGUSR2);
    for(int i=0;i<clients_count;i++)
      kill(clients[i], SIGUSR2);
}
void register_client(int pid){
  printf("[Kernel Process] Registering new client process with pid %d", pid);
  clients[clients_count++] = pid;
}
void process_add_command(char data[64]){
  waiting_for_disk_response = 1;
  struct msgbuff message;
  message.mtype = ADD_DATA_TYPE;
  strncpy(message.mtext,data,64);
  int send_val = msgsnd(DISK_DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Kernel Process] Failed to Send message (command to Disk)\n");

}
void process_del_command(char del_idx){
  waiting_for_disk_response = 1;
  struct msgbuff message;
  message.mtype = DEL_DATA_TYPE;
  message.data = del_idx;
  int send_val = msgsnd(DISK_DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Kernel Process] Failed to Send message (command to Disk)\n");
}
void read_client_command(){
  printf("[Kernel Process] checking commands avaliable to execute \n");
  struct msgbuff message;
  int  rec_val = msgrcv(CLIENT_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, IPC_NOWAIT);
  if(message.mtype == CLIENT_PID_EXCHANGE_TYPE){
    register_client(message.pid);
  }
  if(message.mtype == ADD_DATA_TYPE){
    process_add_command(message.mtext);
  }
  if(message.mtype == DEL_DATA_TYPE){
    process_del_command(message.data);
  }
}

int main(int argc, char *argv[]){


  printf("[Kernel Process] Process Created with process id %d\n" , getpid());
  DISK_UP_QUEUE_ID = msgget(DISK_MASTER_UP, IPC_CREAT|0644);
  DISK_DOWN_QUEUE_ID = msgget(DISK_MASTER_DOWN, IPC_CREAT|0644);
  if(DISK_UP_QUEUE_ID == -1 ||  DISK_DOWN_QUEUE_ID == -1){
    perror("[Kernel Process] Fatal Error:Failed to Create up/down queues (Disk) \n");
  }
  CLIENT_UP_QUEUE_ID = msgget(CLIENT_MASTER_UP, IPC_CREAT|0644);
  CLIENT_DOWN_QUEUE_ID = msgget(CLIENT_MASTER_DOWN, IPC_CREAT|0644);
  if(DISK_UP_QUEUE_ID == -1 ||  DISK_DOWN_QUEUE_ID == -1){
    perror("[Kernel Process] Fatal Error:Failed to Create up/down queues (Client) \n");
  }
  printf("[Kernel Process] Successfully Created Channels (Client/Disk) \n");

  signal (SIGINT, delete_msg_queues);

  printf("[Kernel Process] waiting for Disk Process to be up \n");

  struct msgbuff message;
  int  rec_val = msgrcv(DISK_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
  printf("[Kernel Process] Disk is up with id = %d \n",message.pid);
  DISK_PID = message.pid;

  while(1){
    trigger_clk();
    sleep(1);
    if(waiting_for_disk_response == 0){
      read_client_command();
    }
    else{
      printf("[Kernel Process] Check if Disk is Done\n");
      int  rec_val = msgrcv(DISK_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, IPC_NOWAIT);
      if(message.mtype == -1) continue;
      printf("[Kernel Process] Disk is Done (Asserted)\n");
      waiting_for_disk_response = 0;
      read_client_command();
    }
  }

}