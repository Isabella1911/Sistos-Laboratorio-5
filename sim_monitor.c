/*
 * Lab 5 - Sistemas Operativos
 * Simulador de recursos con monitor (mutex + cond)
 *
 * Compilar: gcc -o sim_monitor sim_monitor.c -lpthread
 * Ejecutar: ./sim_monitor
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define CAPACIDAD    8
#define TRABAJADORES 5
#define CICLOS       4
#define MAX_PEDIDO   3
#define ESPERA_MAX   500

/* --- Monitor --- */
static int stock = CAPACIDAD;
static pthread_mutex_t mtx_monitor = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_stock  = PTHREAD_COND_INITIALIZER;

/* --- Log --- */
static pthread_mutex_t mtx_log = PTHREAD_MUTEX_INITIALIZER;
static FILE *log_fp;
static int linea = 1;

void log_write(const char *s) {
    pthread_mutex_lock(&mtx_log);
    fprintf(log_fp, "%03d | %s\n", linea, s);
    printf("%03d | %s\n", linea, s);
    linea++;
    fflush(log_fp);
    pthread_mutex_unlock(&mtx_log);
}

/*
 * decrease_count: toma `n` recursos del stock.
 * Bloquea al hilo si no hay suficientes hasta que otro los libere.
 */
int decrease_count(int wid, int n) {
    char buf[200];

    pthread_mutex_lock(&mtx_monitor);
    snprintf(buf, sizeof(buf), "W%d | decrease(%d) | entrando al monitor | stock=%d", wid, n, stock);
    log_write(buf);

    while (stock < n) {
        snprintf(buf, sizeof(buf), "W%d | decrease(%d) | SIN STOCK (hay %d) -> cond_wait", wid, n, stock);
        log_write(buf);
        pthread_cond_wait(&cond_stock, &mtx_monitor);
        snprintf(buf, sizeof(buf), "W%d | decrease(%d) | despertado -> stock=%d", wid, n, stock);
        log_write(buf);
    }

    stock -= n;
    snprintf(buf, sizeof(buf), "W%d | decrease(%d) | OK -> stock restante=%d", wid, n, stock);
    log_write(buf);

    pthread_mutex_unlock(&mtx_monitor);
    return 0;
}

/*
 * increase_count: devuelve `n` recursos y notifica a hilos en espera.
 */
int increase_count(int wid, int n) {
    char buf[200];

    pthread_mutex_lock(&mtx_monitor);
    stock += n;
    snprintf(buf, sizeof(buf), "W%d | increase(%d) | stock ahora=%d -> broadcast", wid, n, stock);
    log_write(buf);
    pthread_cond_broadcast(&cond_stock);
    pthread_mutex_unlock(&mtx_monitor);
    return 0;
}

void *worker(void *arg) {
    int id = *(int *)arg;
    free(arg);

    char buf[200];
    snprintf(buf, sizeof(buf), "W%d | iniciado", id);
    log_write(buf);

    for (int c = 1; c <= CICLOS; c++) {
        int pedido = (rand() % MAX_PEDIDO) + 1;

        snprintf(buf, sizeof(buf), "W%d | ciclo %d | pide %d recurso(s)", id, c, pedido);
        log_write(buf);

        decrease_count(id, pedido);

        int ms = (rand() % ESPERA_MAX) + 80;
        struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
        nanosleep(&t, NULL);

        snprintf(buf, sizeof(buf), "W%d | ciclo %d | devuelve %d recurso(s)", id, c, pedido);
        log_write(buf);

        increase_count(id, pedido);
    }

    snprintf(buf, sizeof(buf), "W%d | finalizado", id);
    log_write(buf);
    return NULL;
}

int main(void) {
    srand(time(NULL));

    log_fp = fopen("log_monitor.txt", "w");
    if (!log_fp) { perror("fopen"); return 1; }

    char buf[200];
    snprintf(buf, sizeof(buf), "CONFIG | capacidad=%d workers=%d ciclos=%d max_pedido=%d",
             CAPACIDAD, TRABAJADORES, CICLOS, MAX_PEDIDO);
    log_write(buf);

    pthread_t tids[TRABAJADORES];
    for (int i = 0; i < TRABAJADORES; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&tids[i], NULL, worker, id);
    }

    for (int i = 0; i < TRABAJADORES; i++)
        pthread_join(tids[i], NULL);

    log_write("FIN");

    pthread_mutex_destroy(&mtx_monitor);
    pthread_cond_destroy(&cond_stock);
    pthread_mutex_destroy(&mtx_log);
    fclose(log_fp);
    return 0;
}
