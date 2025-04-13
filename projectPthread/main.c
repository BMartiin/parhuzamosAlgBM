#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define WIDTH 512
#define HEIGHT 512
#define THREAD_COUNT 4


//szurkearnyalatos kep (unsigned 0-255):
unsigned char inputImage[HEIGHT][WIDTH];
unsigned char outputImage[HEIGHT][WIDTH];

//feladat indexe es mutex 
int taskIndex = 0;
pthread_mutex_t taskMutex;

//szalankent feldolgozott sorok szama
int rows_processed_by_thread[THREAD_COUNT] = {0};
//szalankent feldolgozott pixelek szama
int pixels_processed_by_thread[THREAD_COUNT] = {0};

//egyszeru 1D box blur a sorokra
void blur_row(int row){
    for(int col=1; col<WIDTH-1;col++){
        outputImage[row][col] = (
            inputImage[row][col -1] +
            inputImage[row][col] + 
            inputImage[row][col + 1]
        ) / 3;
    }
}

void* worker(void* arg){
    int thread_id = *(int*)arg;

    while(1){
        pthread_mutex_lock(&taskMutex);
        if(taskIndex >= HEIGHT){
            pthread_mutex_unlock(&taskMutex);
            break;
        }
        int row = taskIndex++;
        pthread_mutex_unlock(&taskMutex);

        //elso es vagy az utolso sor kihagyhato vagy kulon kezelheto
        if(row > 0 && row < HEIGHT -1){
            blur_row(row);
        }
    }
    return NULL;
}

int main(){
    //tesztadat
    for(int i = 0; i < HEIGHT; i++)
        for(int j = 0; j < WIDTH; j++)
            inputImage[i][j] = rand()%256;

    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];
    pthread_mutex_init(&taskMutex, NULL);

    //start ido merese
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    //szalak inditasa
    for(int i = 0; i<THREAD_COUNT; i++){
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &thread_ids[i]);
    }

    //szalak megvarasa
    for(int i = 0; i<THREAD_COUNT; i++){
        pthread_join(threads[i], NULL);
    }

    //vege ido merese
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                           (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    pthread_mutex_destroy(&taskMutex);

    //meres kiirasa 
    printf("---TELJESITMENY STATISZTIKA---\n");
    printf("Teljes futasi ido: %.6f másodperc\n", elapsed_time);
    printf("Feldolgozott sorok szama: %d\n", HEIGHT);
    printf("Feldolgozott pixelek szama: %d\n", HEIGHT * WIDTH);
    printf("Szalak szama: %d\n", THREAD_COUNT);

    int total_processed = 0;
    for(int i = 0; i < THREAD_COUNT; i++){
        printf("Szal #%d - feldolgozott sorok: %d, feldolgozott pixelek: %d\n", i, rows_processed_by_thread[i], pixels_processed_by_thread[i]);
        total_processed += rows_processed_by_thread[i];
    }

    //kimeneti ellenorzes
    printf("outputImage[100][100] = %d\n", outputImage[100][100]);

    return 0;
}