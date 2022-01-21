/* cc zrom_cli.c lz4.c -Wall -O3 -std=c99 */
// --- OR ---
/* cc zrom_cli.c lz4.c lz4hc.c -Wall -O3 -std=c99 -DUSE_LZHC -DLZ4HC_HEAPMODE=0 */


// source code below is bad as the idea for zrom changed many times
// and code was *thrown* in each time.
#include "zrom.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <lz4.h>

#ifdef USE_LZHC
    #include <lz4hc.h>
#endif

enum { SIXTEEN_KIB = 16384 };
// adjust this to how much ram you have free
enum { MAX_SPACE_LEFT = 819200 - (SIXTEEN_KIB * 3) };


typedef struct ZromHeader Header_t;
typedef struct ZromBankEntry BankEntry_t;


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        exit(-1);
    }

    static char dst[SIXTEEN_KIB];

    FILE* f = fopen(argv[1], "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    const int size = ftell(f);
    rewind(f);

    char* src = (char*)malloc(size);

    fread(src, size, 1, f);
    fclose(f);
    f = NULL;

    const int banks = (size / SIXTEEN_KIB) - 1;
    assert(banks >= 1);

    // will never happen, max size is 4MiB which is the largest rom
    if (banks >= 0x100)
    {
        printf("maxed banks exeeded!\n");
        exit(-1);
    }

    const Header_t header = {
        .magic = ZROM_MAGIC,
        .banks = banks,
        .reserved = 0
    };

    BankEntry_t* entries = (BankEntry_t*)malloc(sizeof(BankEntry_t) * banks);

    f = fopen("out.gbz", "wb");
    assert(f);

    fwrite(src, SIXTEEN_KIB, 1, f);
    fwrite(&header, sizeof(Header_t), 1, f);
    fwrite(entries, sizeof(BankEntry_t) * banks, 1, f);

    int offset = sizeof(Header_t) + (sizeof(BankEntry_t) * banks) + SIXTEEN_KIB;
    int total_lz4_size = 0;

    for (int i = 0; i < banks; ++i)
    {
        int comp_size = 0;

        #ifdef USE_LZHC
            comp_size = LZ4_compress_HC(src + ((i + 1) * SIXTEEN_KIB), dst, SIXTEEN_KIB, SIXTEEN_KIB, LZ4HC_CLEVEL_MAX);
        #else
            comp_size = LZ4_compress_default(src + ((i + 1) * SIXTEEN_KIB), dst, SIXTEEN_KIB, SIXTEEN_KIB);
        #endif

        // failed to compress, store it raw
        if (comp_size <= 0)
        {
            comp_size = SIXTEEN_KIB;
            entries[i].flags = ZromEntryFlag_UNCOMPRESSED;
            memcpy(dst, src + ((i + 1) * SIXTEEN_KIB), SIXTEEN_KIB);
        }
        else
        {
            entries[i].flags = ZromEntryFlag_COMPRESSED;
        }

        fwrite(dst, comp_size, 1, f);

        entries[i].size = (uint32_t)comp_size;
        entries[i].offset = (uint32_t)offset;
        printf("\tBank %03d: size: %u offset: %u\n", i + 1, entries[i].size, entries[i].offset);

        offset += comp_size;
        total_lz4_size += comp_size;
    }

    fseek(f, SIXTEEN_KIB + sizeof(Header_t), SEEK_SET);
    fwrite(entries, (sizeof(BankEntry_t) * banks), 1, f);

    fclose(f);
    free(entries);
    free(src);

    printf("%s\tsize: %d\tbanks: %d\n\n", argv[1], size, banks);
    printf("lz4_size: %d\tdiff: %d\tleft: %d\t%s\n", total_lz4_size, total_lz4_size - size, MAX_SPACE_LEFT - total_lz4_size, total_lz4_size > MAX_SPACE_LEFT ? "NO-FIT" : "FITS");

    return 0;
}
