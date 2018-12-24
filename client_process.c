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
struct cmd
{
  int cmd_type;
   char data[64];
   int del_idx;
   int t;
};


int MASTER_UP = 1997;
int MASTER_DOWN = 2018;
int CLIENT_PID_EXCHANGE_TYPE = 1;
int ADD_DATA_TYPE = 2;
int DEL_DATA_TYPE = 4;
int UP_QUEUE_ID ,DOWN_QUEUE_ID;
int ADD_TYPE = 0;
int DEL_TYPE = 1;
int commands_count;
int current_command;
struct cmd commands_queue[1000];
int clk;





void clock_update(int s){
  printf("[Client Process] Clock Tick : %d\n",++clk);
}

int read_data_file( int argc ,char *argv[]){
  if(argc < 2){
    perror("[Client Process]  A path is required to spawn a client process. Exiting .. \n" );
    return 0;
  }
  FILE *fp;
  char path[50];
  strncpy(path,argv[1],40);
  printf("[Client Process] Reading From File specified as : %s\n",path);
  fp = fopen(path, "r");
  if(fp == F_OK){
    perror("[Client Process]  A Legit path is required to spawn a client process. Exiting .. \n" );
    return 0;
  }
  int t;
  char command[4];
  int del_idx;
  char command_data[64];
  while(fscanf(fp, "%d", &t) != EOF ){
    fscanf(fp, "%s", command);

    if(command[0] == 'D'){
      fscanf(fp, "%d", &del_idx);
      struct cmd curr_cmd;
      curr_cmd.t =t ;
      curr_cmd.del_idx = del_idx;
      curr_cmd.cmd_type = DEL_TYPE;
      strncpy(curr_cmd.data,command,4);
      commands_queue[commands_count++] = curr_cmd;
    }
    else{
      fscanf(fp, " %[^\n.]s", command_data);
      printf("%d %s %s\n",t,command,command_data);
      struct cmd curr_cmd;
      curr_cmd.t =t ;
      curr_cmd.cmd_type = ADD_TYPE;
      strncpy(curr_cmd.data,command_data,64);
      commands_queue[commands_count++] = curr_cmd;
    }

  }

  return 1;
}

void send_pid_to_kernel(){
  printf("[Client Process] intializing Kernel with Client process PID\n");
  struct msgbuff message;
  message.mtype = CLIENT_PID_EXCHANGE_TYPE;
  message.pid =  getpid();
  int send_val = msgsnd(UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
  if(send_val == -1 )
    perror("[Client Process] Failed to Send message (pid exchange)\n");
}



int main(int argc, char *argv[]){
  if(read_data_file(argc,argv) != 1) return 0;


  printf("[Client Process] Process Created with process id %d\n" , getpid());
  UP_QUEUE_ID = msgget(MASTER_UP, IPC_CREAT|0644);
  DOWN_QUEUE_ID = msgget(MASTER_DOWN, IPC_CREAT|0644);
  if(UP_QUEUE_ID == -1 ||  DOWN_QUEUE_ID == -1){
    perror("[Client Process] Fatal Error:Failed to Create up/down queues \n");
  }
  printf("[Client Process] Successfully Created/Joined Channels\n");

  send_pid_to_kernel();


  signal (SIGUSR2 , clock_update);
  while(1){
    if(current_command >= commands_count){
      printf("[Client Process] Finished all commands . Exiting ...\n");
      break;
    }
    struct cmd curr_cmd = commands_queue[current_command++];
    if(curr_cmd.t > clk){
      current_command--;
      continue;
    }


    printf("[Client Process] Processing Command #%d  \n" , current_command);
    struct msgbuff message;
    message.pid =  getpid();
    if(curr_cmd.cmd_type == ADD_TYPE){
      message.pid = ADD_DATA_TYPE;
      message.mtype = getpid();
      strncpy(message.mtext, curr_cmd.data,64);
    }
    else{
      message.mtype = getpid();
      message.pid = DEL_DATA_TYPE;
      message.data = curr_cmd.del_idx;
    }
    int send_val = msgsnd(UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), !IPC_NOWAIT);
    if(send_val == -1 )
      perror("[Client Process] Failed to Send message (command issuing)\n");
    printf("[Client Process] Command Issued . Waiting for Response \n");
    int  rec_val =  msgrcv(DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), getpid(), IPC_NOWAIT);
    while(rec_val == -1 ){
      rec_val =  msgrcv(DOWN_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), getpid(), IPC_NOWAIT);
    }
    if(message.pid < 2){
      printf("[Client Process] operation Successfull\n");
    }
    else{
      printf("[Client Process] operation Failed\n");
    }
      //perror("[Client Process] Failed to Recieve message (command response)\n");
    printf("[Client Process] Response arrived \n");

  };

}
