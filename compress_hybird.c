#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>
#include <nmmintrin.h>
#include <sys/time.h>
#include <string.h>
#include <vector>
#include <pthread.h>
#include <mutex>
#include <unistd.h> 

#include "compressor.h"

#define MAX_THREAD 128

typedef int64_t (*compress_func)(char *in, size_t insize, char *out, size_t outsize, size_t, size_t, char*);
typedef char* (*init_func)(size_t insize, size_t, size_t);
typedef void (*deinit_func)(char* workmem);

char const *g_compressor = "qpl";
char const *g_sourcefile = "compress_hybird";
char *g_sourcebuf = NULL;
size_t g_source_size = 0;
int g_circles = 1;
int g_pthread_num = 1;
int g_level = 1;
int g_param = 0;

struct thread_op_para
{
    /* compressor */
    char *inbuf;
    char *compbuf;
    size_t *comp_size;
    size_t comp_size_sum;
    char *workmem;

    compress_func compress;
    compress_func decompress;
    init_func init;
    deinit_func deinit;

    /* task */
    int thread_num;
    uint64_t total_time;
};

struct result
{
    double band_width;
    double execution_time;
};

uint64_t cal_time_slot(timeval start, timeval end) {
    uint64_t slot = 0;
    slot += end.tv_usec - start.tv_usec;
    slot += (end.tv_sec - start.tv_sec) * 1000000;
    return slot;
}


double time_us2s(uint64_t in_time) {
    double time_us = (double)(in_time % 1000000) / 1000000;
    double time_s  = (double)(in_time / 1000000);
    return time_s + time_us;
}


void cal_result(thread_op_para *op_para, result *out) {
    uint64_t time_min = op_para[0].total_time;
    uint64_t time_max = op_para[0].total_time;
    uint64_t time_sum = 0;
    
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        if (op_para[thrd].total_time < time_min) {
            time_min = op_para[thrd].total_time;
        }
        if (op_para[thrd].total_time > time_max) {
            time_max = op_para[thrd].total_time;
        }
        time_sum += op_para[thrd].total_time;
    }

    out->execution_time = time_us2s(time_sum/g_circles/g_pthread_num);
    out->band_width = (g_source_size>>20) / time_us2s(time_sum/g_pthread_num) * g_pthread_num * g_circles / 1024;
    printf("%10.2fs %6.2f %7.2f   %8.2f\n",
    time_us2s(time_min/g_circles), time_us2s(time_max/g_circles),
    out->execution_time, out->band_width);

    return;
}


void* memcpy_op(void *arg) {
    thread_op_para *op_para = (thread_op_para *) arg;
    struct timeval time_begin, time_end;

    op_para->total_time = 0;

    for (int i = 0; i < g_circles; i++) {

        char *sourcebuf  = g_sourcebuf;
        char *inbuf      = op_para->inbuf;
        memset(inbuf, 0, g_source_size);

        gettimeofday(&time_begin, NULL);
        memcpy(inbuf, sourcebuf, g_source_size);
        gettimeofday(&time_end, NULL);
        op_para->total_time += cal_time_slot(time_begin, time_end);

        if (memcmp(op_para->inbuf, g_sourcebuf, g_source_size) != 0) {
            printf("ERROR: thread%d: memcpy valid failed!\n", op_para->thread_num);
        }
    }

    return (void*)0;
}


void* compress_op(void *arg) {
    thread_op_para *op_para = (thread_op_para *) arg;
    struct timeval time_begin, time_end;

    op_para->total_time = 0;
    op_para->comp_size = (size_t *)malloc(sizeof(size_t));

    if (op_para->init)
    {
        op_para->workmem = op_para->init(0, g_param, g_param);
        if (!op_para->workmem)
        {
            printf("ERROR: thread%d: init\n", op_para->thread_num);
            return (void*)0;
        }
    }

    for (int i = 0; i < g_circles; i++) {
        char *inbuf = op_para->inbuf;
        char *compbuf = op_para->compbuf;
        memset(compbuf, 0, g_source_size);

        gettimeofday(&time_begin, NULL);
        op_para->comp_size[0] = op_para->compress(inbuf, g_source_size, compbuf, g_source_size, g_level, g_param, op_para->workmem);
        if (op_para->comp_size[0] == 0) {
            printf("ERROR: thread%d: compress\n", op_para->thread_num);
            return (void*)0;
        }
        gettimeofday(&time_end, NULL);
        printf("qijia: comp size = %d\n", op_para->comp_size[0]);
        // op_para->compbuf[8] = 0x02;
        // op_para->compbuf[9] = 0x03;
        for(int iii = 0; iii < 10; iii++) {
            printf("%02x", op_para->compbuf[iii] & 0xff);
        }
        printf("\n");

        op_para->total_time = cal_time_slot(time_begin, time_end);
    }

    return (void*)0;
}


void* decompress_op(void *arg) {
    thread_op_para *op_para = (thread_op_para *) arg;
    struct timeval time_begin, time_end;

    op_para->total_time = 0;

    for (int i = 0; i < g_circles; i++) {
        char *inbuf      = op_para->inbuf;
        char *compbuf   = op_para->compbuf;
        memset(inbuf, 0, g_source_size);

        gettimeofday(&time_begin, NULL);
        size_t clen = op_para->decompress(compbuf, op_para->comp_size[0], inbuf, g_source_size, g_level, g_param, op_para->workmem);
        if (clen == 0)
        {
            printf("ERROR: thread%d: decompress\n", op_para->thread_num);
            return (void*)0;
        }
        gettimeofday(&time_end, NULL);
        op_para->total_time = cal_time_slot(time_begin, time_end);

        if (memcmp(op_para->inbuf, g_sourcebuf, g_source_size) != 0) {
            printf("ERROR: thread%d: compression valid failed!\n", op_para->thread_num);
        }
    }

    if (op_para->deinit) {
        op_para->deinit(op_para->workmem);
    }

    return (void*)0;
}



static int parse_args(int argc, char **argv)
{
    int opt = -1;
    while (-1 != (opt = getopt(argc, argv, "e:f:b:t:p:l:a:h")))
    {
        switch (opt)
        {
            case 'e':
                g_compressor = optarg;
                break;
            case 'f':
                g_sourcefile = optarg;
                break;
            case 't':
                g_circles = atoi(optarg);
                break;
            case 'p':
                g_pthread_num = atoi(optarg);
                if (g_pthread_num > MAX_THREAD) {
                    printf("ERROR: max thread is %d", MAX_THREAD);
                    return 0;
                }
                break;
            case 'l':
                g_level = atoi(optarg);
                break;
            case 'a':
                g_param = atoi(optarg);
                break;
            case'h':
            default:
                printf("-e compressor\n");
                printf("-f source file name\n");
                printf("-b block size (kB)\n");
                printf("-t circles\n");
                printf("-p thread nmuber\n");
                printf("-l compress level\n");
                printf("-a compress additional param\n");
                printf("-h help\n");
                return 0;
        }
    }
    return opt;
}

void read_source()
{   
    FILE* in;

    if ( !(in=fopen(g_sourcefile, "rb")) )
    {
        printf("ERROR: open source file\n");
        return;
    }
    fseeko(in, 0L, SEEK_END);
    g_source_size = ftello(in);
    g_sourcebuf = (char *)calloc(1, g_source_size);
    rewind(in);
    if ( !g_sourcebuf ) {
        printf("ERROR: g_sourcebuf calloc.\n");
        return;
    }
    if (g_source_size != fread(g_sourcebuf, 1, g_source_size, in)) {
        printf("ERROR: source file reading.\n");
        free(g_sourcebuf);
    }
    fclose(in);

    return;
}

int main(int argc, char* argv[])
{
    if (parse_args(argc,argv) == 0)
    {
        return 0;
    }

    read_source();
    if (g_sourcefile == NULL || g_source_size == 0 )
    {
        return 0;
    }

    printf("Test configuration:\n");
    printf("source file: %s\n", g_sourcefile);
    printf("source size: %ld\n", g_source_size);
    printf("compressor: %s\n", g_compressor);
    printf("circles: %d\n", g_circles);
    printf("thread num: %d\n", g_pthread_num);
    printf("compress level: %d\n", g_level);
    printf("compress param: %d\n", g_param);

    result *memcpy_out = (result *)malloc(sizeof(result));
    result *compress_out = (result *)malloc(sizeof(result));
    result *decompress_out = (result *)malloc(sizeof(result));

    /* Prepare datas */
    pthread_t th_handle[MAX_THREAD];
    thread_op_para op_para[MAX_THREAD];
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        op_para[thrd].thread_num = thrd;
        op_para[thrd].inbuf = (char *)malloc(g_source_size);
        op_para[thrd].compbuf = (char *)malloc(g_source_size);

        if (!strcmp(g_compressor, "qpl")) {
            op_para[thrd].compress   = qpl_compress;
            op_para[thrd].decompress = qpl_decompress;
            op_para[thrd].init       = qpl_init;
            op_para[thrd].deinit     = qpl_deinit;
        }
        else if (!strcmp(g_compressor, "zlib")) {
            op_para[thrd].compress   = zlib_compress;
            op_para[thrd].decompress = zlib_decompress;
            op_para[thrd].init       = NULL;
            op_para[thrd].deinit     = NULL;
        }
        else if (!strcmp(g_compressor, "zlib-qpl")) {
            op_para[thrd].compress   = zlib_compress;
            op_para[thrd].decompress = qpl_decompress;
            op_para[thrd].init       = qpl_init;
            op_para[thrd].deinit     = qpl_deinit;
        }
        else if(!strcmp(g_compressor, "qpl-zlib")) {
            op_para[thrd].compress   = qpl_compress;
            op_para[thrd].decompress = zlib_decompress;
            op_para[thrd].init       = qpl_init;
            op_para[thrd].deinit     = qpl_deinit;
        }
        else {
            op_para[thrd].compress   = qpl_compress;
            op_para[thrd].decompress = qpl_decompress;
            op_para[thrd].init       = qpl_init;
            op_para[thrd].deinit     = qpl_deinit;
        }
    }

    /* Memory Copy */
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        int ret = pthread_create(&th_handle[thrd], NULL, memcpy_op, &op_para[thrd]);
        if (!ret) {
            // printf("Thread %d started.\n", thrd);
        } else {
            printf("Memcpy thread %d create error, code: %d\n", thrd, ret);
        }
    }
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        int ret = pthread_join(th_handle[thrd], NULL);
        if (!ret) {
            // printf("Thread %d stopped.\n", thrd);
        } else {
            printf("Memcpy thread %d join error, code: %d\n", thrd, ret);
        }
    }

    printf("\nOperation | Time min(s) max(s) avag(s) | TP(GB/s)\n");
    printf("   memcpy   ");
    cal_result(op_para, memcpy_out);

    /* Compress */
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        int ret = pthread_create(&th_handle[thrd], NULL, compress_op, &op_para[thrd]);
        if (!ret) {
            // printf("Thread %d started.\n", thrd);
        } else {
            printf("Compress thread %d create error, code: %d\n", thrd, ret);
        }
    }
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        int ret = pthread_join(th_handle[thrd], NULL);
        if (!ret) {
            // printf("Thread %d stopped.\n", thrd);
        } else {
            printf("Compress thread %d join error, code: %d\n", thrd, ret);
        }
    }

    printf(" compress   ");    cal_result(op_para, compress_out);


    /* Decompress */
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        int ret = pthread_create(&th_handle[thrd], NULL, decompress_op, &op_para[thrd]);
        if (!ret) {
            // printf("Thread %d started.\n", thrd);
        } else {
            printf("Decompress thread %d create error, code: %d\n", thrd, ret);
        }
    }
    for (int thrd = 0; thrd < g_pthread_num; thrd++) {
        int ret = pthread_join(th_handle[thrd], NULL);
        if (!ret) {
            // printf("Thread %d stopped.\n", thrd);
        } else {
            printf("Decompress thread %d join error, code: %d\n", thrd, ret);
        }
    }

    printf("  decomp.   ");
    cal_result(op_para, decompress_out);

    printf("\nOut: %d %.2f %.2f %.2f %.2f\n",
    g_pthread_num,
    compress_out->execution_time, compress_out->band_width,
    decompress_out->execution_time, decompress_out->band_width);

    return 0;
}
