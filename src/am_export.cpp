#include "amulet.h"

#ifdef AM_EXPORTS_SUPPORTED

#include "SimpleGlob.h"

#define MAX_PATH_SZ 2048

#define RECURSE_LIMIT 20

struct export_config {
    char *pakfile;
    char *appname;
    char *luavm;
    char *grade;
    char *basepath;
};

static void *read_file(const char *filename, size_t *len) {
    FILE *f = fopen(filename, "r");
    size_t l = 0;
    int c;
    do {
        c = fgetc(f);
        if (c == EOF) break;
        else l++;
    } while (1);
    *len = l;
    unsigned char *buf = (unsigned char*)malloc(l);
    fseek(f, 0, SEEK_SET);
    for (size_t i = 0; i < l; i++) {
        buf[i] = fgetc(f);
    }
    return buf;
}

static bool add_matching_files_to_zip(const char *zipfile, const char *rootdir, const char *dir, const char *pat, bool compress) {
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
    char pattern[MAX_PATH_SZ];
    memset(pattern, 0, MAX_PATH_SZ);
    char last = dir[strlen(dir)-1];
    if (last == '/' || last == '\\') {
        snprintf(pattern, MAX_PATH_SZ, "%s%s", dir, pat);
    } else {
        snprintf(pattern, MAX_PATH_SZ, "%s/%s", dir, pat);
    }
    glob.Add(pattern);
    int os;
    last = rootdir[strlen(rootdir)-1];
    if (last == '/' || last == '\\') {
        os = strlen(rootdir);
    } else {
        os = strlen(rootdir) + 1;
    }
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        size_t len;
        void *buf = read_file(file, &len);
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, file + os, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, 0644)) {
            free(buf);
            fprintf(stderr, "Error: failed to add %s to archive\n", file);
            return false;
        }
        free(buf);
    }
    return true;
}

static bool add_files_to_zip_renamed(const char *zipfile, const char *dir, const char *pat, const char *newdir, const char *newname, const char *newext, bool compress, bool executable) {
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
    char pattern[MAX_PATH_SZ];
    memset(pattern, 0, MAX_PATH_SZ);
    char last = dir[strlen(dir)-1];
    int os;
    if (last == '/' || last == '\\') {
        snprintf(pattern, MAX_PATH_SZ, "%s%s", dir, pat);
        os = strlen(dir);
    } else {
        snprintf(pattern, MAX_PATH_SZ, "%s/%s", dir, pat);
        os = strlen(dir) + 1;
    }
    glob.Add(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        size_t len;
        void *buf = read_file(file, &len);
        char name[MAX_PATH_SZ];
        memset(name, 0, MAX_PATH_SZ);
        if (newname == NULL) {
            snprintf(name, MAX_PATH_SZ, "%s/%s", newdir, file + os);
        } else if (newext == NULL) {
            snprintf(name, MAX_PATH_SZ, "%s/%s", newdir, newname);
        } else {
            snprintf(name, MAX_PATH_SZ, "%s/%s%s", newdir, newname, newext);
        }
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, name, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, executable ? 0755 : 0644)) {
            free(buf);
            fprintf(stderr, "Error: failed to add %s to archive\n", file);
            return false;
        }
        free(buf);
    }
    return true;
}

static bool add_data_files_to_zip(const char *zipfile, const char *rootdir, const char *dir) {
    return 
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.lua", true) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.png", false) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.jpg", false) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.ogg", false) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.txt", true) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.obj", true) &&
        true;
}

static bool build_data_pak_2(int level, const char *rootdir, const char *dir, const char *pakfile) {
    if (level >= RECURSE_LIMIT) {
        fprintf(stderr, "Error: maximum directory recursion depth reached (%d)\n", RECURSE_LIMIT);
        return false;
    }
    if (!add_data_files_to_zip(pakfile, rootdir, dir)) {
        return false;
    }
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYDIR | SG_GLOB_NODOT);
    char pattern[MAX_PATH_SZ];
    memset(pattern, 0, MAX_PATH_SZ);
    char last = dir[strlen(dir)-1];
    if (last == '/' || last == '\\') {
        snprintf(pattern, MAX_PATH_SZ, "%s*", dir);
    } else {
        snprintf(pattern, MAX_PATH_SZ, "%s/*", dir);
    }
    glob.Add(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *subdir = glob.File(n);
        if (!build_data_pak_2(level + 1, rootdir, subdir, pakfile)) {
            return false;
        }
    }
    return true;
}

static bool build_data_pak(export_config *conf, const char *dir) {
    if (am_file_exists(conf->pakfile)) {
        am_delete_file(conf->pakfile);
    }
    return build_data_pak_2(0, dir, dir, conf->pakfile);
}

static char *get_bin_path(export_config *conf, const char *platform) {
    char *bin_path = (char*)malloc(MAX_PATH_SZ);
    memset(bin_path, 0, MAX_PATH_SZ);
    snprintf(bin_path, MAX_PATH_SZ, "%sbuilds/%s/%s/%s/bin", conf->basepath, platform, conf->luavm, conf->grade);
    if (!am_file_exists(bin_path)) {
        fprintf(stderr, "Error: export configuration %s/%s/%s not available in your installation.\n", platform, conf->luavm, conf->grade);
        fprintf(stderr, "(the path %s does not exist)\n", bin_path);
        free(bin_path);
        return NULL;
    }
    return bin_path;
}

static char *get_export_zip_name(export_config *conf, const char *platname) {
    char *name = (char*)malloc(MAX_PATH_SZ);
    memset(name, 0, MAX_PATH_SZ);
    snprintf(name, MAX_PATH_SZ, "%s-%s.zip", conf->appname, platname);
    return name;
}

static bool build_windows_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "windows");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "msvc32");
    if (binpath == NULL) return false;
    bool ok =
        add_files_to_zip_renamed(zipname, binpath, "amulet.exe", conf->appname, conf->appname, ".exe", true, true) &&
        add_files_to_zip_renamed(zipname, binpath, "*.dll", conf->appname, NULL, NULL, true, true) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, NULL, NULL, false, false) &&
        true;
    printf("%s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool build_linux_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "linux");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath64 = get_bin_path(conf, "linux64");
    if (binpath64 == NULL) return false;
    char *binpath32 = get_bin_path(conf, "linux32");
    if (binpath32 == NULL) return false;
    bool ok =
        add_files_to_zip_renamed(zipname, binpath64, "amulet", conf->appname, conf->appname, ".x86_64", true, true) &&
        add_files_to_zip_renamed(zipname, binpath32, "amulet", conf->appname, conf->appname, ".i686", true, true) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, NULL, NULL, false, false) &&
        true;
    printf("%s\n", zipname);
    free(zipname);
    free(binpath32);
    free(binpath64);
    return ok;
}

static bool build_html_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "html");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "html");
    if (binpath == NULL) return false;
    bool ok =
        add_files_to_zip_renamed(zipname, binpath, "amulet.js", conf->appname, NULL, NULL, true, false) &&
        add_files_to_zip_renamed(zipname, binpath, "player.html", conf->appname, "index.html", NULL, true, false) &&
        add_files_to_zip_renamed(zipname, binpath, "jquery-2.1.3.min.js", conf->appname, NULL, NULL, true, false) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, NULL, NULL, false, false) &&
        true;
    printf("%s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

bool am_build_exports(const char *dir) {
    export_config conf;
    conf.basepath = am_get_base_path();
    conf.appname = (char*)"Untitled";
    conf.luavm = (char*)"lua51";
    conf.grade = (char*)"release";
    conf.pakfile = (char*)"data.pak";
    if (!build_data_pak(&conf, dir)) return false;
    bool ok =
        build_windows_export(&conf) &&
        build_linux_export(&conf) &&
        build_html_export(&conf) &&
        true;
    am_delete_file(conf.pakfile);
    free(conf.basepath);
    return ok;
}

#endif
