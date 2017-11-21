#include <string.h>                    /* for strerror() */
#include <stdio.h>                     // for printf, rand
#include <stdlib.h>
#include <unistd.h>                    // for sleep
#include <sys/types.h>                 // for type "pid_t"

//shared memory
#include <sys/shm.h>

// the followings are for semaphores ----- 
#include <sys/sem.h>
#include <sys/ipc.h>

#define NUM_REPEAT 50 // each boiler-man repeats
#define BATHER_TIME_01 300 // 300ms = 0.3 seconds
#define BATHER_TIME_02 800 // 800ms = 0.8 seconds
#define BOILERMAN_TIME_01 1200 // 1200ms = 1.2 seconds
#define BOILERMAN_TIME_02 1600 // 1600ms = 1.6 seconds
#define SHM_KEY 71007109 //SHM KEY
#define SEM_KEY 81008109 //SEM KEY 
#define TRUE  1;
#define FALSE 0;

void main(void){
struct my_mem{
  long int counter;
  int parent;
  int child;
  int boilerManDone[2];
  int finishedCounter; //Used to detect completion of all processes
  int batherCounter;
};


union senum{
int val;
struct semid_ds *buf;
ushort * arry;
}argument;
argument.val = 1;

pid_t  process_id;  
int    i;                     // external loop counter  
int    j;                     // internal loop counter  
int    k = 0;                 // dumy integer  
int    sem_id;                // the semaphore ID
struct sembuf operations[1];  // Define semaphore operations 
int    ret_val;               // system-call return value    
int    shm_id;                // the shared memory ID 
int    shm_size;              // the size of the shared memoy  
struct my_mem * p_shm;        // pointer to the attached shared memory
int my_bid = 0;
int my_tid = 0;
int sleep_time;
long int my_rand;
long int temp;
 // find the shared memory size in bytes ----
shm_size = sizeof * p_shm;  
printf("size of shared memory: %lu\n", sizeof *p_shm);

if (shm_size <= 0)
     {  
       fprintf(stderr, "sizeof error in acquiring the shared memory size. Terminating ..\n");
       exit(0); 
     }

 //create semaphore
sem_id = semget(SEM_KEY, 3, 0666 | IPC_CREAT); 
if (sem_id < 0)
   {
     fprintf(stderr, "Failed to create a new semaphore. Terminating ..\n"); 
     exit(0);
   }


//Initialize the three semaphores
for(int i = 0; i < 3; i++)
{
  if (semctl(sem_id, i, SETVAL, argument) < 0)
  {
    fprintf(stderr, "Failed to initialize the semapgore by 1. Terminating ..\n"); 
    exit(0);  
  }

}

// create  shared memory ----
shm_id = shmget(SHM_KEY, shm_size, 0666 | IPC_CREAT);         
if (shm_id < 0) 
   {
     fprintf(stderr, "Failed to create the shared memory. Terminating ..\n");  
     exit(0);  
   }

p_shm = (struct my_mem *)shmat(shm_id, NULL, 0);     
if (p_shm == (struct my_mem*) -1)
   {
     fprintf(stderr, "Failed to attach the shared memory.  Terminating ..\n"); 
     exit(0);   
   }   

// initialize the shared memory ----
p_shm->counter  = 0;  
p_shm->parent   = 0;   
p_shm->child    = 0;

//used for condition in bather while loop
p_shm->boilerManDone[0] = 0;
p_shm->boilerManDone[1] = 0;

//used for waiting on all processes to finish
p_shm->finishedCounter = 0;
//bather count
p_shm->batherCounter = 0;


//first child spawn
process_id = fork();
if(process_id < 0)
{
  printf("Unable to Fork\n");
  exit(1);
}

//init first spawn to be bather1
if(process_id==0){

  my_tid = 1;

}

//only parent should spawn more children
if(process_id > 0)
{
  for(int i = 0; i < 3; i++)
  {
    if(process_id > 0)
    {
      if(((process_id) = fork()) < 0)
    {
      printf("Unable to Fork\n");
      exit(1);
    }

    }
    if(process_id == 0 && i == 2)
    {
      my_bid = 2;
      break;
    }
    else if(process_id == 0 && i != 2)
    {
    my_tid = i+2;
    break;
    } 
  }
}






//if parent process

if(process_id >  0)
{
my_bid = 1;
//Signal Parent Ready
p_shm->parent = 1;
while(p_shm->child == 0)
{
  k++;
}
}
  
else if(process_id == 0)
{
    operations[0].sem_num = 3;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;
    ret_val = semop(sem_id,operations,1);
    
    p_shm->counter = p_shm->counter + 1;
 

    if(p_shm->counter == 3)
  {
    p_shm->child = 1; 
  } 
   //signal completion
    operations[0].sem_num = 3;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;
    ret_val = semop(sem_id,operations,1);

    while(p_shm->child == 0 || p_shm->parent == 0)
    {
          k++;
    }
   
}

//boiler code
if(my_bid > 0)
{
  for(int i = 0; i<NUM_REPEAT;++i)
  {

    my_rand = rand() % BOILERMAN_TIME_01;
    usleep(my_rand);


   
		//wait for boilers
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;
    ret_val = semop(sem_id,operations,1);
    
 
    
    printf("B%d starts his boiler ...\n",my_bid);

    sleep_time = rand() % BOILERMAN_TIME_02;
    usleep(sleep_time);

    printf("B%d is leaving the bathing area...\n",my_bid);

     

    //this boiler man is done 
    if(i == NUM_REPEAT-1)
    {
        p_shm->boilerManDone[my_bid-1] = 1;   
    }
  
    
    //signal to boilers
    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;
    ret_val = semop(sem_id,operations,1);

 

  }
}
//bather code
else
{ 
while((p_shm->boilerManDone[0] == 0) || (p_shm->boilerManDone[1] == 0))
{
  my_rand = rand() % BATHER_TIME_01;
  usleep(my_rand);

  //bathers wait to increment count
  operations[0].sem_num = 1;
  operations[0].sem_op = -1;
  operations[0].sem_flg = 0;
  ret_val = semop(sem_id,operations,1);


  p_shm->batherCounter = p_shm->batherCounter + 1;
  if(p_shm->batherCounter == 1)
  {
  operations[0].sem_num = 0;
  operations[0].sem_op = -1;
  operations[0].sem_flg = 0;
  ret_val = semop(sem_id,operations,1);

  }


 //more bathers allowed in now
  operations[0].sem_num = 1;
  operations[0].sem_op = 1;
  operations[0].sem_flg = 0;
  ret_val = semop(sem_id,operations,1);
 
  printf("T%d is entering the bathing area\n",my_tid);

  //relax...
  
  my_rand = rand() % BATHER_TIME_02;
  usleep(my_rand);

    
  //bathers wait on each other to leave
  operations[0].sem_num = 1;
  operations[0].sem_op = -1;
  operations[0].sem_flg = 0;
  ret_val = semop(sem_id,operations,1);
 
  printf("T%d is leaving the bathing area...\n",my_tid);

  p_shm->batherCounter = p_shm->batherCounter -1;

   if(p_shm->batherCounter == 0)
  {
  //boilermen can come back in now.
  operations[0].sem_num = 0;
  operations[0].sem_op = 1;
  operations[0].sem_flg = 0;
  ret_val = semop(sem_id,operations,1);
  }
  
   //signal to other bathers
  operations[0].sem_num = 1;
  operations[0].sem_op = 1;
  operations[0].sem_flg = 0;
  ret_val = semop(sem_id,operations,1);

 
 }
}

if(process_id >  0)
{
//Signal Parent finished
p_shm->parent = 0;
}
 else if(process_id == 0)
 {
    operations[0].sem_num = 3;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;
    ret_val = semop(sem_id,operations,1);
    
  p_shm->finishedCounter = p_shm->finishedCounter + 1;
      
  if(p_shm->finishedCounter == 4)
  {
    p_shm->child = 0;     //all processes finished
  }

 
   //signal completion
    operations[0].sem_num = 3;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;
    ret_val = semop(sem_id,operations,1);
}

while(p_shm->parent != 0 || p_shm->child !=0)
{
  k++;
}

// only allow parent to detach SHM and Semaphores
if(process_id >  0){
ret_val = shmdt(p_shm);  
if (ret_val != 0) { printf ("shared memory detach failed ....\n"); }
ret_val = shmctl(shm_id, IPC_RMID, 0); 
if (ret_val != 0) { printf("shared memory ID remove ID failed ... \n"); } 
ret_val = semctl(sem_id, IPC_RMID, 0);  
if (ret_val != 0) { printf("semaphore remove ID failed ... \n"); }
}

}//END Main


