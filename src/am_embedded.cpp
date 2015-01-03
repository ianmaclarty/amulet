#include "amulet.h"

am_embedded_file_record *am_get_embedded_file(const char *filename) {
    am_embedded_file_record *rec = &am_embedded_files[0];
    while (rec->filename != NULL) {
        if (strcmp(filename, rec->filename) == 0) {
            return rec;
        }
        rec++;
    }
    return NULL;
}
