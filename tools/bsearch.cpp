#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHRS_PER_LINE 20

static uint8_t *read_file(const char *filename, size_t *len) {
    if (len != NULL) *len = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to open file %s\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t l = ftell(f);
    rewind(f);
    if (len != NULL) *len = l;
    uint8_t *buf = (uint8_t*)malloc(l);
    size_t r = fread(buf, 1, l, f);
    fclose(f);
    if (r != l) {
        free(buf);
        fprintf(stderr, "Error: unable to read file %s\n", filename);
        return NULL;
    }
    return buf;
}

static void usage() {
    fprintf(stderr, "usage: bsearch <needle file> <haystack file> <bytes> <offset>\n");
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        usage();
        return EXIT_FAILURE;
    }
    const char *needle_filename = argv[1];
    const char *haystack_filename = argv[2];
    size_t search_len = (size_t)atoi(argv[3]);
    size_t offset = (size_t)atoi(argv[4]);
    if (search_len == 0) {
        usage();
        return EXIT_FAILURE;
    }

    size_t needle_len;
    uint8_t *needle_data = read_file(needle_filename, &needle_len);
    size_t haystack_len;
    uint8_t *haystack_data = read_file(haystack_filename, &haystack_len);

    if (needle_len == 0 || needle_data == NULL || haystack_len == 0 || haystack_data == 0) {
        usage();
        return EXIT_FAILURE;
    }

    needle_data += offset;

    for (size_t p = 0; p < haystack_len - search_len; p++) {
        if (memcmp(&haystack_data[p], needle_data, search_len) == 0) {
            printf("found match at position %lu\n", p);
            return EXIT_SUCCESS;
        }
    }

    printf("no match found\n");
    return EXIT_SUCCESS;
}
