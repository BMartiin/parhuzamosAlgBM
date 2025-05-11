#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define WIDTH 800
#define HEIGHT 800
#define FRAMES 60
#define MAX_BASE_ITER 500
#define MAX_EXTRA_ITER 1000
#define MAX_THREADS 128  // Biztonság kedvéért felső korlát

typedef struct {
    unsigned char r, g, b;
} Color;

int burning_ship(double cx, double cy, int max_iter) {
    double x = 0.0, y = 0.0;
    int iter = 0;
    while (x * x + y * y <= 4.0 && iter < max_iter) {
        double xtemp = x * x - y * y + cx;
        y = fabs(2.0 * x * y) + cy;
        x = fabs(xtemp);
        iter++;
    }
    return iter;
}

Color get_color(int iter, int max_iter) {
    Color color;
    if (iter == max_iter) {
        color.r = color.g = color.b = 0;
    } else {
        double t = (double)iter / max_iter;
        color.r = (unsigned char)(9 * (1 - t) * t * t * t * 255);
        color.g = (unsigned char)(15 * (1 - t) * (1 - t) * t * t * 255);
        color.b = (unsigned char)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    }
    return color;
}

void save_image(Color image[HEIGHT][WIDTH], int frame) {
    char filename[64];
    sprintf(filename, "burning_ship_frame_%03d.ppm", frame);
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("fopen");
        return;
    }
    fprintf(f, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            fprintf(f, "%d %d %d\n", image[y][x].r, image[y][x].g, image[y][x].b);
    fclose(f);
}

int main() {
    Color image[HEIGHT][WIDTH];
    double zoom = 1.0;
    double zoom_factor = 0.95;
    double center_x = -1.8, center_y = -0.01;

    int thread_work[MAX_THREADS] = {0};

    double start_time = omp_get_wtime();

    for (int frame = 0; frame < FRAMES; frame++) {
        double scale = 3.0 * zoom;
        double x_start = center_x - scale / 2.0;
        double y_start = center_y - scale / 2.0;
        double step = scale / WIDTH;

        #pragma omp parallel for collapse(2) schedule(dynamic)
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                double cx = x_start + x * step;
                double cy = y_start + y * step;

                int max_iter = MAX_BASE_ITER;
                if (x > WIDTH / 3 && x < 2 * WIDTH / 3 &&
                    y > HEIGHT / 3 && y < 2 * HEIGHT / 3) {
                    max_iter += MAX_EXTRA_ITER;
                }

                int iter = burning_ship(cx, cy, max_iter);
                image[y][x] = get_color(iter, max_iter);

                // Szálmunkaszámláló növelése
                int tid = omp_get_thread_num();
                #pragma omp atomic
                thread_work[tid]++;
            }
        }

        save_image(image, frame);
        zoom *= zoom_factor;
        printf("Completed frame: %d/%d\n", frame + 1, FRAMES);
    }

    double end_time = omp_get_wtime();
    double total_time = end_time - start_time;

    int num_threads = omp_get_max_threads();
    int total_pixels = WIDTH * HEIGHT * FRAMES;

    printf("\n=== Teljesítmény statisztika ===\n");
    printf("Szálak száma        : %d\n", num_threads);
    printf("Képkockák száma     : %d\n", FRAMES);
    printf("Kép felbontás       : %dx%d px\n", WIDTH, HEIGHT);
    printf("Teljes feldolgozott : %d pixel\n", total_pixels);
    printf("Teljes futási idő   : %.2f másodperc\n", total_time);
    printf("Átlag idő/képkocka  : %.2f másodperc\n", total_time / FRAMES);

    printf("\n=== Szálak munkamegoszlása ===\n");
    for (int i = 0; i < num_threads; i++) {
        double ratio = 100.0 * thread_work[i] / total_pixels;
        printf("Szál %2d: %8d pixel (%.2f%%)\n", i, thread_work[i], ratio);
    }
    printf("================================\n");

    return 0;
}
