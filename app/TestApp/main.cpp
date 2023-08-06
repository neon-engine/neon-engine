#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <zlib.h>

int main() {
    char buffer_in [256] = {"Hello from main"};
    char buffer_out [256] = {0};

    z_stream zStream;
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
    zStream.avail_in = (uInt) strlen(buffer_in);
    zStream.next_in = (Bytef *) buffer_in;
    zStream.avail_out = (uInt) sizeof(buffer_out);
    zStream.next_out = (Bytef *) buffer_out;

    deflateInit(&zStream, Z_BEST_COMPRESSION);
    deflate(&zStream, Z_FINISH);
    deflateEnd(&zStream);

    printf("Uncompressed size is: %lu\n", strlen(buffer_in));
    printf("Compressed size is: %lu\n", strlen(buffer_out));

    printf("ZLIB VERSION: %s\n", zlibVersion());

    return EXIT_SUCCESS;
}
