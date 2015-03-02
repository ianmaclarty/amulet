#include <stdio.h>

typedef struct clamped_view_def {
    const char *tname;
    const char *ctype;
    const char *minval;
    const char *maxval;
} clamped_view_def;

static const clamped_view_def clamped_views[] = {
    {"ubyte",   "uint8_t",    "0",          "UINT8_MAX"},
    {"byte",    "int8_t",     "INT8_MIN",   "INT8_MAX"},
    {"ushort",  "uint16_t",   "0",          "UINT16_MAX"},
    {"short",   "int16_t",    "INT16_MIN",  "INT16_MAX"},
};

int main() {
    int i;
    for (i = 0; i < sizeof(clamped_views) / sizeof(clamped_view_def); i++) {
        printf("#define TNAME %s\n", clamped_views[i].tname);
        printf("#define CTYPE %s\n", clamped_views[i].ctype);
        printf("#define MINVAL %s\n", clamped_views[i].minval);
        printf("#define MAXVAL %s\n", clamped_views[i].maxval);
        printf("#include \"am_clamped_view_template.inc\"\n\n");
    }
    for (i = 0; i < sizeof(clamped_views) / sizeof(clamped_view_def); i++) {
        printf("#define TNAME %s_norm\n", clamped_views[i].tname);
        printf("#define CTYPE %s\n", clamped_views[i].ctype);
        printf("#define MINVAL %s\n", clamped_views[i].minval);
        printf("#define MAXVAL %s\n", clamped_views[i].maxval);
        printf("#include \"am_normalized_view_template.inc\"\n\n");
    }
    return 0;
}
