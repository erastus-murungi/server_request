//
// Created by Erastus Murungi on 3/3/20.
//

#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


typedef struct {
    // input
    void *A;
    size_t len;
    void *output;
} thread_args;

#define SHIFT (5)

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static inline int64_t max(int64_t a, int64_t b) {
    return (a > b) ? a : b;
}

void print_array(int64_t *A, size_t na) {
    /**
     * simple helper to print an array in python style
     */
    if (na == 0) {
        printf("[]");
        return;
    }
    int i;

    printf("[");
    for (i = 0; i < (na - 1); i++) {
        printf("%lld, ", A[i]);
    }
    printf("%lld]\n", A[na - 1]);
}

void fill_array(int64_t *A, size_t na) {
    int i;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srandom(tv.tv_usec ^ tv.tv_sec); // random seeding

    for (i = 0; i < na; i++) {
        A[i] = random() >> SHIFT;
    }
}


int64_t findmax(int64_t *array, size_t na);

void *find_max_threaded(void *args);


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Size of the random int array must be given.\n");
        exit(EXIT_FAILURE);
    }
    //
    size_t len = strtoul(argv[1], NULL, 10);
    if (!len) {
        fprintf(stderr, "Invalid len or string. \n");
        exit(EXIT_FAILURE);
    }

    // generate random array of size len
    int64_t *array = malloc(sizeof(int64_t) * len);
    fill_array(array, len);
    printf("The global max is %lld \n", findmax(array, len));
    free(array);

    return EXIT_SUCCESS;
}

void *find_max_threaded(void *args) {
    int64_t local_max;
    size_t i, n;
    thread_args *args_cast = (thread_args *) args;
    int64_t *A = args_cast->A;
    n = args_cast->len;
//    uint64_t tid;
//    pthread_threadid_np(NULL, &tid);

    if (n < 2) {
        return NULL;
    }
    local_max = *A;
    for (i = 0; i < n - 1; i++, A++) {
        if (*A > local_max) {
            local_max = *A;
        }
    }
//    printf("Thread id %llu found a max of %lld\n", tid, local_max);
//    pthread_mutex_lock(&lock);
    args_cast->output = (void *) local_max;
//    pthread_mutex_unlock(&lock);
    return NULL;
}

int64_t findmax(int64_t *array, size_t na) {
    pthread_t *t;
    size_t d, i, num_threads;
    d = ceil(log2((double) na));
    num_threads = na / d;
    pthread_t thread[d];
    thread_args args[d];
    int64_t global_max = *array;

//    memset(&args, 0, sizeof(args));

    for (i = 0; i < d; i++) {
        t = &thread[i];
        args[i].A = (void *) (array + (i * num_threads));
        args[i].len = (i == d - 1) ? (na - (i * num_threads)): num_threads;
        if (pthread_create(t, NULL, find_max_threaded, (void *) &args[i])) {
            perror("create:");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < d; i++) {
        if (pthread_join(thread[i], NULL)) {
            perror("join");
            exit(EXIT_FAILURE);
        }
        global_max = max(global_max, (int64_t) args[i].output);
    }
    return global_max;
}
