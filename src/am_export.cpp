#include "amulet.h"

#ifdef AM_EXPORTS_SUPPORTED

#include "SimpleGlob.h"

#define MAX_PATH_SZ 2048
#define RECURSE_LIMIT 20
#define AM_TMP_DIR ".amulet_tmp"

// See https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT section 4.4.2.2:
#define ZIP_PLATFORM_NTFS 10
#define ZIP_PLATFORM_OSX 19
#define ZIP_PLATFORM_UNIX 3
#define ZIP_PLATFORM_DOS 0

struct export_config {
    char *pakfile;
    char *appname;
    char *appid;
    char *appversion;
    char *luavm;
    char *grade;
    char *basepath;
};

static void *read_file(const char *filename, size_t *len) {
    *len = 0;
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to open file %s\n", filename);
        return NULL;
    }
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

static bool add_matching_files_to_zip(const char *zipfile, const char *rootdir, const char *dir, const char *pat, bool compress, uint8_t platform) {
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
        if (buf == NULL) return false;
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, file + os, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, 0100644, platform)) {
            free(buf);
            fprintf(stderr, "Error: failed to add %s to archive\n", file);
            return false;
        }
        free(buf);
    }
    return true;
}

static bool add_files_to_zip_renamed(const char *zipfile, const char *dir, const char *pat, const char *newdir, const char *newname, const char *newext, bool compress, bool executable, uint8_t platform) {
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
        if (buf == NULL) return false;
        char name[MAX_PATH_SZ];
        memset(name, 0, MAX_PATH_SZ);
        if (newname == NULL) {
            snprintf(name, MAX_PATH_SZ, "%s/%s", newdir, file + os);
        } else if (newext == NULL) {
            snprintf(name, MAX_PATH_SZ, "%s/%s", newdir, newname);
        } else {
            snprintf(name, MAX_PATH_SZ, "%s/%s%s", newdir, newname, newext);
        }
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, name, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, executable ? 0100755 : 0100644, platform)) {
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
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.lua", true, ZIP_PLATFORM_DOS) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.png", false, ZIP_PLATFORM_DOS) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.jpg", false, ZIP_PLATFORM_DOS) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.ogg", false, ZIP_PLATFORM_DOS) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.txt", true, ZIP_PLATFORM_DOS) &&
        add_matching_files_to_zip(zipfile, rootdir, dir, "*.obj", true, ZIP_PLATFORM_DOS) &&
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
        add_files_to_zip_renamed(zipname, binpath, "amulet.exe", conf->appname, conf->appname, ".exe", true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_zip_renamed(zipname, binpath, "*.dll", conf->appname, NULL, NULL, true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, "data.pak", NULL, false, false, ZIP_PLATFORM_DOS) &&
        true;
    printf("%s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool create_mac_info_plist(const char *filename, export_config *conf) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s", filename);
        return false;
    }
    fprintf(f,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"  <key>CFBundleName</key>\n"
"  <string>%s</string>\n"
"\n"
"  <key>CFBundleDisplayName</key>\n"
"  <string>%s</string>\n"
"\n"
"  <key>CFBundleIdentifier</key>\n"
"  <string>%s</string>\n"
"\n"
"  <key>CFBundleVersion</key>\n"
"  <string>%s</string>\n"
"\n"
"  <key>CFBundlePackageType</key>\n"
"  <string>APPL</string>\n"
"\n"
"  <key>CFBundleExecutable</key>\n"
"  <string>amulet</string>\n"
"\n"
"  <key>LSMinimumSystemVersion</key>\n"
"  <string>10.6.8</string>\n"
"</dict>\n"
"</plist>\n",
        conf->appname,
        conf->appname,
        conf->appid,
        conf->appversion);
    fclose(f);
    return true;
}

static bool build_mac_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "mac");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "osx");
    if (binpath == NULL) return false;
    if (!create_mac_info_plist(AM_TMP_DIR "/Info.plist", conf)) return false;
    bool ok =
        add_files_to_zip_renamed(zipname, binpath, "amulet", conf->appname, conf->appname, ".app/Contents/MacOS/amulet", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, conf->appname, ".app/Contents/Resources/data.pak", false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, AM_TMP_DIR, "Info.plist", conf->appname, conf->appname, ".app/Contents/Info.plist", true, false, ZIP_PLATFORM_UNIX) &&
        true;
    am_delete_file(AM_TMP_DIR "/Info.plist");
    printf("%s\n", zipname);
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
        add_files_to_zip_renamed(zipname, binpath64, "amulet", conf->appname, conf->appname, ".x86_64", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath32, "amulet", conf->appname, conf->appname, ".i686", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
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
        add_files_to_zip_renamed(zipname, binpath, "amulet.js", conf->appname, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "player.html", conf->appname, "index.html", NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "jquery-2.1.3.min.js", conf->appname, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, conf->appname, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        true;
    printf("%s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool main_exists(const char *dir) {
    char main_path[MAX_PATH_SZ];
    memset(main_path, 0, MAX_PATH_SZ);
    char last = dir[strlen(dir)-1];
    if (last == '/' || last == '\\') {
        snprintf(main_path, MAX_PATH_SZ, "%s%s", dir, "main.lua");
    } else {
        snprintf(main_path, MAX_PATH_SZ, "%s/%s", dir, "main.lua");
    }
    return am_file_exists(main_path);
}

bool am_build_exports(const char *dir) {
    if (!main_exists(dir)) {
        fprintf(stderr, "Error: could not find main.lua in directory %s\n", dir);
        return false;
    }
    am_make_dir(AM_TMP_DIR);
    export_config conf;
    conf.basepath = am_get_base_path();
    conf.appname = (char*)"Untitled";
    conf.appid = (char*)"xyz.amulet.untitled";
    conf.appversion = (char*)"1.0";
    conf.luavm = (char*)"lua51";
    conf.grade = (char*)"release";
    conf.pakfile = (char*)".amulet_tmp/data.pak";
    if (!build_data_pak(&conf, dir)) return false;
    bool ok =
        build_windows_export(&conf) &&
        build_mac_export(&conf) &&
        build_linux_export(&conf) &&
        build_html_export(&conf) &&
        true;
    am_delete_file(conf.pakfile);
    am_delete_empty_dir(AM_TMP_DIR);
    free(conf.basepath);
    return ok;
}

#endif
