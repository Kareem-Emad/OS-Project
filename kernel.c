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

/*Intializing inter-communication related variables*/
int MASTER_UP_DISK = 2020;
int MASTER_DOWN_DISK = 2015;
int MASTER_UP_PROCESS = 1997;
int MASTER_DOWN_PROCESS = 2014;
int UP_QUEUE_ID_DISK, DOWN_QUEUE_ID_DISK;
int UP_QUEUE_ID_PROCESS, DOWN_QUEUE_ID_PROCESS;

/*System clock*/
int clk;

/*Defining system data sturctures*/
const int MAX_SIZE = 101;
int numberOfProcesses, numberOfClients;
int processes[MAX_SIZE], DiskPID;

/*Define message types*/
enum MessageTypes
{
    PROCESS_ARRIVAL,
    PROCESS_ADD_REQUEST,
    PROCESS_DEL_REQUEST,
    DISK_FREE_COUNT_RESPONSE,
    DISK_REQUEST_STATUS_RESPONSE,
    PROCESS_REQUEST_STATUS_RESPONSE
};

/*Defining Disk responses*/
enum DiskResponses
{
    SUCCESSFULL_ADD,
    SUCCESSFULL_DEL,
    UNSUCCESSFULL_ADD,
    UNSUCCESSFULL_DEL
}

/*Declaring the structure of the communication message*/
struct msgbuff
{
    long mtype;
    char mtext[64];
    int data; //Contains the data either [Count of free slots sent by disk, the index at which we insert]
    int pid; //The id of the process which sent the request.
};



/*-----------------------------------System Init functions---------------------------------------*/
/*Establishes the message queues between the Kernel and the Disk process*/
bool establishDiskMessagingQueues()
{
    //Create the up and down communication queues with the Disk.
    UP_QUEUE_ID_DISK = msgget(MASTER_UP_DISK, IPC_CREAT|0644);
    DOWN_QUEUE_ID_DISK = msgget(MASTER_DOWN_DISK, IPC_CREAT|0644);

    //Handle failure to create Kernel-Disk message queues
    if(UP_QUEUE_ID_DISK == -1 ||  DOWN_QUEUE_ID_DISK == -1){
        perror("[Kernel] Fatal Error: Failed to Create Kernel-Disk up/down queues\n");
        return false;
    }
    else{
        printf("[Kernel] Kernel-Disk message queues created successfully\n");
        return true;
    }
}

/*Establishes the message queues between the Kernel and the Disk process*/
bool establishProcessMessagingQueues()
{
    //Create the up and down communication queues with the all other processes (clients).
    UP_QUEUE_ID_PROCESS = msgget(MASTER_UP_PROCESS, IPC_CREAT|0644);
    DOWN_QUEUE_ID_PROCESS = msgget(MASTER_DOWN_PROCESS, IPC_CREAT|0644);

    //Handle failure to create Kernel-Process message queues
    if(UP_QUEUE_ID_PROCESS == -1 ||  DOWN_QUEUE_ID_PROCESS == -1){
        perror("[Kernel] Fatal Error: Failed to Create Kernel-Process up/down queues\n");
         return false;
    }
    else{
        printf("[Kernel] Kernel-Process message queues created successfully\n");
        return true;
    }
}

/*Initiates the system. Waits for the Disk Process and all other client Porcesses to send their pids [Up and running]*/
bool initiateSystem()
{
    /*Recieve the Disk pid*/
    struct msgbuff message = receiveMessage(UP_QUEUE_ID_DISK, PROCESS_ARRIVAL, !IPC_NOWAIT);
    if(message.data == -1){
        perror("[Kernel]: Couldn't obtain the Disk PID");
        return false;
    }
    else{
        printf("[Kernel]: Successfully received the Disk PID\n");
        addProcessHandler(message.pid);
        return true;
    }

    /*Recieve pid of all other processes*/
    while(numberOfProcesses < numberOfClients){
        struct msgbuff message = receiveMessage(UP_QUEUE_ID_PROCESS, PROCESS_ARRIVAL, !IPC_NOWAIT);

        if(message.data == -1){
            perror("[Kernel]: Couldn't obtain the PID of process number: %d\n", numberOfProcesses);
            return false;
        }

        addProcessHandler(message.pid);
    }

    printf("[Kernel]: Successfully received all %d processes PIDs\n", numberOfProcesses);
    return true;
}
/*-----------------------------------System Init functions---------------------------------------*/



/*-----------------------------------------Sync functions----------------------------------------*/
bool updateSystemClock()
{
    clk++;

    /*Send signal to the Disk Process to update its clock*/
    int error = kill(DiskPID, SIGUSR2);
    if(error == EINVAL || error == EPERM || error == ESRCH){
        perror("[Kernel]: Lost communication with the Disk Process\n");
        return false;
    }

    printf("Current clock = %d\n", clk);
    return true;
}
/*-----------------------------------------Sync functions----------------------------------------*/



/*-------------------------------Inter-Process communcation functions----------------------------*/
/*A function used by the kernel to send messages to Disk and Processes*/
struct msgbuff receiveMessage(int queueID, int messageType, int waitingStatus)
{
    struct msgbuff message; //A struct to recieve the message in.
    int rec_val;
    int msgsz = sizeof(message.mtext) + sizeof(message.data) + sizeof(message.pid); //Size of the message excluding mType
	
	/* receive all types of messages */
	rec_val = msgrcv(queueID, &message, msgsz, messageType, waitingStatus);  
	  
	if(rec_val == -1){
		perror("[Kernel]: Error in receiving message on queue = %d with type = %d\n", queueID, messageType);
        message.data = -1; //Error code.
    }
	
    return message;
}

/*A function by the kernel to send messages to Disk and Processes*/
bool sendMessage(int queueID, const msgbuff& message, int waitingStatus)
{
    int msgsz = sizeof(message.mtext) + sizeof(message.data) + sizeof(message.pid); //Size of the message excluding mType

	int send_val = msgsnd(queueID, message, msgsz, waitingStatus);
	  
	if(send_val == -1){
		perror("[Kernel]: Error in sending message on queue = %d with type = %d\n", queueID, message.messageType);
        return false;
    }

    return true;
}
/*-------------------------------Inter-Process communcation functions----------------------------*/



/*------------------------------Request Handlers (Helper Functions) -----------------------------*/
bool getDiskFreeSlots()
{
    /*Send signal to the Disk Process to obtain the free slots count*/
    int error = kill(DiskPID, SIGUSR1);
    if(error == EINVAL || error == EPERM || error == ESRCH){
        perror("[Kernel]: Lost communication with the Disk Process\n");
        return false;
    }

    return true;
}

bool sendResponseToProcess(int response, int pid)
{
    struct msgbuff message;
    message.mtype = pid;
    message.data = response;
    message.pid = getpid();

    return sendMessage(DOWN_QUEUE_ID_PROCESS, message, IPC_NOWAIT);
}

/*------------------------------Request Handlers (Helper Functions) -----------------------------*/



/*-------------------------------------Request Handlers------------------------------------------*/
void addProcessHandler(int pid)
{
    processes[numberOfProcesses] = pid;
    numberOfProcesses++;
    return;
}

void addOperationHandler(struct msgbuff message)
{
    //STEP: 1
    bool succeeded = getDiskFreeSlots();
    if(!succeeded) return;

    struct msgbuff freeSlotsResponse = receiveMessage(UP_QUEUE_ID_DISK, DISK_FREE_COUNT_RESPONSE, !IPC_NOWAIT);

    if(freeSlotsResponse.data == -1){
        printf("[Kernel]: Can't perform add operation, lost contact with the Disk\n");
        return;
    }
    if(freeSlotsResponse.data == 0){
        printf("[Kernel]: Can't perform add operation, Disk is full\n");
        return;
    }

    //STEP: 2
    bool succeeded = sendMessage(DOWN_QUEUE_ID_DISK, message, !IPC_NOWAIT);

    if(!succeeded){
        printf("[Kernel]: Can't perform add operation, lost contact with the Disk\n");
        return;
    }
    
    //STEP: 3
    struct  msgbuff diskResponse = receiveMessage(UP_QUEUE_ID_DISK, DISK_REQUEST_STATUS_RESPONSE, !IPC_NOWAIT);

    if(diskResponse.data == SUCCESSFULL_ADD){
        printf("Add operation successfully executed at clk = %d\n", clk);
        sendResponseToProcess(SUCCESSFULL_ADD, message.pid);
        return;
    }
    if(diskResponse.data == UNSUCCESSFULL_ADD){
        printf("Add operation failed at clk = %d\n", clk);
        sendResponseToProcess(UNSUCCESSFULL_ADD, message.pid);
        return;
    }
}

void deleteOperationHandler(struct msgbuff message)
{
    //STEP: 1
    bool succeeded = sendMessage(DOWN_QUEUE_ID_DISK, message, !IPC_NOWAIT);

    if(!succeeded){
        printf("[Kernel]: Can't perform delete operation, lost contact with the Disk\n");
        return;
    }

    //STEP: 2
    struct  msgbuff diskResponse = receiveMessage(UP_QUEUE_ID_DISK, DISK_REQUEST_STATUS_RESPONSE, !IPC_NOWAIT);

    if(diskResponse.data == SUCCESSFULL_DEL){
        printf("Delete operation successfully executed at clk = %d\n", clk);
        sendResponseToProcess(SUCCESSFULL_DEL, message.pid);
        return;
    }
    if(diskResponse.data == UNSUCCESSFULL_DEL){
        printf("Delete operation failed at clk = %d\n", clk);
        sendResponseToProcess(UNSUCCESSFULL_DEL, message.pid);
        return;
    }
}
/*-------------------------------------Request Handlers------------------------------------------*/

void work()
{
    while(true){
        //See if any messages came in the processes messages queues.
        struct msgbuff message = recieveMessage(UP_QUEUE_ID_PROCESS, 0, !IPC_NOWAIT);
        

    }
}

int main()
{
    printf("[Kernel] has started with id = %d\n", getpid());

    //Establish communication with Disk
    if(!establishDiskMessagingQueues()) return 0;

    //Establish communication with Processes
    if(!establishProcessMessagingQueues()) return 0;

    //Initiate the system
    if(!initiateSystem()) return 0;

    work();

    return 0;
}
