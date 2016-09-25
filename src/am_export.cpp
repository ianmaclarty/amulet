#include "amulet.h"

#ifdef AM_EXPORT

#include "SimpleGlob.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define RECURSE_LIMIT 20
#define AM_TMP_DIR ".amulet_tmp"

// See https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT section 4.4.2.2:
#define ZIP_PLATFORM_NTFS 10
#define ZIP_PLATFORM_OSX 19
#define ZIP_PLATFORM_UNIX 3
#define ZIP_PLATFORM_DOS 0

struct export_config {
    const char *pakfile;
    const char *title;
    const char *shortname;
    const char *appid;
    const char *version;
    const char *display_name;
    const char *dev_region;
    const char *supported_languages;
    am_display_orientation orientation;
    const char *icon;
    const char *launch_image;
    const char *grade;
    const char *basepath;
};

static bool create_mac_info_plist(const char *filename, export_config *conf);
static bool create_ios_info_plist(const char *binpath, const char *filename, export_config *conf);
static bool create_ios_pkginfo(const char *filename);
static bool create_ios_icon_files(const char *dir, export_config *conf);
static bool create_ios_launch_images(const char *dir, export_config *conf);
static bool create_ios_lproj_dirs(const char *zipname, const char *dir, export_config *conf);

static void replace_backslashes(char *str) {
    for (unsigned int i = 0; i < strlen(str); i++) {
        if (str[i] == '\\') str[i] = '/';
    }
}

static void sub_win_newlines(void **ptr, size_t *len) {
    char *str = (char*)(*ptr);
    int n = (int)(*len);
    int num_subs = 0;
    for (int i = 0; i < n; i++) {
        if (str[i] == '\n') num_subs++;
        if (str[i] == '\r') i++;
    }
    int new_n = n + num_subs;
    char *new_str = (char*)malloc(new_n);
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (str[i] == '\r') {
            new_str[j++] = '\r';
            i++;
            if (i < n) new_str[j++] = str[i];
        } else if (str[i] == '\n') {
            new_str[j++] = '\r';
            new_str[j++] = '\n';
        } else {
            new_str[j++] = str[i];
        }
    }
    *ptr = (void*)new_str;
    *len = (size_t)new_n;
}

static bool add_files_to_pak(const char *zipfile, const char *rootdir, const char *dir, const char *pat, bool compress, uint8_t platform) {
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
    char *pattern = am_format("%s%c%s", dir, AM_PATH_SEP, pat);
    glob.Add(pattern);
    free(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        size_t len;
        void *buf = am_read_file(file, &len);
        if (buf == NULL) return false;
        replace_backslashes(file);
        char *name = file + strlen(rootdir) + 1;
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, name,
                buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, 0100644, platform)) {
            free(buf);
            fprintf(stderr, "Error: failed to add %s to archive\n", file);
            return false;
        }
        free(buf);
        printf("+ %s\n", name);
    }
    return true;
}

static bool add_files_to_dist(const char *zipfile, const char *dir, const char *pat, const char *newdir, const char *newname, const char *newext, bool compress, bool executable, uint8_t platform) {
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
    char *pattern = am_format("%s%c%s", dir, AM_PATH_SEP, pat);
    glob.Add(pattern);
    free(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        size_t len;
        void *buf = am_read_file(file, &len);
        if (buf == NULL) return false;
        bool is_txt = strlen(file) > 4 && strcmp(file + strlen(file) - 4, ".txt") == 0;
        if (is_txt && platform == ZIP_PLATFORM_DOS) {
            void *tmp = buf;
            sub_win_newlines(&buf, &len);
            free(tmp);
        }
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
        add_files_to_pak(zipfile, rootdir, dir, "*.lua", true, ZIP_PLATFORM_DOS) &&
        add_files_to_pak(zipfile, rootdir, dir, "*.png", false, ZIP_PLATFORM_DOS) &&
        add_files_to_pak(zipfile, rootdir, dir, "*.jpg", false, ZIP_PLATFORM_DOS) &&
        add_files_to_pak(zipfile, rootdir, dir, "*.ogg", false, ZIP_PLATFORM_DOS) &&
        add_files_to_pak(zipfile, rootdir, dir, "*.obj", true, ZIP_PLATFORM_DOS) &&
        true;
}

static bool build_data_pak_2(int level, bool recursive, const char *rootdir, const char *dir, const char *pakfile) {
    if (level >= RECURSE_LIMIT) {
        fprintf(stderr, "Error: maximum directory recursion depth reached (%d)\n", RECURSE_LIMIT);
        return false;
    }
    if (!add_data_files_to_zip(pakfile, rootdir, dir)) {
        return false;
    }
    if (recursive) {
        CSimpleGlobTempl<char> glob(SG_GLOB_ONLYDIR | SG_GLOB_NODOT);
        char *pattern = am_format("%s%c*", dir, AM_PATH_SEP);
        glob.Add(pattern);
        free(pattern);
        for (int n = 0; n < glob.FileCount(); ++n) {
            char *subdir = glob.File(n);
            if (!build_data_pak_2(level + 1, recursive, rootdir, subdir, pakfile)) {
                return false;
            }
        }
    }
    return true;
}

static bool build_data_pak(export_config *conf) {
    if (am_file_exists(conf->pakfile)) {
        am_delete_file(conf->pakfile);
    }
    return build_data_pak_2(0, false, am_opt_data_dir, am_opt_data_dir, conf->pakfile);
}

static const char *platform_luavm(const char *platform) {
    if (strcmp(platform, "msvc32")) return "luajit";
    if (strcmp(platform, "linux32")) return "luajit";
    if (strcmp(platform, "linux64")) return "luajit";
    if (strcmp(platform, "osx")) return "luajit";
    return "lua51";

static char *get_bin_path(export_config *conf, const char *platform) {
    const char *builds_path = conf->basepath;
    if (strcmp(conf->basepath, "/usr/local/bin/") == 0) {
        builds_path = "/usr/local/share/amulet/";
    } else if (strcmp(conf->basepath, "/usr/bin/") == 0) {
        builds_path = "/usr/share/amulet/";
    }
    const char *luavm = platform_luavm(platform);
    char *bin_path = am_format("%sbuilds/%s/%s/%s/bin", builds_path, platform, luavm, conf->grade);
    if (!am_file_exists(bin_path)) {
        free(bin_path);
        return NULL;
    }
    return bin_path;
}

static char *get_export_zip_name(export_config *conf, const char *platname) {
    const char *ext = "zip";
    if (strcmp(platname, "ios") == 0) ext = "ipa";
    return am_format("%s-%s-%s.%s", conf->shortname, conf->version, platname, ext);
}

static bool build_windows_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "windows");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "msvc32");
    if (binpath == NULL) return true;
    const char *name = conf->shortname;
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, binpath, "amulet.exe", name, name, ".exe", true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, binpath, "*.dll", name, NULL, NULL, true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, ".", conf->pakfile, name, "data.pak", NULL, false, false, ZIP_PLATFORM_DOS) &&
        true;
    printf("Generated %s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool build_mac_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "mac");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "osx");
    if (binpath == NULL) return true;
    if (!create_mac_info_plist(AM_TMP_DIR AM_PATH_SEP_STR "Info.plist", conf)) return false;
    const char *name = conf->shortname;
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet", name, name, ".app/Contents/MacOS/amulet", true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, name, name, ".app/Contents/Resources/data.pak", false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Info.plist", name, name, ".app/Contents/Info.plist", true, false, ZIP_PLATFORM_UNIX) &&
        true;
    am_delete_file(AM_TMP_DIR AM_PATH_SEP_STR "Info.plist");
    printf("Generated %s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool build_ios_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "ios");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "ios");
    if (binpath == NULL) return true;
    if (!create_ios_info_plist(binpath, AM_TMP_DIR AM_PATH_SEP_STR "Info.plist", conf)) return false;
    if (!create_ios_pkginfo(AM_TMP_DIR AM_PATH_SEP_STR "PkgInfo")) return false;
    if (!create_ios_icon_files(AM_TMP_DIR, conf)) return false;
    if (!create_ios_launch_images(AM_TMP_DIR, conf)) return false;
    const char *name = conf->shortname;
    char *appdir = am_format("Payload/%s.app", name);
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", appdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", appdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet", appdir, name, NULL, true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, appdir, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Info.plist", appdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "PkgInfo", appdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Icon.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Icon@2x.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon40.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon57.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon72.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon76.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon80.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon114.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon120.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon144.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon152.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "icon180.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "iTunesArtwork", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default@2x.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-568h@2x.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-667h@2x.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-736h@2x.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-Landscape@2x~ipad.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-Landscape~ipad.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-Portrait@2x~ipad.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Default-Portrait~ipad.png", appdir, NULL, NULL, false, false, ZIP_PLATFORM_UNIX) &&
        create_ios_lproj_dirs(zipname, appdir, conf) &&
        true;
    free(appdir);
    am_delete_file(AM_TMP_DIR AM_PATH_SEP_STR "Info.plist");
    am_delete_file(AM_TMP_DIR AM_PATH_SEP_STR "PkgInfo");
    printf("Generated %s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool build_linux_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "linux");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath64 = get_bin_path(conf, "linux64");
    char *binpath32 = get_bin_path(conf, "linux32");
    if (binpath64 == NULL && binpath32 == NULL) return true;
    const char *name = conf->shortname;
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath64, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, name, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        true;
    if (ok && binpath32 != NULL) {
        ok = add_files_to_dist(zipname, binpath32, "amulet", name, name, ".i686", true, true, ZIP_PLATFORM_UNIX);
    }
    if (ok && binpath64 != NULL) {
        ok = add_files_to_dist(zipname, binpath64, "amulet", name, name, ".x86_64", true, true, ZIP_PLATFORM_UNIX)
            && add_files_to_dist(zipname, binpath64, "amulet.sh", name, name, "", true, true, ZIP_PLATFORM_UNIX);
    }
    printf("Generated %s\n", zipname);
    free(zipname);
    if (binpath32 != NULL) free(binpath32);
    if (binpath64 != NULL) free(binpath64);
    return ok;
}

static bool build_html_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "html");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "html");
    if (binpath == NULL) return true;
    const char *name = conf->shortname;
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet.js", name, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "player.html", name, "index.html", NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, name, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        true;
    printf("Generated %s\n", zipname);
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

bool am_build_exports(uint32_t flags) {
    if (!file_exists("main.lua")) {
        fprintf(stderr, "Error: could not find main.lua in directory %s\n", am_opt_data_dir);
        return false;
    }
    am_make_dir(AM_TMP_DIR);
    if (!am_load_config()) return false;
    export_config conf;
    conf.basepath = (const char*)am_get_base_path();
    conf.title = am_conf_app_title;
    conf.shortname = am_conf_app_shortname;
    conf.appid = am_conf_app_id;
    conf.version = am_conf_app_version;
    conf.display_name = am_conf_app_display_name;
    conf.dev_region = am_conf_app_dev_region;
    conf.supported_languages = am_conf_app_supported_languages;
    conf.orientation = am_conf_app_display_orientation;
    conf.icon = am_conf_app_icon;
    conf.launch_image = am_conf_app_launch_image;
    conf.grade = "release";
    conf.pakfile = AM_TMP_DIR AM_PATH_SEP_STR "data.pak";
    if (!build_data_pak(&conf)) return false;
    bool ok =
        ((!(flags & AM_EXPORT_FLAG_WINDOWS)) || build_windows_export(&conf)) &&
        ((!(flags & AM_EXPORT_FLAG_OSX))     || build_mac_export(&conf)) &&
        ((!(flags & AM_EXPORT_FLAG_IOS))     || build_ios_export(&conf)) &&
        ((!(flags & AM_EXPORT_FLAG_LINUX))   || build_linux_export(&conf)) &&
        ((!(flags & AM_EXPORT_FLAG_HTML))    || build_html_export(&conf)) &&
        true;
    am_delete_file(conf.pakfile);
    am_delete_empty_dir(AM_TMP_DIR);
    free((void*)conf.basepath);
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
        conf->shortname,
        conf->title,
        conf->appid,
        conf->version);
    fclose(f);
    return true;
}

static bool create_ios_info_plist(const char *binpath, const char *filename, export_config *conf) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s", filename);
        return false;
    }
    char *template_filename = am_format("%s%c%s", binpath, AM_PATH_SEP, "Info.plist");
    char *template_fmt = (char*)am_read_file(template_filename, NULL);
    free(template_filename);
    if (template_fmt == NULL) return false;
    const char *orientation_xml = "";
    switch (conf->orientation) {
        case AM_DISPLAY_ORIENTATION_ANY:
            orientation_xml = 
                "<key>UISupportedInterfaceOrientations</key>"
                "<array>"
                "<string>UIInterfaceOrientationPortrait</string>"
                //"<string>UIInterfaceOrientationPortraitUpsideDown</string>"
		"<string>UIInterfaceOrientationLandscapeLeft</string>"
		"<string>UIInterfaceOrientationLandscapeRight</string>"
                "</array>"
                "<key>UISupportedInterfaceOrientations~ipad</key>"
                "<array>"
                "<string>UIInterfaceOrientationPortrait</string>"
                //"<string>UIInterfaceOrientationPortraitUpsideDown</string>"
		"<string>UIInterfaceOrientationLandscapeLeft</string>"
		"<string>UIInterfaceOrientationLandscapeRight</string>"
                "</array>";
            break;
        case AM_DISPLAY_ORIENTATION_PORTRAIT:
            orientation_xml = 
                "<key>UISupportedInterfaceOrientations</key>"
                "<array>"
                "<string>UIInterfaceOrientationPortrait</string>"
                //"<string>UIInterfaceOrientationPortraitUpsideDown</string>"
                "</array>"
                "<key>UISupportedInterfaceOrientations~ipad</key>"
                "<array>"
                "<string>UIInterfaceOrientationPortrait</string>"
                //"<string>UIInterfaceOrientationPortraitUpsideDown</string>"
                "</array>";
            break;
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            orientation_xml = 
                "<key>UISupportedInterfaceOrientations</key>"
                "<array>"
		"<string>UIInterfaceOrientationLandscapeLeft</string>"
		"<string>UIInterfaceOrientationLandscapeRight</string>"
                "</array>"
                "<key>UISupportedInterfaceOrientations~ipad</key>"
                "<array>"
		"<string>UIInterfaceOrientationLandscapeLeft</string>"
		"<string>UIInterfaceOrientationLandscapeRight</string>"
                "</array>";
            break;
    }


    fprintf(f, template_fmt, 
        conf->title, // CFBundleName
        conf->display_name, // CFBundleDisplayName
        conf->dev_region, // CFBundleDevelopmentRegion
        conf->version, // CFBundleShortVersionString
        conf->version, // CFBundleVersion
        conf->shortname, // CFBundleExecutable
        conf->appid, // CFBundleIdentifier
        orientation_xml
    );
    fclose(f);

    return true;
}

static bool create_ios_pkginfo(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s", filename);
        return false;
    }
    fprintf(f, "APPL????");
    fclose(f);
    return true;
}

static bool create_ios_lproj_dirs(const char *zipname, const char *dir, export_config *conf) {
    const char *ptr = conf->supported_languages;
    char lang[100];
    while (*ptr == ' ') ptr++;
    while (*ptr != 0) {
        int i = 0;
        while (*ptr != 0 && *ptr != ',') {
            if (i >= 100) {
                fprintf(stderr, "conf.lua: language id too long\n");
                return false;
            }
            lang[i] = *ptr;
            i++;
            ptr++;
        }
        lang[i] = 0;
        char *name = am_format("%s/%s.lproj/x", dir, lang);
        if (!mz_zip_add_mem_to_archive_file_in_place(zipname, name, "x", 1, "", 0, 0, 0100644, ZIP_PLATFORM_UNIX)) {
            free(name);
            return false;
        }
        free(name);
        while (*ptr == ',' || *ptr == ' ') ptr++;
    }
    return true;
}

static bool resize_image(void *img_data, int in_w, int in_h, const char *dir, const char *filename, int out_w, int out_h) {
    void *out_data = malloc(out_w * out_h * 4);
    double in_aspect = (double)in_w / (double)in_h;
    double out_aspect = (double)out_w / (double)out_h;
    if (fabs(in_aspect - out_aspect) < 0.05) {
        stbir_resize_uint8((const unsigned char*)img_data, in_w, in_h, 0, (unsigned char *)out_data, out_w, out_h, 0, 4);
    } else {
        uint32_t *pxl_data = (uint32_t*)out_data;
        uint32_t bg = ((uint32_t*)img_data)[0];
        for (int i = 0; i < out_w * out_h; i++) {
            pxl_data[i] = bg;
        }
        double scaling_x = (double)out_w / (double)in_w;
        double scaling_y = (double)out_h / (double)in_h;
        double scaling = fmin(scaling_x, scaling_y);
        int scaled_w = (int)floor((double)in_w * scaling);
        int scaled_h = (int)floor((double)in_h * scaling);
        int x_os = (out_w - scaled_w) / 2;
        int y_os = (out_h - scaled_h) / 2;
        void *tmp_data = malloc(scaled_w * scaled_h * 4);
        stbir_resize_uint8((const unsigned char*)img_data, in_w, in_h, 0, (unsigned char *)tmp_data, scaled_w, scaled_h, 0, 4);
        uint32_t *tmp_pxl_data = (uint32_t*)tmp_data;
        for (int i = 0; i < scaled_w; i++) {
            for (int j = 0; j < scaled_h; j++) {
                pxl_data[(i + x_os) + (j + y_os) * out_w] = tmp_pxl_data[i + j * scaled_w];
            }
        }
        free(tmp_data);
    }
    size_t len;
    void *png_data = tdefl_write_image_to_png_file_in_memory_ex(
        out_data, out_w, out_h, 4, &len, MZ_DEFAULT_LEVEL, 0);
    free(out_data);
    char *path = am_format("%s%c%s", dir, AM_PATH_SEP, filename);
    FILE *f = fopen(path, "wb");
    free(path);
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open %s for writing\n", filename);
        return false;
    }
    fwrite(png_data, len, 1, f);
    fclose(f);
    free(png_data);
    return true;
}

static bool create_ios_icon_files(const char *dir, export_config *conf) {
    size_t len;
    stbi_uc *img_data;
    int width, height;
    if (conf->icon == NULL) {
        width = 1024;
        height = 1024;
        len = width * height * 4;
        img_data = (stbi_uc*)malloc(len);
        memset(img_data, 255, len);
    } else {
        void *data = am_read_file(conf->icon, &len);
        int components = 4;
        stbi_set_flip_vertically_on_load(0);
        img_data =
            stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
        free(data);
        if (img_data == NULL) return false;
    }
    if (!resize_image(img_data, width, height, dir, "Icon.png", 57, 57)
        || !resize_image(img_data, width, height, dir, "Icon@2x.png", 114, 114)
        || !resize_image(img_data, width, height, dir, "icon40.png", 40, 40)
        || !resize_image(img_data, width, height, dir, "icon57.png", 57, 57)
        || !resize_image(img_data, width, height, dir, "icon72.png", 72, 72)
        || !resize_image(img_data, width, height, dir, "icon76.png", 76, 76)
        || !resize_image(img_data, width, height, dir, "icon80.png", 80, 80)
        || !resize_image(img_data, width, height, dir, "icon114.png", 114, 114)
        || !resize_image(img_data, width, height, dir, "icon120.png", 120, 120)
        || !resize_image(img_data, width, height, dir, "icon144.png", 144, 144)
        || !resize_image(img_data, width, height, dir, "icon152.png", 152, 152)
        || !resize_image(img_data, width, height, dir, "icon180.png", 180, 180)
        || !resize_image(img_data, width, height, dir, "iTunesArtwork", 1024, 1024)
        ) return false;
    free(img_data);
    return true;
}

static bool create_ios_launch_images(const char *dir, export_config *conf) {
    size_t len;
    stbi_uc *img_data;
    int width, height;
    if (conf->launch_image == NULL) {
        width = 1024;
        height = 1024;
        len = width * height * 4;
        img_data = (stbi_uc*)malloc(len);
        memset(img_data, 255, len);
    } else {
        void *data = am_read_file(conf->launch_image, &len);
        int components = 4;
        stbi_set_flip_vertically_on_load(0);
        img_data =
            stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
        free(data);
        if (img_data == NULL) return false;
    }
    if (!resize_image(img_data, width, height, dir,    "Default.png", 320, 480)
        || !resize_image(img_data, width, height, dir, "Default@2x.png", 640, 960)
        || !resize_image(img_data, width, height, dir, "Default-568h@2x.png", 640, 1136)
        || !resize_image(img_data, width, height, dir, "Default-667h@2x.png", 750, 1334)
        || !resize_image(img_data, width, height, dir, "Default-736h@2x.png", 1242, 2208)
        || !resize_image(img_data, width, height, dir, "Default-Landscape@2x~ipad.png", 2048, 1536)
        || !resize_image(img_data, width, height, dir, "Default-Landscape~ipad.png", 1024, 768)
        || !resize_image(img_data, width, height, dir, "Default-Portrait@2x~ipad.png", 1536, 2048)
        || !resize_image(img_data, width, height, dir, "Default-Portrait~ipad.png", 768, 1024)
        ) return false;
    free(img_data);
    return true;
}

#endif
