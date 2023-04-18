#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>

#include "zlib.h"
#include "zutil.h"
#include "deflate.h"

int64_t zlib_compress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem) {
   
   /*
    param in zlib is the bits of slide window.
    WBITS = 15 -> 32K
    WBITS = 12 -> 4k
   */

    z_stream stream;
    int err;
    const uInt max = (uInt)-1;
    uLong sourceLen = insize;
    uLong left = outsize;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    if (param) {
        err = deflateInit2(&stream, level, Z_DEFLATED, param + 16, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    }
    else {
        err = deflateInit(&stream, level);
    }
    if (err != Z_OK) {
        printf("Error occurred while deflateInit2. Code:%d\n", err);
        return 0;
    }

    stream.next_out = (Bytef *)outbuf;
    stream.avail_out = 0;
    stream.next_in = (z_const Bytef *)inbuf;
    stream.avail_in = 0;

    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = sourceLen > (uLong)max ? max : (uInt)sourceLen;
            sourceLen -= stream.avail_in;
        }
        err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
    } while (err == Z_OK);
    
    if (err != Z_STREAM_END ) 
    {
        printf("Error occurred while deflate. Code:%d\n", err);
        return 0;
    }

    deflateEnd(&stream);
    
    return (int64_t)stream.total_out;
}

int64_t zlib_decompress(char *inbuf, size_t insize, char *outbuf, size_t outsize, size_t level, size_t param, char* workmem) {
    
    z_stream stream;
    int err;
    const uInt max = (uInt)-1;
    uLong sourceLen = insize;
    uLong destLen = outsize;
    uLong len, left;
    Bytef *dest = (Bytef *)outbuf;
    Byte buf[1];    /* for detection of incomplete stream when destLen == 0 */

    len = sourceLen;
    if (destLen) {
        left = destLen;
        destLen = 0;
    }
    else {
        left = 1;
        dest = buf;
    }

    stream.next_in = (z_const Bytef *)inbuf;
    stream.avail_in = 0;
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    // err = inflateInit(&stream);
    err = inflateInit2(&stream, param + 16);
    if (err != Z_OK) return err;

    stream.next_out = dest;
    stream.avail_out = 0;

    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = len > (uLong)max ? max : (uInt)len;
            len -= stream.avail_in;
        }
        err = inflate(&stream, Z_NO_FLUSH);
    } while (err == Z_OK);
    sourceLen -= len + stream.avail_in;

    if (err != Z_STREAM_END ) 
    {
        printf("Error occurred while inflate. Code:%d\n", err);
        return 0;
    }

    inflateEnd(&stream);

    return (int64_t)stream.total_out;
}





