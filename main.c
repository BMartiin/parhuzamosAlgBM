#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define WIDTH 800
#define HEIGHT 800
#define FRAMES 60
#define MAX_BASE_ITER 500 //részletesség
#define MAX_EXTRA_ITER 1000 //ez dinamikusság miatt kell mert a közepe pl sűrűbb a fraktálnak, ott részletesebb kell
#define MAX_THREADS 4 

typedef struct {
    unsigned char r, g, b;
} Color; //képpont struktúra rgb

//matekos: megnézi konvergál e pont, vagyis benne van e halmazba, he nem akkor hány iteráció után hagyja el a halmazt
int burning_ship(double cx, double cy, int max_iter) {
    double x = 0.0, y = 0.0;
    int iter = 0;
    while (x * x + y * y <= 4.0 && iter < max_iter) {
        double xtemp = x * x - y * y + cx;
        y = fabs(2.0 * x * y) + cy;
        x = fabs(xtemp); //mandelbrot halmaz módosított vált., komplex szám absz ért.
        iter++;
    }
    return iter;
}

//a fraktál iterációs eredményéből egy színt generál
Color get_color(int iter, int max_iter) {
    Color color; //-> vált. tart. az adott képpont színét 
    //ami a hlmazon belül maradt, azt feketének színezi
    if (iter == max_iter) {
        color.r = color.g = color.b = 0;
    } 
    //amik kiugrottak t -> milyen gyorsan, szín, csak szebb így
    else {
        double t = (double)iter / max_iter;
        color.r = (unsigned char)(9 * (1 - t) * t * t * t * 255);
        color.g = (unsigned char)(15 * (1 - t) * (1 - t) * t * t * 255);
        color.b = (unsigned char)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
    }
    return color;
}

//egy adott kékocka mentése ppm formátumban
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

// Egyszálú szekvenciális referencia futás !csak az első képkockára!
double run_sequential_frame() {
    Color image[HEIGHT][WIDTH];
    double zoom = 1.0;
    double scale = 3.0 * zoom;
    double center_x = -1.8, center_y = -0.01;
    double x_start = center_x - scale / 2.0;
    double y_start = center_y - scale / 2.0;
    double step = scale / WIDTH;

    double t0 = omp_get_wtime(); //időmérés kezdete

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
        }
    }

    double t1 = omp_get_wtime();
    return t1 - t0;
}

int main() {
    Color image[HEIGHT][WIDTH];
    double zoom = 1.0;
    double zoom_factor = 0.95; //minden kékocka után a zoomot ezzel szorozza
    double center_x = -1.8, center_y = -0.01; //a fraktál középpontja (komplex síkon)

    int thread_work[MAX_THREADS] = {0};

    double sequential_time = run_sequential_frame();

    double start_time = omp_get_wtime();

    //futtatja a kékockagenerálást
    for (int frame = 0; frame < FRAMES; frame++) {
        //koordináta rendszer beállítása
        double scale = 3.0 * zoom;
        double x_start = center_x - scale / 2.0;
        double y_start = center_y - scale / 2.0;
        double step = scale / WIDTH;

        //párhu. ciklusok, összelapítja a kettőt (collapse) " 1 naggyá"
        #pragma omp parallel for collapse(2) schedule(dynamic)
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                double cx = x_start + x * step;
                double cy = y_start + y * step;

                int max_iter = MAX_BASE_ITER;
                if (x > WIDTH / 3 && x < 2 * WIDTH / 3 &&
                    y > HEIGHT / 3 && y < 2 * HEIGHT / 3) {
                    max_iter += MAX_EXTRA_ITER; //iter. küszöböt emeljük középen, mert ott több minden van
                }

                int iter = burning_ship(cx, cy, max_iter);
                image[y][x] = get_color(iter, max_iter); //színez

                int tid = omp_get_thread_num(); //aktuális szál azonosítója
                #pragma omp atomic //versenyhelyzet-mentes
                thread_work[tid]++; //szálak munkamegoszlása szál/db
            }
        }
        //kép mentés
        save_image(image, frame);
        //zoom növelése
        zoom *= zoom_factor;
        printf("Completed frame: %d/%d\n", frame + 1, FRAMES);
    }
    //időmérés vége
    double end_time = omp_get_wtime();
    double parallel_time = end_time - start_time;

    int num_threads = omp_get_max_threads(); //szálak számának lekérd (mennyit használt a prog)
    int total_pixels = WIDTH * HEIGHT * FRAMES;

    double speedup = (sequential_time * FRAMES) / parallel_time;
    double efficiency = speedup / num_threads;

    printf("\n=== Teljesítmény statisztika ===\n");
    printf("Szálak száma        : %d\n", num_threads);
    printf("Képkockák száma     : %d\n", FRAMES);
    printf("Kép felbontás       : %dx%d px\n", WIDTH, HEIGHT);
    printf("Teljes feldolgozott : %d pixel\n", total_pixels);
    printf("Szekvenciális idő   : %.2f másodperc (1 kép)\n", sequential_time);
    printf("Párhuzamos idő      : %.2f másodperc (összes kép)\n", parallel_time);
    printf("Gyorsulás (Speedup) : %.2f\n", speedup);
    printf("Hatékonyság         : %.2f%%\n", efficiency * 100);
    printf("Átlag idő/képkocka  : %.2f másodperc\n", parallel_time / FRAMES);

    printf("\n=== Szálak munkamegoszlása ===\n");
    for (int i = 0; i < num_threads; i++) {
        double ratio = 100.0 * thread_work[i] / total_pixels;
        printf("Szál %2d: %8d pixel (%.2f%%)\n", i, thread_work[i], ratio);
    }
    printf("================================\n");

    return 0;
}
