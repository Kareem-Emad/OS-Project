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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


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
int current_client_pid;
int clk;

void delete_msg_queues(){
  printf("[Kernel Process] Shutting Down Communication\n");
  msgctl(DISK_UP_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(DISK_DOWN_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(CLIENT_UP_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(CLIENT_DOWN_QUEUE_ID, IPC_RMID, (struct msqid_ds *) 0);
  exit(0);
}

void trigger_clk(){
    printf("[Kernel Process] clk Trigger %d  \n", ++clk);
    kill(DISK_PID, SIGUSR2);
    for(int i=0;i<clients_count;i++)
      kill(clients[i], SIGUSR2);
}
//###############################################################
//####################Processing Commands########################
//###############################################################
void register_client(int pid){
  printf("[Kernel Process] Registering new client process with pid %d\n", pid);
  clients[clients_count++] = pid;
}
void process_add_command(char data[64]){
  printf("[Kernel Process] Recieved Add Command from process %d \n",current_client_pid);
  printf("[Kernel Process] validating command  with Disk (requesting count of free slots) \n");
  kill(DISK_PID,SIGUSR1);
  struct msgbuff message;
  int  rec_val = msgrcv(DISK_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
  if(message.data <= 0){
    printf("[Kernel Process] Disk is Full ,Add operation rejected \n");
    message.mtype = current_client_pid;
    message.pid = 3;
    int send_val = msgsnd(CLIENT_DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);

    return;
  }
  waiting_for_disk_response = 1;
  message.mtype = ADD_DATA_TYPE;
  strncpy(message.mtext,data,64);
  int send_val = msgsnd(DISK_DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Kernel Process] Failed to Send message (command to Disk)\n");

}
void process_del_command(char del_idx){
  printf("[Kernel Process] Recieved Delete Command from process %d \n",current_client_pid);
  printf("[Kernel Process] validating command  with Disk (requesting count of free slots) \n");
  kill(DISK_PID,SIGUSR1);
  struct msgbuff message;
  int  rec_val = msgrcv(DISK_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
  if(message.mtext[del_idx] == 1){
    printf("[Kernel Process] Slot is Empty ,Del operation rejected \n");
    printf("[Kernel Process] ");
    for(int i=0;i<10;i++){
      printf("%d ",message.mtext[i]);
    }
    printf("\n");
    message.mtype = current_client_pid;
    message.pid = 4;
    int send_val = msgsnd(CLIENT_DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
    return;
  }
  waiting_for_disk_response = 1;
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
  current_client_pid = message.mtype;
  if(message.mtype == CLIENT_PID_EXCHANGE_TYPE){
    register_client(message.pid);
  }
  if(message.pid == ADD_DATA_TYPE){
    process_add_command(message.mtext);
  }
  if(message.pid == DEL_DATA_TYPE){
    process_del_command(message.data);
  }
}
//###############################################################
//############################Main###############################
//###############################################################

int main(int argc, char *argv[]){

  //initializing system queses
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


  //waiting for disk to connect
  printf("[Kernel Process] waiting for Disk Process to be up \n");

  struct msgbuff message;
  int  rec_val = msgrcv(DISK_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
  printf("[Kernel Process] Disk is up with id = %d \n",message.pid);
  DISK_PID = message.pid;

  int clients_wait = 1;
  while(clients_wait--){
    printf("[Kernel Process] waiting for Client Process to be up \n");
    rec_val = msgrcv(CLIENT_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
    printf("[Kernel Process] Client is up with id = %d \n",message.pid);
    register_client(message.pid);
  }

  //iteratively processing commands from processes
  while(1){
    trigger_clk();
    usleep(500 *1000);
    if(waiting_for_disk_response == 0){
      read_client_command();
      usleep(250 *1000);

    }
    else{
      printf("[Kernel Process] Check if Disk is Done\n");
      int  rec_val = msgrcv(DISK_UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, IPC_NOWAIT);
      if(message.mtype != DISK_DONE_TYPE || rec_val == -1){
        usleep(500 *1000);
        continue;
      }
      printf("[Kernel Process] Disk is Done (Asserted)\n");
      waiting_for_disk_response = 0;
      struct msgbuff message;
      message.mtype = current_client_pid;
      message.pid = ADD_DATA_TYPE == message.pid ? 0 : 1;
      if(message.pid == 0){
        printf("[Kernel Process] ADD COMMAND successfull. Sending Feedback to process %d \n",current_client_pid);
      }
      else{
        printf("[Kernel Process] DEL COMMAND successfull. Sending Feedback to process %d \n",current_client_pid);
      }
      int send_val = msgsnd(CLIENT_DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
      if(send_val == -1 )
        perror("[Kernel Process] Failed to Send message (Client Feedback)\n");
      usleep(250 *1000);

      read_client_command();
    }
    usleep(250 *1000);
  }

}
