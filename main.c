#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#define WIDTH 512
#define HEIGHT 512
#define MAX_THREADS 4 //amúgy se adtam tán többet a vm-nek

unsigned char inputImage[HEIGHT][WIDTH]; //szinek miatt uns 0-255
unsigned char outputImage[HEIGHT][WIDTH];
unsigned char sequentialOutput[HEIGHT][WIDTH];

int rows_processed_by_thread[MAX_THREADS];
int pixels_processed_by_thread[MAX_THREADS];

//blur: előző pixel aktuális meg köv pixel átlagolunk
void blur_row(unsigned char src[HEIGHT][WIDTH], unsigned char dest[HEIGHT][WIDTH], int row) {
    for (int col = 1; col < WIDTH - 1; col++) {
        dest[row][col] = (
            src[row][col - 1] +
            src[row][col] +
            src[row][col + 1]
        ) / 3;
    }
}

//szál fgv, void*->hogy tetsz. típ. adhassunk vissza
void* worker(void* arg) {
    //3 elemű tömb szétbontása elemeire
    int* data = (int*)arg;
    int thread_id = data[0];
    int start_row = data[1];
    int end_row = data[2];
    free(arg); //a memoria miatt (malloc, heap-en (kukac-kupac))

    for (int row = start_row; row < end_row; row++) {
        if (row > 0 && row < HEIGHT - 1) {
            blur_row(inputImage, outputImage, row);
            rows_processed_by_thread[thread_id]++;
            pixels_processed_by_thread[thread_id] += WIDTH - 2;
        }
    }

    return NULL;
}

void sequential_blur() {
    for (int row = 1; row < HEIGHT - 1; row++) {
        blur_row(inputImage, sequentialOutput, row);
    }
}

//nagyon pontosan adja meg az eltelt időt 
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

//vizualizálás
void save_pgm(const char* filename, unsigned char image[HEIGHT][WIDTH]) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Nem sikerült megnyitni a PGM fájlt");
        return;
    }

    fprintf(f, "P5\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(image, sizeof(unsigned char), WIDTH * HEIGHT, f);
    fclose(f);
}


int main() {
    //pitonhoz
    FILE* fp = fopen("performance.csv", "w");
    if (!fp) {
        perror("Nem sikerült megnyitni a CSV fájlt");
        return 1;
    }

    fprintf(fp, "Threads,SequentialTime,ParallelTime,Speedup,Efficiency\n");

    // Véletlen kép generálása egyszer
    for (int i = 0; i < HEIGHT; i++)
        for (int j = 0; j < WIDTH; j++)
            inputImage[i][j] = rand() % 256;

    //sorba végig megy a szálakon, minden itérációban egy szálat indít
    for (int thread_count = 1; thread_count <= MAX_THREADS; thread_count++) {
        memset(rows_processed_by_thread, 0, sizeof(rows_processed_by_thread)); //memset 0zás miatt, hogy korábbi adat ne maradjon
        memset(pixels_processed_by_thread, 0, sizeof(pixels_processed_by_thread));

        //tárolja a szálak követéséhez szük. vált., max thread van mindig, de csak countnyit használunk
        pthread_t threads[MAX_THREADS];

        // Szekvenciális blur (egyszer mérjük)
        static double sequential_time = 0.0;
        if (thread_count == 1) {
            struct timespec seq_start, seq_end;
            clock_gettime(CLOCK_MONOTONIC, &seq_start); //indítjuk az időmérést
            sequential_blur(); //itt fut
            clock_gettime(CLOCK_MONOTONIC, &seq_end); //zárjuk
            sequential_time = get_time_diff(seq_start, seq_end);
        }

        // Párhuzamos blur mérése
        struct timespec par_start, par_end;
        clock_gettime(CLOCK_MONOTONIC, &par_start); //clock mono: nem áll vissza/nem függ a rendszeridőtől

        int total_rows_to_process = HEIGHT - 2; //csak a feldolgozható sorokat osztjuk el
        int rows_per_thread = total_rows_to_process / thread_count;
        int extra_rows = total_rows_to_process % thread_count;
        int current_row = 1; //1-től indul, nem 0-tól!
        //köv sor index, amit kiosztunk egy szálnak

        for (int i = 0; i < thread_count; i++) {
            int start_row = current_row;
            int end_row = start_row + rows_per_thread + (i < extra_rows ? 1 : 0); //fancy kód, csak annyi, hogy az első pár szál megkapja a maradék sorokat, ha van
            current_row = end_row;
            
            //itt van a kis dinamikusan foglalt tömb, amit majd szétszedünk
            int* data = malloc(3 * sizeof(int));
            data[0] = i;
            data[1] = start_row;
            data[2] = end_row;

            pthread_create(&threads[i], NULL, worker, data); //elindítjuk az i-edik szálat
        }

        for (int i = 0; i < thread_count; i++) {
            pthread_join(threads[i], NULL);
        } //minden szálat meg kell várni, mielőtt tovább lépne

        clock_gettime(CLOCK_MONOTONIC, &par_end); //az idő, amikor minden szál kész van
        double parallel_time = get_time_diff(par_start, par_end); //telj par fut idő

        //számítások
        double speedup = sequential_time / parallel_time;
        double efficiency = speedup / thread_count;

        //eredmények kiírása csv-be
        fprintf(fp, "%d,%.6f,%.6f,%.2f,%.2f\n",
                thread_count,
                sequential_time,
                parallel_time,
                speedup,
                100.0 * efficiency);

        printf("\nFutás %d szállal kész\n", thread_count);

        // Statisztikák kiírása
        printf("\n--- STATISZTIKA (%d szál) ---\n", thread_count);
        printf("Szekvenciális futási idő: %.6f másodperc\n", sequential_time);
        printf("Párhuzamos futási idő: %.6f másodperc\n", parallel_time);
        printf("Gyorsulás (Speedup): %.2f\n", speedup);
        printf("Hatékonyság (Efficiency): %.2f%%\n", 100.0 * efficiency);

        int total_rows = 0;
        int total_pixels = 0;
        for (int i = 0; i < thread_count; i++) {
            total_rows += rows_processed_by_thread[i];
            total_pixels += pixels_processed_by_thread[i];
        }

        printf("Feldolgozott sorok: %d\n", total_rows);
        printf("Feldolgozott pixelek: %d\n", total_pixels);
        printf("Szálak száma: %d\n", thread_count);
        for (int i = 0; i < thread_count; i++) {
            printf("Szál #%d - sorok: %d, pixelek: %d\n",
                   i, rows_processed_by_thread[i], pixels_processed_by_thread[i]);
        }

        if (outputImage[100][100] == sequentialOutput[100][100]) {
            printf("outputImage[100][100] = %d (egyezik a szekvenciálissal)\n", outputImage[100][100]);
        } else {
            printf("outputImage[100][100] = %d, de sequentialOutput = %d\n",
                   outputImage[100][100], sequentialOutput[100][100]);
        }

        //mentés képfájlba (csak utolsó iterációban)
        if (thread_count == MAX_THREADS) {
            save_pgm("inputImage.pgm", inputImage);
            save_pgm("outputImage.pgm", outputImage);
            save_pgm("sequentialOutput.pgm", sequentialOutput);
        }
    }

    fclose(fp);
    printf("Eredmények elmentve a performance.csv fájlba.\n");
    printf("Képek elmentve PGM formátumban.\n");
    return 0;
}
