#include "helper.h"

// Global Variable
Queue queue;
int mutex = 0;
int space = 1;
int items = 2;

// main program
int main (int argc, char **argv){

  // set random seed
  srand(time(NULL));
  
  // check command line arguments for incorrect input
  if ( argc != 5 ) {
    fprintf(stderr, "Insufficient command line arguments: 4 required\n");
    return -1;
  }
  
  for ( int i = 1; i < argc; i++){
    if ( check_arg(argv[i]) == -1 ) return -1;
  }

  // Set up
  queue.size_of_queue = check_arg(argv[1]);
  int number_of_jobs = check_arg(argv[2]);
  int number_of_producers = check_arg(argv[3]);
  int number_of_consumers = check_arg(argv[4]);
  queue.number_of_producers = number_of_producers;
  queue.total_jobs = number_of_jobs * number_of_producers;
  queue.jobs_to_be_produced = queue.total_jobs;     // for generation of jobs id

  queue.jobs_array = new Job[queue.size_of_queue];  // circular job queue  
  
  int semid = sem_create(SEM_KEY, 3);
  if ( sem_init(semid, mutex, 1) == -1 ){  // mutex
    fprintf(stderr, "Failed to create mutex");
    return -1;
  }
  
  if ( sem_init(semid, space, queue.size_of_queue) == -1){   // space
    fprintf(stderr, "Failed to create space semaphore");
    return -1;
  }
  
  if ( sem_init(semid, items, 0) == -1){                     // item
    fprintf(stderr, "Failed to create items semaphore");
    return -1;
  }  
	
  // Print starting banner
  cout << "================ Start of production and consumption ================" << endl;
    
  // Creating the required number of producer threads
  thread_info pt_info[number_of_producers]; 

  for ( int i = 0; i < number_of_producers; i++){
    pt_info[i].thread_num = i + 1;
    pt_info[i].semid = semid;

    // error handling if thread creation fails
    if(pthread_create (&(pt_info[i].thread_id), NULL, producer, (void*)&pt_info[i]) != 0){
      fprintf(stderr, "Failed to create Producer(%d).", pt_info[i].thread_num);
      return -1;
    }
  }

  // Creating the required number of consumer threads
  thread_info ct_info[number_of_consumers];   

  for ( int i = 0; i < number_of_consumers; i++){
    ct_info[i].thread_num = i + 1;
    ct_info[i].semid = semid;
    
    // error handling if thread creation fails
    if(pthread_create (&(ct_info[i].thread_id), NULL, consumer, (void*)&ct_info[i]) != 0){
      fprintf(stderr, "Failed to create Consumer(%d).", ct_info[i].thread_num);
      return -1;
    }
  }
 
  // calling thread is main().
  // main() will be suspended until each of the individual threads has terminated
  for ( int i = 0; i < number_of_producers; i++){
    if(pthread_join (pt_info[i].thread_id, NULL) != 0){
      fprintf(stderr, "Fail to join Producer(%d).", pt_info[i].thread_num);
      return -1;
    }
  }
  
  for ( int i = 0; i < number_of_consumers; i++){
    if(pthread_join (ct_info[i].thread_id, NULL) != 0){
      fprintf(stderr, "Failed to join Consumer(%d).", ct_info[i].thread_num);
      return -1;
    }
  } 

  // destroying semaphore array
  if( sem_close(semid) == -1 ){
    fprintf(stderr, "Failed to close semaphore id %d", semid);
    return -1;
  }

  // tallying total production and consumption record
  cout << endl;
  cout << "================ End of production and consumption ================" << endl;
  cout << "Total jobs produced = " << queue.total_jobs - queue.jobs_to_be_produced << " out of " << queue.total_jobs << " jobs allocated"<< endl; 
  cout << "Jobs left in queue unconsumed = " << queue.jobs_in_queue << " in a queue size of " << queue.size_of_queue << endl;
  cout << "Total jobs consumed = " << queue.total_jobs - queue.jobs_to_be_produced - queue.jobs_in_queue << " out of " << queue.total_jobs - queue.jobs_to_be_produced << " jobs produced" << endl;
  
  return 0;
}


// =========================   producer function   ============================
void *producer (void *parameter) 
{
  int producer_count = 0;
  int jobs_per_producer = queue.total_jobs / queue.number_of_producers;

  // casting back to thread_info
  thread_info* param = (thread_info*) parameter;

  // thread continues working as long as there are jobs to complete
  while ( true ) {

    // break if quota per producer is already met
    if ( producer_count == jobs_per_producer) break;

    // Each producer can only add job once every 1-5 seconds
    sleep ((rand() % 5) +1);
    
    // preparing duration
    int consumption_duration = (rand() % 10) + 1;
    int production_duration = (rand() % 10) + 1;
    
    // checking if there is space. wait if full.
    if ( sem_wait((*param).semid, space) == -1 ) break;   
        
    // entering critical section. wait if blocked.
    sem_wait((*param).semid, mutex);   
  
    // produce one job
    int id = (queue.total_jobs - queue.jobs_to_be_produced) % queue.size_of_queue;
    queue.jobs_to_be_produced--;

    // deposit item
    queue.jobs_array[queue.jobs_in_queue] = Job(id, consumption_duration);
    queue.jobs_in_queue++;
    producer_count++;
    
    fprintf(stderr, "Producer(%d): Producing Job id %d (with duration %d) which takes %d seconds. Quota %d/%d\n", (*param).thread_num, id, consumption_duration, production_duration, producer_count, jobs_per_producer);

    sleep (production_duration);

    // exiting critical section
    sem_signal((*param).semid, mutex); 

    // depositing item
    sem_signal((*param).semid, items);
  }

  fprintf(stderr, "Producer(%d): No more jobs to generate. Fulfilled quota %d/%d\n", (*param).thread_num, producer_count, jobs_per_producer);

  // terminates the calling thread
  pthread_exit(0);
}

// ========================   consumer function   ================================
void *consumer (void *parameter) 
{

  // casting back to thread_info
  thread_info* param = (thread_info*) parameter;

  while ( true ){
  
    // checking if it is empty. wait if empty.
    if ( sem_wait((*param).semid, items) == -1 ) break;
    
    // entering critical section. wait if blocked.
    sem_wait((*param).semid, mutex);   

    // fetching item
    int id = queue.jobs_array[0].id;
    int consumption_duration = queue.jobs_array[0].consumption_duration;
    
    // consuming item
    queue.jobs_in_queue--;
    
    // shift all jobs_id in front by one index position
    for ( int i = 0; i < queue.jobs_in_queue; i++ ){
      queue.jobs_array[i] = queue.jobs_array[i+1];
    }
    
    fprintf(stderr, "\tConsumer(%d): Consuming Job id %d. Executing %d seconds of sleep\n", (*param).thread_num, id, consumption_duration);

    sleep (consumption_duration);
    
    // exiting critical section
    sem_signal((*param).semid, mutex);

    // consuming item
    sem_signal((*param).semid, space);

    fprintf(stderr, "\tConsumer(%d): Job id %d consumed\n", (*param).thread_num, id);
  }

  fprintf(stderr, "\tConsumer(%d): No more jobs left\n", (*param).thread_num);
  
  // terminate the calling thread
  pthread_exit (0);

}
