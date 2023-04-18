# qpl library is required before build
# QPL v1.1.0
QPL_PATH=/root/qpl_install

QPL_LIB="-I$QPL_PATH/include -L$QPL_PATH/lib64"
QPL_FLAG="-lqpl -laccel-config -ldl"


# ZLIB v1.2.13
ZLIB_PATH=/root/qijia/zlib-1.2.13
ZLIB_LIB="-I$ZLIB_PATH -L$ZLIB_PATH"
ZLIB_FLAG="-lz"

FILES=$FILES" zlib/zlib_compress.cpp"
FILES=$FILES" qpl/qpl_compress.cpp"
# MIX_FILES=$FILES" compress_mix.c"
# SEPARATE_FILES=$FILES" compress_separate.c"
# HUGE_FILES=$FILES" compress_huge.c"
HYBIRD_FILES=$FILES" compress_hybird.c"

BUILD_LIB="$QPL_LIB $ZLIB_LIB"
BUILD_FLAG="$QPL_FLAG $ZLIB_FLAG"
# g++ -O3 -std=gnu++2a -march=skylake-avx512 -g $BUILD_LIB $MIX_FILES $BUILD_FLAG -lpthread -o compress_mix
# g++ -O3 -std=gnu++2a -march=skylake-avx512 -g $BUILD_LIB $SEPARATE_FILES $BUILD_FLAG -lpthread -o compress_separate
# g++ -O3 -std=gnu++2a -march=skylake-avx512 -g $BUILD_LIB $HUGE_FILES $BUILD_FLAG -lpthread -o compress_huge
g++ -O3 -std=gnu++2a -march=skylake-avx512 -g $BUILD_LIB $HYBIRD_FILES $BUILD_FLAG -lpthread -o compress_hybird
