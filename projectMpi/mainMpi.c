#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_URL_LEN 128
#define MAX_LINKS 8
#define STEP_COMM_INTERVAL 5
#define MAX_STEPS 30

// Szimulalt link-halmaz
const char* mock_web[][MAX_LINKS + 1] = {
    {"http://site0.com", "http://site1.com", "http://site2.com", NULL},
    {"http://site1.com", "http://site3.com", NULL},
    {"http://site2.com", "http://site4.com", NULL},
    {"http://site3.com", NULL},
    {"http://site4.com", "http://site5.com", NULL},
    {"http://site5.com", NULL},
};

int web_size = 6;

int already_visited(const char* url, char visited[][MAX_URL_LEN], int visited_count) {
    for (int i = 0; i < visited_count; ++i)
        if (strcmp(visited[i], url) == 0) return 1;
    return 0;
}

void mock_extract_links(const char* url, char result[][MAX_URL_LEN], int* count) {
    *count = 0;
    for (int i = 0; i < web_size; ++i) {
        if (strcmp(mock_web[i][0], url) == 0) {
            for (int j = 1; mock_web[i][j] != NULL; ++j) {
                strncpy(result[*count], mock_web[i][j], MAX_URL_LEN);
                (*count)++;
            }
            return;
        }
    }
    *count = 0;
}

int main(int argc, char** argv) {
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char queue[100][MAX_URL_LEN];
    int queue_size = 0;
    char visited[1000][MAX_URL_LEN];
    int visited_count = 0;

    // Minden processz kap egy kezdo URL-t
    if (rank < web_size) {
        strncpy(queue[queue_size++], mock_web[rank][0], MAX_URL_LEN);
    }

    int step = 0;
    double start_time = MPI_Wtime();  // idomeres kezdete

    while (queue_size > 0 && step < MAX_STEPS) {
        char url[MAX_URL_LEN];
        strncpy(url, queue[--queue_size], MAX_URL_LEN);

        if (already_visited(url, visited, visited_count)) continue;
        strncpy(visited[visited_count++], url, MAX_URL_LEN);

        // Linkek kinyerese
        char new_links[8][MAX_URL_LEN];
        int new_link_count = 0;
        mock_extract_links(url, new_links, &new_link_count);

        for (int i = 0; i < new_link_count; ++i) {
            if (!already_visited(new_links[i], visited, visited_count)) {
                strncpy(queue[queue_size++], new_links[i], MAX_URL_LEN);
            }
        }

        // Kommunikacio minden STEP_COMM_INTERVAL lepes utan
        if (step % STEP_COMM_INTERVAL == 0) {
            for (int dest = 0; dest < size; ++dest) {
                if (dest == rank) continue;
                MPI_Send(&new_link_count, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
                MPI_Send(new_links, MAX_URL_LEN * new_link_count, MPI_CHAR, dest, 1, MPI_COMM_WORLD);
            }
        }

        // Fogadas mas processzekbol
        MPI_Status status;
        int incoming_count;
        for (int source = 0; source < size; ++source) {
            if (source == rank) continue;
            MPI_Recv(&incoming_count, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);
            if (incoming_count > 0) {
                char buffer[8][MAX_URL_LEN];
                MPI_Recv(buffer, MAX_URL_LEN * incoming_count, MPI_CHAR, source, 1, MPI_COMM_WORLD, &status);
                for (int i = 0; i < incoming_count; ++i) {
                    if (!already_visited(buffer[i], visited, visited_count)) {
                        strncpy(queue[queue_size++], buffer[i], MAX_URL_LEN);
                    }
                }
            }
        }

        step++;
    }

    double end_time = MPI_Wtime();  // idomeres vege
    double duration = end_time - start_time;

    printf("[Process %d] URL-ek szama: %d | Idő: %.4f mp\n", rank, visited_count, duration);

    MPI_Finalize();
    return 0;
}