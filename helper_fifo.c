#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MIN_ISSUE_DELAY_TIME 100
#define MAX_ISSUE_DELAY_TIME 500
#define BUFFER_SIZE 3

typedef struct request {
    int request_id;
    double arrival_time;
    double burst_time;
} request_t;

request_t buffer[BUFFER_SIZE];
int use = 0, fill = 0, size = 0;

sem_t mutex, empty, exist;

request_t create_request(int min_time_process, int max_time_process) {
    request_t request;
    request.request_id = -1;
    request.arrival_time = -1;
    request.burst_time = rand() % (max_time_process + 1 - min_time_process) + min_time_process;
    return request;
}

void enqueue(request_t request) {
    buffer[fill] = request; size++;
    fill = (fill + 1) % BUFFER_SIZE;
}

request_t dequeue() {
    request_t request = buffer[use]; size--;
    use = (use + 1) % BUFFER_SIZE; 
    return request;
}

void queue_to_string() {
    if(size == 0) {
        printf(": buffer is empty\n\n");
    } else {
        printf(": ");
        int i = 0, front = use;
        while(i < size) {
            printf("%d ", buffer[front].request_id);
            front = (front + 1) % BUFFER_SIZE;
            i++;
        }
        printf("\n\n");
    }
}

int msleep(unsigned int tms) {
    return usleep(tms * 1000);
}

unsigned int seed_gen(long int tid) {
    return tid * time(0);
}


