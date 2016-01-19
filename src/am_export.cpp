#include "amulet.h"

#ifdef AM_BUILD_TOOLS

#include "SimpleGlob.h"

#define RECURSE_LIMIT 20
#define AM_TMP_DIR ".amulet_tmp"

// See https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT section 4.4.2.2:
#define ZIP_PLATFORM_NTFS 10
#define ZIP_PLATFORM_OSX 19
#define ZIP_PLATFORM_UNIX 3
#define ZIP_PLATFORM_DOS 0

struct export_config {
    const char *pakfile;
    const char *apptitle;
    const char *appshortname;
    const char *appid;
    const char *appversion;
    const char *luavm;
    const char *grade;
    const char *basepath;
};

static bool add_matching_files_to_zip(const char *zipfile, const char *rootdir, const char *dir, const char *pat, bool compress, uint8_t platform) {
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
    char *pattern = am_format("%s%c%s", dir, AM_PATH_SEP, pat);
    glob.Add(pattern);
    free(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        size_t len;
        void *buf = am_read_file(file, &len);
        if (buf == NULL) return false;
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, file + strlen(rootdir) + 1,
                buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, 0100644, platform)) {
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
    char *pattern = am_format("%s%c%s", dir, AM_PATH_SEP, pat);
    glob.Add(pattern);
    free(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        size_t len;
        void *buf = am_read_file(file, &len);
        if (buf == NULL) return false;
        char *name;
        if (newname == NULL) {
            name = am_format("%s/%s", newdir, file + strlen(dir) + 1);
        } else if (newext == NULL) {
            name = am_format("%s/%s", newdir, newname);
        } else {
            name = am_format("%s/%s%s", newdir, newname, newext);
        }
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, name, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, executable ? 0100755 : 0100644, platform)) {
            free(buf);
            free(name);
            fprintf(stderr, "Error: failed to add %s to archive\n", file);
            return false;
        }
        free(name);
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
    char *pattern = am_format("%s%c*", dir, AM_PATH_SEP);
    glob.Add(pattern);
    free(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *subdir = glob.File(n);
        if (!build_data_pak_2(level + 1, rootdir, subdir, pakfile)) {
            return false;
        }
    }
    return true;
}

static bool build_data_pak(export_config *conf) {
    if (am_file_exists(conf->pakfile)) {
        am_delete_file(conf->pakfile);
    }
    return build_data_pak_2(0, am_opt_data_dir, am_opt_data_dir, conf->pakfile);
}

static char *get_bin_path(export_config *conf, const char *platform) {
    const char *builds_path = conf->basepath;
    if (strcmp(conf->basepath, "/usr/local/bin/") == 0) {
        builds_path = "/usr/local/share/amulet/";
    } else if (strcmp(conf->basepath, "/usr/bin/") == 0) {
        builds_path = "/usr/share/amulet/";
    }
    char *bin_path = am_format("%sbuilds/%s/%s/%s/bin", builds_path, platform, conf->luavm, conf->grade);
    if (!am_file_exists(bin_path)) {
        fprintf(stderr, "Error: export configuration %s/%s/%s not available in your installation.\n", platform, conf->luavm, conf->grade);
        fprintf(stderr, "(the path %s does not exist)\n", bin_path);
        free(bin_path);
        return NULL;
    }
    return bin_path;
}

static char *get_export_zip_name(export_config *conf, const char *platname) {
    return am_format("%s-%s-%s.zip", conf->appshortname, conf->appversion, platname);
}

static bool build_windows_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "windows");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "msvc32");
    if (binpath == NULL) return false;
    const char *name = conf->appshortname;
    bool ok =
        add_files_to_zip_renamed(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_DOS) &&
        add_files_to_zip_renamed(zipname, binpath, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_DOS) &&
        add_files_to_zip_renamed(zipname, binpath, "amulet.exe", name, name, ".exe", true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_zip_renamed(zipname, binpath, "*.dll", name, NULL, NULL, true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, name, "data.pak", NULL, false, false, ZIP_PLATFORM_DOS) &&
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
        conf->appshortname,
        conf->apptitle,
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
    const char *name = conf->appshortname;
    bool ok =
        add_files_to_zip_renamed(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "amulet", name, name, ".app/Contents/MacOS/amulet", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, name, name, ".app/Contents/Resources/data.pak", false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, AM_TMP_DIR, "Info.plist", name, name, ".app/Contents/Info.plist", true, false, ZIP_PLATFORM_UNIX) &&
        true;
    am_delete_file(AM_TMP_DIR "/Info.plist");
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
    const char *name = conf->appshortname;
    bool ok =
        add_files_to_zip_renamed(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath64, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath64, "amulet", name, name, ".x86_64", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath32, "amulet", name, name, ".i686", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, name, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
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
    const char *name = conf->appshortname;
    bool ok =
        add_files_to_zip_renamed(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "amulet.js", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, binpath, "player.html", name, "index.html", NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_zip_renamed(zipname, ".", conf->pakfile, name, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        true;
    printf("%s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool file_exists(const char *filename) {
    char *path = am_format("%s%c%s", am_opt_data_dir, AM_PATH_SEP, filename);
    bool exists = am_file_exists(path);
    free(path);
    return exists;
}

bool am_build_exports() {
    if (!file_exists("main.lua")) {
        fprintf(stderr, "Error: could not find main.lua in directory %s\n", am_opt_data_dir);
        return false;
    }
    am_make_dir(AM_TMP_DIR);
    if (!am_load_config()) return false;
    export_config conf;
    conf.basepath = (const char*)am_get_base_path();
    conf.apptitle = am_conf_app_title;
    conf.appshortname = am_conf_app_shortname;
    conf.appid = am_conf_app_id;
    conf.appversion = am_conf_app_version;
    conf.luavm = "lua51";
    conf.grade = "release";
    conf.pakfile = ".amulet_tmp/data.pak";
    if (!build_data_pak(&conf)) return false;
    bool ok =
        build_windows_export(&conf) &&
        build_mac_export(&conf) &&
        build_linux_export(&conf) &&
        build_html_export(&conf) &&
        true;
    am_delete_file(conf.pakfile);
    am_delete_empty_dir(AM_TMP_DIR);
    free((void*)conf.basepath);
    return ok;
}

#endif
