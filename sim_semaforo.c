/*
 * Lab 5 - Sistemas Operativos
 * Simulador de recursos con semaforos POSIX
 *
 * Compilar: gcc -o sim_semaforo sim_semaforo.c -lpthread
 * Ejecutar: ./sim_semaforo
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define POOL_SIZE    4
#define TRABAJADORES 8
#define CICLOS       3
#define ESPERA_MAX   600

sem_t pool;
pthread_mutex_t log_mtx;
FILE *bitacora;
int nlinea = 1;

void registrar(const char *texto) {
    pthread_mutex_lock(&log_mtx);
    fprintf(bitacora, "[%03d] %s\n", nlinea, texto);
    printf("[%03d] %s\n", nlinea, texto);
    nlinea++;
    fflush(bitacora);
    pthread_mutex_unlock(&log_mtx);
}

void adquirir_recurso(int id) {
    char msg[128];
    snprintf(msg, sizeof(msg), "W%d -> bloqueado esperando recurso", id);
    registrar(msg);

    sem_wait(&pool);

    int disponibles;
    sem_getvalue(&pool, &disponibles);
    snprintf(msg, sizeof(msg), "W%d -> recurso adquirido (quedan %d libres)", id, disponibles);
    registrar(msg);
}

void liberar_recurso(int id) {
    sem_post(&pool);

    int disponibles;
    sem_getvalue(&pool, &disponibles);

    char msg[128];
    snprintf(msg, sizeof(msg), "W%d -> recurso liberado  (quedan %d libres)", id, disponibles);
    registrar(msg);
}

void trabajar(int id, int ciclo) {
    int ms = (rand() % ESPERA_MAX) + 100;
    char msg[128];
    snprintf(msg, sizeof(msg), "W%d [ciclo %d] -> trabajando %dms...", id, ciclo, ms);
    registrar(msg);

    struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

void *worker(void *arg) {
    int id = *(int *)arg;
    free(arg);

    char msg[128];
    snprintf(msg, sizeof(msg), "W%d -> iniciado", id);
    registrar(msg);

    for (int c = 1; c <= CICLOS; c++) {
        adquirir_recurso(id);
        trabajar(id, c);
        liberar_recurso(id);
    }

    snprintf(msg, sizeof(msg), "W%d -> terminado", id);
    registrar(msg);
    return NULL;
}

int main(void) {
    srand(time(NULL));

    bitacora = fopen("log_semaforo.txt", "w");
    if (!bitacora) { perror("fopen"); return 1; }

    pthread_mutex_init(&log_mtx, NULL);
    sem_init(&pool, 0, POOL_SIZE);

    char msg[128];
    snprintf(msg, sizeof(msg), "Inicio | pool=%d workers=%d ciclos=%d", POOL_SIZE, TRABAJADORES, CICLOS);
    registrar(msg);

    pthread_t tids[TRABAJADORES];
    for (int i = 0; i < TRABAJADORES; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&tids[i], NULL, worker, id);
    }

    for (int i = 0; i < TRABAJADORES; i++)
        pthread_join(tids[i], NULL);

    registrar("Fin del programa");

    sem_destroy(&pool);
    pthread_mutex_destroy(&log_mtx);
    fclose(bitacora);
    return 0;
}
