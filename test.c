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

int MASTER_UP = 1997;
int MASTER_DOWN = 2018;
int UP_QUEUE_ID ,DOWN_QUEUE_ID;
int DATA_COUNT_TYPE = 0;
int DATA_ADD_TYPE = 1;
int DATA_DELETE_TYPE = 2;
int COMMAND_DONE = 0;
int NO_COMMAND = -1;


int main(){

  UP_QUEUE_ID = msgget(MASTER_UP, IPC_CREAT|0644);
  DOWN_QUEUE_ID = msgget(MASTER_DOWN, IPC_CREAT|0644);
  if(UP_QUEUE_ID == -1 ||  DOWN_QUEUE_ID == -1){
    perror("[test Process] Fatal Error:Failed to Create up/down queues \n");
  }
  printf("[test Process] Successfully Created/Joined Channels\n");
  struct msgbuff message;

  int  rec_val = msgrcv(UP_QUEUE_ID, &message, sizeof(message) - sizeof(message.mtype), 0, !IPC_NOWAIT);
  if(rec_val == -1 ){
    perror("[test Process]  Failed to Recieve Message");
  }
  else{
    printf("[test Process] recieved message with pid %d\n", message.pid);
    int pid = message.pid;
    kill(pid, SIGUSR2);


  }


  while(1){};

}
