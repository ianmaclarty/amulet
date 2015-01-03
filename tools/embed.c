#include <stdio.h>
#include <stdlib.h>

#define CHRS_PER_LINE 20

int main(int argc, char *argv[]) {
    char *filename;
    FILE *f;
    int chr;
    int count;
    int i;
    if (argc < 2) {
        fprintf(stderr, "Error: expecting at least one argument.\n");
        return EXIT_FAILURE;
    }
    printf("#include \"amulet.h\"\n\n");
    for (i = 1; i < argc; i++) {
        filename = argv[i];
        f = fopen(filename, "rb");
        count = 0;
        printf("static const uint8_t data%d[] = {\n    ", i);
        while (1) {
            if (count == CHRS_PER_LINE) {
                printf("\n    ");
                count = 0;
            }
            chr = getc(f);
            if (chr == EOF) {
                printf("};\n\n");
                break;
            }
            printf("0x%02X, ", chr);
            count++;
        }
        fclose(f);
    }
    printf("\n");
    printf("am_embedded_file_record am_embedded_files[] = {");
    for (i = 1; i < argc; i++) {
        filename = argv[i];
        printf("\n    {\"%s\", data%d, sizeof(data%d)},", filename, i, i);
    }
    printf("\n    {NULL, NULL, 0}\n};\n");
    return EXIT_SUCCESS;
}
