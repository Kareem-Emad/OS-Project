#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
using namespace std;

struct msgbuff
{
   long mtype;
   char mtext[64];
   int free_slots_count;
};

class DiskProcess {
  public:
    static bool empty_slots[10];

    DiskProcess(int,int);
  private:
    int clk;
    char arr[10][64];
    int master_up;
    int master_down;
    int up_q_id;
    int down_q_id;
    void delete_msg_queues();
    void increment_clock();
    void count_free_slots(int);
    void recieve_command();

};
void DiskProcess::recieve_command(){
  struct msgbuff message;
  int rec_val = msgrcv(up_q_id, &message,sizeof(message.mtext) + sizeof(message.free_slots_count) , 0, !IPC_NOWAIT);
  if(rec_val !=-1){
    cout << "Disk Process Recieved Command  "  << endl;
    // process command
  }
}
void
void DiskProcess::count_free_slots(int num){
  int free_slots_count = 0;
  //for(int i=0;i<10;i++) free_slots_count += DiskProcess::empty_slots[i];
  //send message with free slots count
}
void DiskProcess::delete_msg_queues(){
  cout << "Shutting Down Communication";
  msgctl(up_q_id, IPC_RMID, (struct msqid_ds *) 0);
  msgctl(down_q_id, IPC_RMID, (struct msqid_ds *) 0);

}
void DiskProcess::increment_clock(){clk++;}

DiskProcess::DiskProcess(int mup,int mdn){
  clk = 0;
  master_up = mup;
  master_down = mdn;
  up_q_id = msgget(master_up, IPC_CREAT|0644);
  down_q_id = msgget(master_down, IPC_CREAT|0644);
  if(up_q_id == -1 or down_q_id == -1){
    perror("Fatal Error:Failed to Create up/down queues between Disk/Kernel\n");
  }
  //signal (SIGINT, delete_msg_queues);
  //signal (SIGUSR2, increment_clock);
  //signal (SIGUSR2, DiskProcess::count_free_slots);

}


int main(){
  cout << "hello world " << endl;
}
