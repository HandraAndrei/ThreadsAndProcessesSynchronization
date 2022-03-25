#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>

pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_start = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_finish = PTHREAD_COND_INITIALIZER;

sem_t* sem1=NULL;
sem_t* sem2=NULL;
sem_t* sem3=NULL;
sem_t* sem4=NULL;

int start = 0;
int finish = 0;
int N = 6;
int threads_at_barrier = 0;
int max_threads = 36;
int waiting_for_ten = 0;
sem_t barrier;
sem_t mutex;
sem_t simultaneously;
sem_t mutex_10;
sem_t barrier10;

void* same_process(void* args){
    int i=(int)(size_t)args;
    pthread_mutex_lock(&lock);
    while(i == 2 && start==0){
        pthread_cond_wait(&cond_start,&lock);
    }
    if(i==1){
        sem_wait(sem4);
        info(BEGIN,4,i);

    }else{
        info(BEGIN,4,i);
    }
    if(i==4){
        start = 1;
        pthread_cond_signal(&cond_start);
    }
    pthread_mutex_unlock(&lock);
    
    pthread_mutex_lock(&lock);
    while(i == 4 && finish==0){
        pthread_cond_wait(&cond_finish,&lock);
    }
    info(END,4,i);
    if(i==2){
        finish = 1;
        pthread_cond_signal(&cond_finish);
    }
    if(i==1){
        sem_post(sem2);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}
void* different_processes(void* args){
    int i = (int)(size_t)args;
    sem_wait(sem1);
    if(i==1){
        sem_post(sem1);
        sem_wait(sem2);
    }
    info(BEGIN,7,i);
    sem_post(sem1);

    sem_wait(sem3);
    info(END,7,i);
    if(i==2){
        sem_post(sem4);
    }
    sem_post(sem3);
    return NULL;
}

void* threads_barrier(void* args){
    int i=(int)(size_t)args;
    sem_wait(&mutex);
    if(i!=10){
        waiting_for_ten++;
        sem_post(&mutex);
        sem_wait(&mutex_10);
    }else{
        sem_post(&mutex);
    }
    sem_wait(&simultaneously);
    info(BEGIN,6,i);
    sem_wait(&mutex);  
    threads_at_barrier++;
    if(threads_at_barrier == N){
        sem_post(&barrier10);
    }
    if(i==10){
        for(int j=0;j<waiting_for_ten;j++){
            sem_post(&mutex_10);
        }
        sem_post(&mutex);
        sem_wait(&barrier10);
    }else{
        sem_post(&mutex);
        sem_wait(&barrier);     
    }
    if(i==10){
        info(END,6,i);
        for(int j=0;j<36;j++){
            sem_post(&barrier);
            sem_post(&mutex_10);
        }
        sem_post(&simultaneously);
    }else{
        info(END,6,i);
        sem_post(&simultaneously);
    }
    return NULL;
}

int main(){

    sem1 = sem_open("/handra_andrei",O_CREAT,0777,1);
    if(sem1 == SEM_FAILED){
        perror("Cannot create semaphore");
        return -1;
    }
    sem2 = sem_open("/handra_andrei_2",O_CREAT,0777,0);
    if(sem2==SEM_FAILED){
        perror("Cannot create semaphore");
        return -1;
    }
    sem3 = sem_open("/handra_andrei_3",O_CREAT,0777,1);
    if(sem3 == SEM_FAILED){
        perror("Cannot create semaphore");
        return -1;
    }
    sem4 = sem_open("/handra_andrei_4",O_CREAT,0777,0);
    if(sem4==SEM_FAILED){
        perror("Cannot create semaphore");
        return -1;
    }    
    sem_init(sem1,2,1);
    sem_init(sem2,2,0);
    sem_init(sem3,2,1);
    sem_init(sem4,2,0);
    init();

    info(BEGIN, 1, 0);
    pid_t p2;
    pid_t p3;
    pid_t p4;
    pid_t p5;
    pid_t p6;
    pid_t p7;
    p2=fork();
    switch(p2){
        case -1:
            perror("Cannot create a new child");
            exit(1);
        case 0:
            //p2, child of process p1
            info(BEGIN,2,0);
            p6=fork();
            switch(p6){
                case -1:
                    perror("Cannot create a new child");
                    exit(1);
                case 0:
                    //p6, child of process p2
                    info(BEGIN,6,0);
                    int sem_error1 = sem_init(&mutex,0,1);
                    if(sem_error1){
                        printf("sem_init: %d",sem_error1);
                        return -1;
                    }
                    int sem_error2 = sem_init(&barrier,0,0);
                    if(sem_error2){
                        printf("sem_init: %d",sem_error2);
                        return -1;
                    }
                    int sem_error3 = sem_init(&mutex_10,0,0);
                    if(sem_error3){
                        printf("sem_init: %d",sem_error3);
                        return -1;
                    }
                    int sem_error4 = sem_init(&simultaneously,0,6);
                    if(sem_error4){
                        printf("sem_init: %d",sem_error4);
                        return -1;
                    }
                    int sem_error5 = sem_init(&barrier10,0,0);
                    if(sem_error5){
                        printf("sem_init: %d",sem_error5);
                        return -1;
                    }
                    pthread_t* threads6 = (pthread_t*)malloc(sizeof(pthread_t)*37);
                    for(int i=1;i<=36;i++){
                        pthread_t tid = 0;
                        int error = pthread_create(&tid,NULL,threads_barrier,(void*)(size_t)i);
                        if(error){
                            printf("Cannot create thread %d",error);
                            return -1;
                        }
                        threads6[i]=tid;
                    }

                    for(int i=1;i<=36;i++){
                        pthread_join(threads6[i],NULL);
                    }
                    info(END,6,0);
                    sem_destroy(&mutex);
                    sem_destroy(&barrier);
                    sem_destroy(&mutex_10);
                    sem_destroy(&barrier10);
                    sem_destroy(&simultaneously);
                    break;
                default:
                    //parent: process p2
                    waitpid(p6,NULL,0);
                    info(END,2,0);
                    break;
            }
           break;
        default:
            //parent: p1
            p3=fork();
            switch(p3){
                case -1:
                    perror("Cannot create a new child");
                    exit(1);
                case 0:
                    //p3, child of process p1
                    info(BEGIN,3,0);

                    info(END,3,0);
                    break;
                default:
                    //parent:p1
                    p4=fork();
                    switch(p4){
                        case -1:
                            perror("Cannot create a new child");
                            exit(1);
                        case 0:
                            //p4, child of process p1
                            info(BEGIN,4,0);
                            pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*6);
                            for(int i=1;i<=5;i++){
                                pthread_t tid=0;
                                int error = pthread_create(&tid,NULL,same_process,(void*)(size_t)i);
                                if(error){
                                    printf("Cannot create thread %d",error);
                                    return -1;
                                }
                                threads[i]=tid;
                            }
                            
                            for(int i=1;i<=5;i++){
                                pthread_join(threads[i],NULL);
                            }
                            info(END,4,0);
                            break;
                        default:
                            //parent p1
                            p5=fork();
                            switch(p5){
                                case -1:
                                    perror("Cannot create a new child");
                                    exit(1);
                                case 0:
                                    //p5, child of process p1
                                    info(BEGIN,5,0);
                                    p7=fork();
                                    switch(p7){
                                        case -1:
                                            perror("Cannot create a new child");
                                            exit(1);
                                        case 0:
                                            //p7, child of process p5
                                            info(BEGIN,7,0);
                                            pthread_t* threads7 = (pthread_t*)malloc(sizeof(pthread_t)*5);
                                            for(int i=1;i<=4;i++){
                                                pthread_t tid2=0;
                                                int error2 = pthread_create(&tid2,NULL,different_processes,(void*)(size_t)i);
                                                if(error2){
                                                    printf("Cannot create thread %d",error2);
                                                    return -1;
                                                }
                                                threads7[i]=tid2;
                                            }
                            
                                            for(int i=1;i<=4;i++){
                                                pthread_join(threads7[i],NULL);
                                            }
                                            info(END,7,0);
                                            break;
                                        default:
                                            //parent p5
                                            waitpid(p7,NULL,0);
                                            info(END,5,0);
                                            break;
                                    }
                                    break;
                                default:
                                    //parent p1
                                    waitpid(p5,NULL,0);
                                    waitpid(p4,NULL,0);
                                    waitpid(p3,NULL,0);
                                    waitpid(p2,NULL,0);
                                    info(END,1,0);
                                    break;   
                            }
                            break;
                    }
                    break;
            }
            break;
    }
    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);
    sem_close(sem4);
    unlink("/handra_andrei");
    unlink("/handra_andrei_2");
    unlink("/handra_andrei_3");
    unlink("/handra_andrei_4");
    return 0;
}
