#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <semaphore.h>

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condstart=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_end=PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_end2=PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex2= PTHREAD_MUTEX_INITIALIZER;
sem_t sem_slots;
sem_t sem_finish;
sem_t sem_condition;
sem_t sem_T35ends;
sem_t sem_T52ends;
int active_threads = 0;
int t1started = 0;
int t4_finished = 0;
int condition2=0;
int number_threads=0;
int thread12_started=0;
int condition=0;
int thread12_ended=0;
void *threadFunctionP3(void *arg)
{
    int thread_id = *(int *)arg;
    if (thread_id == 1)
    {  
        info(BEGIN, 3, 1);
        t1started = 1;
        pthread_cond_signal(&condstart);
        while (!t4_finished)
        {
            pthread_cond_wait(&condstart, &mutex);
        }
        
        info(END, 3, 1);
    }
    else if (thread_id == 4)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condstart, &mutex);
        info(BEGIN, 3, 4);
        info(END, 3, 4);
        t4_finished = 1;
        pthread_cond_broadcast(&condstart);
        pthread_mutex_unlock(&mutex);
    }
     else if (thread_id == 5)
    {
        sem_wait(&sem_T52ends);
        info(BEGIN, 3, 5);
        info(END,3,5);
      sem_post(&sem_T35ends);
    }
    else
    {
        info(BEGIN, 3, thread_id);
        info(END, 3, thread_id);
    }
    return NULL;
}


void *threadFunctionP4(void *arg) {
    int thread_id = *(int *)arg;
       
    sem_wait(&sem_slots);
    pthread_mutex_lock(&mutex2);
    active_threads++;
       number_threads++;
    if (active_threads == 6) {
        condition=1;
       
        pthread_cond_broadcast(&cond_end);
    }
   if(number_threads==44 && active_threads<6)
         {
            condition=1;
             pthread_cond_broadcast(&cond_end);
         }
    pthread_mutex_unlock(&mutex2);

    info(BEGIN, 4, thread_id);
  
    if (thread_id == 12) {
        thread12_started = 1; 
        pthread_mutex_lock(&mutex2);
        while (active_threads <6 && !condition) {
            { sem_wait(&sem_finish);
                pthread_cond_wait(&cond_end, &mutex2);
            }
           
        }
       
        pthread_mutex_unlock(&mutex2);
     
        info(END, 4, 12);
     

        pthread_mutex_lock(&mutex2);
        active_threads--;
         thread12_ended = 1; 
       if(thread12_ended==1)
           { pthread_cond_broadcast(&cond_end2);}
       
      pthread_mutex_unlock(&mutex2);
    } else {
        pthread_mutex_lock(&mutex2);
        if(thread12_started==1)
        {
            for(int i=0;i<6;i++)
            {sem_post(&sem_finish);}
            while (!thread12_ended) {
            pthread_cond_wait(&cond_end2, &mutex2);
        }
         
         info(END, 4, thread_id);
          active_threads--;
        pthread_mutex_unlock(&mutex2);
      
       
        }
        else
        {
          info(END, 4, thread_id);
            active_threads--;
        pthread_mutex_unlock(&mutex2);
    
    }
    }
 
    sem_post(&sem_slots);
    return NULL;
}


void *threadFunctionP5(void *arg) {
    int thread_id = *(int *)arg;
if (thread_id == 1) {
    sem_wait(&sem_T35ends);
        info(BEGIN, 5, 1);
        info(END, 5, 1);
    }
    
    else if (thread_id == 2) {
        info(BEGIN, 5, 2);
        info(END, 5, 2);
      
        sem_post(&sem_T52ends);
    } else{
        info(BEGIN, 5, thread_id);
        info(END, 5, thread_id);
    }
    return NULL;
}



void create_threads_p35()
{
    pthread_t threads3[5];
     pthread_t threads5[5];
    int id_thread3[5];
        int id_thread5[5];

    for (int i = 0; i < 5; i++)
    {
        id_thread5[i] = i + 1;
         id_thread3[i] = i + 1;
        pthread_create(&threads5[i], NULL, threadFunctionP5, &id_thread5[i]);
         pthread_create(&threads3[i], NULL, threadFunctionP3, &id_thread3[i]);
    }
    for (int i = 0; i < 5; i++)
    {
        pthread_join(threads3[i], NULL);
    }
     for (int i = 0; i < 5; i++)
    {
        pthread_join(threads5[i], NULL);
    }
}
void create_threads_p4()
{
    pthread_t threads[44];
    int id_thread[44];
    for (int i = 0; i < 44; i++)
    {
        id_thread[i] = i + 1;
        pthread_create(&threads[i], NULL, threadFunctionP4, &id_thread[i]);
    }


    for (int i = 0; i < 44; i++)
    { 
        pthread_join(threads[i], NULL);
    }
  
}

int main()
{
    init();
    pthread_mutex_init(&mutex, NULL);
        pthread_mutex_init(&mutex2, NULL);
    pthread_cond_init(&condstart, NULL);
    pthread_cond_init(&cond_end, NULL);
pthread_cond_init(&cond_end2, NULL);

    sem_init(&sem_slots, 0, 6);
    sem_init(&sem_finish, 0, 0);
    sem_init(&sem_T52ends,0,0);
  sem_init(&sem_T35ends,0,0);

    info(BEGIN, 1, 0);
    int status;
    pid_t pid;
    int process_ids[] = {2, 4, 7};

    for (int i = 0; i < sizeof(process_ids) / sizeof(process_ids[0]); i++)
    {
        pid = fork();
        switch (pid)
        {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            info(BEGIN, process_ids[i], 0);
            switch (process_ids[i])
            {
            case 2:
            {
                pid_t pid3 = fork();
                switch (pid3)
                {
                case -1:
                    perror("fork");
                    exit(1);
                case 0:
                    info(BEGIN, 3, 0);
                   
                    pid_t pid5 = fork();
                    switch (pid5)
                    {
                    case -1:
                        perror("fork");
                        exit(1);
                    case 0:
                        info(BEGIN, 5, 0);
                         
                        create_threads_p35();
                 
                        pid_t pid6 = fork();
                        switch (pid6)
                        {
                        case -1:
                            perror("fork");
                            exit(1);
                        case 0:
                            info(BEGIN, 6, 0);
                            info(END, 6, 0);
                            exit(0);
                        default:
                            wait(&status);
                            info(END, 5, 0);
                            exit(0);
                        }
                    default:
                        wait(&status);
                        info(END, 3, 0);
                        exit(0);
                    }
                default:
                    wait(&status);
                    info(END, 2, 0);
                    exit(0);
                }
            }
            case 4:
                create_threads_p4();
                info(END, 4, 0);
                exit(0);
            case 7:
                info(END, 7, 0);
                exit(0);
            }
        default:
            waitpid(pid, NULL, 0);
            break;
        }
    }
 sem_destroy(&sem_T52ends);
        sem_destroy(&sem_T35ends);

pthread_cond_destroy(&cond_end2);

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex2);
    pthread_cond_destroy(&condstart);
        pthread_cond_destroy(&cond_end);
    sem_destroy(&sem_slots);
    sem_destroy(&sem_finish);
    info(END, 1, 0);
    return 0;
}