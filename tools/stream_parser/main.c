#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include "register_to_string.h"

int usage();
bool parse_and_print(void *, size_t);

int
main(int argc, char **argv)
{
    FILE* f = NULL;
    long fsize = 0;
    void *data;

    if (argc != 2)
        return usage();

    f = fopen(argv[1], "rb");
    if (f == NULL)
        goto error;

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(fsize);
    if (data == NULL)
        goto error;

    fread(data, fsize, 1, f);
    fclose(f);
    f = NULL;

    parse_and_print(data, fsize);

    return 0;
error:
    if (f != NULL)
        fclose(f);
    if (data != NULL)
        free(data);
    printf("Error: %s\n", strerror(errno));
    return 2;
}

int
usage()
{

    printf("Usage: bun_parse_stream <file>\n");
    return 1;
}

bool parse_and_print(void *data, size_t len)
{

    struct bun_frame frame;
    bun_reader_t r;

    if (bun_reader_init(&r, data, len) == false)
        return false;

    while (bun_frame_read(&r, &frame) != false) {
        printf("Frame: %s\n", frame.symbol);
        printf("  PC: %p\n", (void *)frame.addr);
        printf("  Registers: %d\n", frame.register_count);
        for (size_t i = 0; i < frame.register_count; i++) {
            uint16_t reg;
            uint64_t val;
            const char *str;
            bun_frame_register_get(&frame, i, &reg, &val);
            str = bun_register_to_string(reg);
            printf("    Register %s(%04X): %lX\n", str, reg, val);
        }
    }
}
