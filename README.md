# IAA_Hybird_Compress
## Prepare
1. zlib
Download the source code form https://www.zlib.net/ and build.
2. QPL
3. Modify Zlib and QPL path in build.sh
```
QPL_PATH=/root/qpl_install
ZLIB_PATH=/root/qijia/zlib-1.2.13
```

## Build
Run build.sh

## Run
```
./compress_hybird -ezlib-qpl -a12 -f build.sh
```
Use -e to choose compress/decompress method
- zlib-qpl: zlib to compress, qpl to decompress
- qpl-zlib: qpl to compress, zlib to decompress
- zlib: zlib to compress and decompress
- qpl: qpl to compress and decompress

Use -a to set the zlib window_bits
- 12: 4k
- 15: 32k

Use -f to choose source file
