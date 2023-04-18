#ifndef BENCH_REMOVE_QPL
int64_t qpl_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem);
int64_t qpl_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem);
int64_t qpl_compress_huge(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem);
int64_t qpl_decompress_huge(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem);

char* qpl_init(size_t insize, size_t level, size_t path);
void qpl_deinit(char* workmem);
#endif

#ifndef BENCH_REMOVE_ZLIB
int64_t zlib_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem);
int64_t zlib_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem);
char* zlib_init(size_t insize, size_t level, size_t path);
void zlib_deinit(char* workmem);
#endif
