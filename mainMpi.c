#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_URL_LEN 128
#define MAX_LINKS 8
#define MAX_QUEUE 1000
#define MAX_VISITED 2000
#define MAX_WEB 6
#define TAG_COUNT 0
#define TAG_URLS 1

// Minta webhelyek
//első oszlop a kezdő URL, a többi oszlop a kimenő linkek
const char* mock_web[MAX_WEB][MAX_LINKS + 1] = {
    {"http://site0.com", "http://site1.com", "http://site2.com", NULL},
    {"http://site1.com", "http://site3.com", NULL},
    {"http://site2.com", "http://site4.com", NULL},
    {"http://site3.com", NULL},
    {"http://site4.com", "http://site5.com", NULL},
    {"http://site5.com", NULL}
};

int web_size = MAX_WEB;

int already_visited(const char* url, char visited[][MAX_URL_LEN], int visited_count) {
    for (int i = 0; i < visited_count; i++)
        if (strcmp(visited[i], url) == 0) //(stringes)
            return 1;
    return 0;
}
//megkeresi az adott url-t a mock_web tömbben és kimásolja belőle az összes linket a result tömbbe
void extract_links(const char* url, char result[][MAX_URL_LEN], int* count) {
    *count = 0;
    for (int i = 0; i < web_size; i++) {
        if (strcmp(mock_web[i][0], url) == 0) { 
            for (int j = 1; mock_web[i][j] != NULL; ++j) {
                strncpy(result[*count], mock_web[i][j], MAX_URL_LEN); //strncpy másolja a linket a result tömbbe
                (*count)++;
            }
            return;
        }
    }
}

int main(int argc, char** argv) {
    int rank, size;
    int sent_links = 0;
    int received_links = 0;

    // MPI inicializálás
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //akt. folyamat sorsz. (rank)
    MPI_Comm_size(MPI_COMM_WORLD, &size); //hány folyamat fut össz.

    char queue[MAX_QUEUE][MAX_URL_LEN]; //FIFO, amiket még nem dolgozott fel tárolja az akt. rank
    int queue_size = 0; //db url van benne

    char visited[MAX_VISITED][MAX_URL_LEN];
    int visited_count = 0;

    //ez bizt, hogy minden folyamat különböző kezdőponttal induljon
    if (rank < web_size) {
        strncpy(queue[queue_size++], mock_web[rank % web_size][0], MAX_URL_LEN);
        printf("[Rank %d] Kezdő URL: %s\n", rank, mock_web[rank % web_size][0]);
    }

    //minden folyamat bevárja a többit
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime(); //pontos idő, mpi óra

    //fő ciklus, 10 lépésben dolgozza fel a linkeket (meglátogat 1 url-t, küldi a többieknek a benne lévő linkekekt, fogadja a többi linket)
    for (int step = 0; step < 10; ++step) {
        printf("[Rank %d] Feldolgozás folyamatban...\n", rank);

        //ez LIFO
        if (queue_size > 0) {
            char url[MAX_URL_LEN];
            strncpy(url, queue[--queue_size], MAX_URL_LEN);
            //ha még nem volt, akkor kinyeri a hivatkozásokat és hozzá adja a visitidedhez
            if (!already_visited(url, visited, visited_count)) {
                strncpy(visited[visited_count++], url, MAX_URL_LEN);

                char new_links[MAX_LINKS][MAX_URL_LEN];
                int new_link_count = 0;
                //szétküldi minden más ranknak
                extract_links(url, new_links, &new_link_count);
                //megnézi van e beérkező üzenet
                for (int dest = 0; dest < size; ++dest) {
                    if (dest == rank) continue;
                    MPI_Send(&new_link_count, 1, MPI_INT, dest, TAG_COUNT, MPI_COMM_WORLD);
                    if (new_link_count > 0) {
                        MPI_Send(new_links, MAX_URL_LEN * new_link_count, MPI_CHAR, dest, TAG_URLS, MPI_COMM_WORLD);
                        sent_links += new_link_count;
                    }
                }
            }
        }

        for (int source = 0; source < size; ++source) {
            if (source == rank) continue;

            int flag = 0;
            MPI_Status status;
            MPI_Iprobe(source, TAG_COUNT, MPI_COMM_WORLD, &flag, &status);
            
            //beolvassa a dvarabszámot, majd url-ek szövegként tömbbe
            if (flag) {
                int incoming_count = 0;
                MPI_Recv(&incoming_count, 1, MPI_INT, source, TAG_COUNT, MPI_COMM_WORLD, &status);

                if (incoming_count > 0) {
                    char buffer[MAX_LINKS][MAX_URL_LEN];
                    MPI_Recv(buffer, MAX_URL_LEN * incoming_count, MPI_CHAR, source, TAG_URLS, MPI_COMM_WORLD, &status);

                    for (int i = 0; i < incoming_count; ++i) {
                        if (!already_visited(buffer[i], visited, visited_count)) {
                            strncpy(visited[visited_count++], buffer[i], MAX_URL_LEN);
                            strncpy(queue[queue_size++], buffer[i], MAX_URL_LEN);
                            received_links++;
                        }
                    }
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD); //bevárás
    }

    double end_time = MPI_Wtime();

    printf("\n[Rank %d] --- STATISZTIKA ---\n", rank);
    printf("Feldolgozott URL-ek: %d\n", visited_count);
    printf("Küldött linkek: %d\n", sent_links);
    printf("Kapott linkek: %d\n", received_links);
    printf("Idő: %.4f mp\n", end_time - start_time);

    // CSV fájl kiírása
    char filename[64];
    snprintf(filename, sizeof(filename), "stats_rank%d.csv", rank);
    FILE* f = fopen(filename, "w");
    if (f) {
        fprintf(f, "rank,visited,sent,received,time\n");
        fprintf(f, "%d,%d,%d,%d,%.4f\n", rank, visited_count, sent_links, received_links, end_time - start_time);
        fclose(f);
    }

    MPI_Finalize();
    return 0;
}
