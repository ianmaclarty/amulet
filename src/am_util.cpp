#include "amulet.h"

#include <sys/stat.h>

#define MAX_MSG_LEN 51200
#define MAX_CMD_LEN 51200

char *am_format(const char *fmt, ...) {
    char msg[MAX_MSG_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(msg, MAX_MSG_LEN, fmt, argp);
    va_end(argp);
    int n = strlen(msg);
    char *str = (char*)malloc(n+1);
    strncpy(str, msg, n+1);
    return str;
}

bool am_file_exists(const char *filename) {
    struct stat info;
    return stat(filename, &info) == 0;
}

void am_replchr(char *str, char c0, char c) {
    while (*str) {
        if (*str == c0) *str = c;
        str++;
    }
}

char *am_replace_strings(char *source, char** replacements) {
    char *result = am_format("%s", source);
    char **pair = replacements;
    while (*pair != NULL) {
        char *key = *pair;
        char *val = *(pair + 1);
        int keylen = strlen(key);
        int vallen = strlen(val);
        char *pos = strstr(result, key);
        while (pos != NULL) {
            *pos = '\0';
            char *rest = pos + keylen;
            char *new_result = am_format("%s%s%s", result, val, rest);
            pos = strstr(new_result + (pos - result) + vallen, key);
            free(result);
            result = new_result;
        }
        pair += 2;
    }
    return result;
}

void am_delete_file(const char *file) {
#ifdef AM_WINDOWS
    _unlink(file);
#else
    unlink(file);
#endif
}

void am_make_dir(const char* dir) {
#if defined(AM_WINDOWS)
    _mkdir(dir);
#else
    mkdir(dir, S_IRWXU | S_IRWXG);
#endif
}

void am_delete_empty_dir(const char* dir) {
#if defined(AM_WINDOWS)
    _rmdir(dir);
#else
    rmdir(dir);
#endif
}

#ifdef AM_WINDOWS
static wchar_t *to_wstr(const char *str) {
    int len16 = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    wchar_t *str16 = (wchar_t*)malloc(len16 * 2);
    MultiByteToWideChar(CP_UTF8, 0, str, -1, str16, len16);
    return str16;
}
#endif

extern "C" {
// handles utf8 filenames on windows
FILE *am_fopen(const char *path, const char *mode) {
#ifdef AM_WINDOWS
    wchar_t *path16 = to_wstr(path);
    wchar_t *mode16 = to_wstr(mode);
    FILE *f = _wfopen(path16, mode16);
    free(path16);
    free(mode16);
    return f;
#else
    return fopen(path, mode);
#endif
}
}

void *am_read_file(const char *filename, size_t *len) {
    if (len != NULL) *len = 0;
    FILE *f = am_fopen(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to open file %s\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size_t l = ftell(f);
    rewind(f);
    if (len != NULL) *len = l;
    unsigned char *buf = (unsigned char*)malloc(l + 1);
    size_t r = fread(buf, 1, l, f);
    fclose(f);
    buf[l] = 0; // null terminate so text file data can be cast to string
    if (r != l) {
        free(buf);
        fprintf(stderr, "Error: unable to read file %s\n", filename);
        return NULL;
    }
    return buf;
}

bool am_write_text_file(const char *filename, char *content) {
    FILE *f = am_fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s\n", filename);
        return false;
    }
    int len = strlen(content);
    if (len > 0) {
        if (fwrite(content, len, 1, f) != 1) {
            fprintf(stderr, "Error writing to file %s\n", filename);
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

bool am_write_bin_file(const char *filename, void *content, size_t len) {
    FILE *f = am_fopen(filename, "wb");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s\n", filename);
        return false;
    }
    if (len > 0) {
        if (fwrite(content, len, 1, f) != 1) {
            fprintf(stderr, "Error writing to file %s\n", filename);
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

#ifdef AM_HAVE_GLOB

#include "SimpleGlob.h"

void am_expand_args(int *argc_ptr, char ***argv_ptr) {
    char **argv = *argv_ptr;
    int argc = *argc_ptr;
    int expanded_argc = 0;
    for (int a = 0; a < argc; a++) {
        if (strchr(argv[a], '*') == NULL) {
            expanded_argc++;
        } else {
            CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
            glob.Add(argv[a]);
            expanded_argc += glob.FileCount();
        }
    }
    char **expanded_argv = (char**)malloc(sizeof(char*) * expanded_argc);
    int i = 0;
    for (int a = 0; a < argc; a++) {
        if (strchr(argv[a], '*') == NULL) {
            expanded_argv[i++] = am_format("%s", argv[a]);
        } else {
            CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
            glob.Add(argv[a]);
            for (int n = 0; n < glob.FileCount(); ++n) {
                char *file = glob.File(n);
                expanded_argv[i++] = am_format("%s", file);
            }
        }
    }
    *argc_ptr = expanded_argc;
    *argv_ptr = expanded_argv;
}

void am_free_expanded_args(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
}
#endif

bool am_execute_shell_cmd(const char *fmt, ...) {
#ifdef AM_IOS
    return false; // system not allowed on iOS
#else
    char cmd[MAX_CMD_LEN];
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(cmd, MAX_CMD_LEN, fmt, argp);
    va_end(argp);
    int rt = system(cmd);
    if (rt != 0) {
        fprintf(stderr, "Error running shell command: %s\n", cmd);
    }
    return rt == 0;
#endif
}
