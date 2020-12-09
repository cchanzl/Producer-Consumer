/******************************************************************
 * Header file for the helper functions. This file includes the
 * required header files, as well as the function signatures and
 * the semaphore values (which are to be changed as needed).
 ******************************************************************/


# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <sys/sem.h>
# include <sys/time.h>
# include <math.h>
# include <errno.h>
# include <string.h>
# include <pthread.h>
# include <ctype.h>
# include <iostream>
# include <time.h>
using namespace std;

# define SEM_KEY 0x22 // Change this number as needed

union semun {
    int val;               /* used for SETVAL only */
    struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
    ushort *array;         /* used for GETALL and SETALL */
};

// used as an argument to pthread_create()
struct thread_info{
  pthread_t thread_id;
  int thread_num;
  int semid;
};

struct Job {
  int id;
  int consumption_duration;
  Job(){};
  Job (int id, int consumption_duration)
    :id(id), consumption_duration(consumption_duration){};
};

struct Queue {
  Job* jobs_array = nullptr;    // jobs queue added to and consume by producers/consumers
  int number_of_producers = 0;
  int size_of_queue;            // size of circular queue
  int total_jobs;               // total number of jobs to be produced by all producers
  int jobs_in_queue = 0;        // number of jobs in queue at any one time
  int jobs_to_be_produced = 0;  // total jobs left to be produced
  
  ~Queue(){
    delete[] jobs_array;
  };
};

void *producer (void *parameter);
void *consumer (void *parameter);

int check_arg (char *);
int sem_create (key_t, int);
int sem_init (int, int, int);
int sem_wait (int, short unsigned int);
void sem_signal (int, short unsigned int);
int sem_close (int);
