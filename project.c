#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "common.h"
#include "helper_fifo.c"
// #include "helper_sjf.c"

int producer_num, consumer_num, request_num, min_time_process, max_time_process, act;
int success_push = 0, success_pop = 0, push = 0, pop = 0;
int current_id = 0;

double launch_time;
double average_waiting_time = 0, total_time_simulation = 0, reject = 0, drop = 0;

sem_t count_push, count_pop, inspect_size;

void init() {   
    scanf("%d", &producer_num);
    scanf("%d", &consumer_num);
    scanf("%d", &request_num);
    scanf("%d", &min_time_process);
    scanf("%d", &max_time_process);
    scanf("%d", &act);
    launch_time = GetTime();
    printf("\n");
}

int queue_is_full() {
    int isFull = 0;
    sem_wait(&inspect_size);
    if(size >= BUFFER_SIZE) {
        isFull = 1;
        if(act == 2) {
            reject++;
        } else if(act == 3) {
            reject++; drop++;
        }
    }
    sem_post(&inspect_size);
    return isFull;
}   

void push_detail(long int tid, request_t *request, double dt) {
    request->request_id = current_id++;
    request->arrival_time = (GetTime() - launch_time) / 0.001;
    printf("[tid: %03ld] produce: %d - at %.2f ms, dt %.2f ms\n\n", 
        tid, request->request_id, request->arrival_time, dt);
}

void pop_detail(long int tid, request_t *request) {
    double st = 0, ft = 0, wt = 0;
    st = (GetTime() - launch_time) / 0.001;
    wt = st - request->arrival_time;
    ft = st + request->burst_time;
    average_waiting_time += wt;
    total_time_simulation = ft;
    printf("[tid: %03ld] consume: %d - wt %.2f ms, st %.2f ms, ft %.2f ms, bt %.2f ms\n\n", 
        tid, request->request_id, wt, st, ft, request->burst_time);
}

void *producer(void *args) {
    long int tid = (long int)args;
    int tmp = 0;
    srand(seed_gen(tid));
    while(1) {
        
        if(act == 1) {
            sem_wait(&count_push); 
            tmp = push++; 
            sem_post(&count_push);
            if(tmp >= request_num) break; 
        }
        
        double dt = rand() % (MAX_ISSUE_DELAY_TIME + 1 - MIN_ISSUE_DELAY_TIME) + MIN_ISSUE_DELAY_TIME;
        msleep(dt);
        
        request_t request = create_request(min_time_process, max_time_process);
        if(act == 1) {
            sem_wait(&empty);
            sem_wait(&mutex);
            ////////////////////////////////////////////////////////////
            push_detail(tid, &request, dt);
            enqueue(request);
            success_push++;
            queue_to_string();
            ////////////////////////////////////////////////////////////
            sem_post(&mutex);
            sem_post(&exist);
        } else {
            if(queue_is_full()) {
                if(act == 2) {
                    sem_wait(&mutex);
                    ////////////////////////////////////////////////////////////
                    printf("[tid: %03ld] request %d is rejected...\n\n", tid, current_id);
                    current_id++;
                    queue_to_string();
                    ////////////////////////////////////////////////////////////
                    sem_post(&mutex);
                } else {
                    sem_wait(&mutex);
                    ////////////////////////////////////////////////////////////
                    int oldest_id = buffer[use].request_id;
                    request.request_id = current_id;
                    request.arrival_time = (GetTime() - launch_time) / 0.001;
                    average_waiting_time += request.arrival_time - buffer[use].arrival_time;

                    size--;
                    enqueue(request); 
                    use = (use + 1) % BUFFER_SIZE;
                    printf("[tid: %03ld] replaced request %d with request %d...\n\n", tid, oldest_id, request.request_id);
                    current_id++;
                    queue_to_string();
                    ////////////////////////////////////////////////////////////
                    sem_post(&mutex);
                }
            } else {
                sem_wait(&mutex);
                ////////////////////////////////////////////////////////////
                if(success_push >= request_num) {
                    sem_post(&mutex);
                    break;
                }
                push_detail(tid, &request, dt);
                enqueue(request);
                success_push++;
                queue_to_string();
                ////////////////////////////////////////////////////////////
                sem_post(&mutex);
                sem_post(&exist);
            }
        }
    }
    
    return NULL;
}

void *consumer(void *args) {
    long int tid = (long int)args;
    int tmp = 0;
    srand(seed_gen(tid));
    while(1) {
        
        sem_wait(&count_pop);
        tmp = pop++;
        sem_post(&count_pop);

        if(tmp >= request_num) break;

        request_t request;
        if(act == 1) {
            sem_wait(&exist);
            sem_wait(&mutex);
            ////////////////////////////////////////////////////////////
            request = dequeue();
            success_pop++;
            pop_detail(tid, &request);
            queue_to_string();
            ////////////////////////////////////////////////////////////
            sem_post(&mutex);
            sem_post(&empty);        
        } else {
            sem_wait(&exist);
            sem_wait(&mutex);
            ////////////////////////////////////////////////////////////
            request = dequeue();
            success_pop++;
            pop_detail(tid, &request);
            queue_to_string();
            ////////////////////////////////////////////////////////////
            sem_post(&mutex);
        }

        msleep(request.burst_time);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    
    init();
    sem_init(&mutex, 0, 1);
    sem_init(&count_push, 0, 1);
    sem_init(&count_pop, 0, 1);
    sem_init(&inspect_size, 0, 1);
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&exist, 0, 0);
    
    int thread_num = producer_num + consumer_num;
    pthread_t thread[thread_num];
    
    for(int i = 0; i < thread_num; i++) {
        if(i < producer_num) {
            pthread_create(&thread[i], NULL, &producer, (void*)(long int)i);
        } else {
            pthread_create(&thread[i], NULL, &consumer, (void*)(long int)i);
        }
    }
    
    for(int i = 0; i < thread_num; i++) {
        pthread_join(thread[i], NULL);
    }
    
    average_waiting_time /= (success_pop + drop);

    printf("\n");
    printf(": average waiting time: %.2f ms\n", average_waiting_time);
    printf(": rejected / dropped requests (%%): %.2f %%\n", reject / (request_num + reject) * 100);
    printf(": total time: %.2f ms\n", total_time_simulation);

    printf("\n");
    printf(": push: %d\n", success_push);
    printf(": pop: %d\n", success_pop);
    printf(": done\n");

    sem_destroy(&mutex);
    sem_destroy(&count_push);
    sem_destroy(&count_pop);
    sem_destroy(&inspect_size);
    sem_destroy(&empty);
    sem_destroy(&exist);

    return 0;
}
