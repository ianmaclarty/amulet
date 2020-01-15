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
    const char *zipdir;
    const char *version;
    const char *display_name;
    const char *dev_region;
    const char *supported_languages;
    am_display_orientation orientation;
    const char *launch_image;
    // each time we generate an iOS launch image we give it a unique suffix
    // to avoid a bug where if the same file name is used, then image isn't
    // updated on the device.
    unsigned int launch_image_id;
    const char *grade;
    char *basepath;
    const char *mac_category;
    bool recurse;
    char *outdir;
    const char *outpath;
    bool allfiles;
};

static bool create_mac_info_plist(const char *binpath, const char *filename, export_config *conf);
static bool create_mac_icons(export_config *conf);
static bool create_mac_entitlements(export_config *conf);
static bool create_mac_lproj_dirs(const char *zipname, const char *dir, export_config *conf);

static bool create_ios_xcode_icon_files(const char *dir, export_config *conf);
static bool create_ios_xcode_lproj_dirs(const char *dir, export_config *conf);
static char* get_ios_xcodeproj_lang_list(export_config *conf);
static char* get_ios_launchscreen_entries(export_config *conf);
static char* get_ios_launchscreen_children(export_config *conf);

static bool create_android_proj_icon_files(const char *dir, export_config *conf);
static bool create_android_proj_lang_dirs(const char *dir, export_config *conf);

static char *get_bin_path(export_config *conf, const char *platform);

static void replace_backslashes(char *str) {
    for (unsigned int i = 0; i < strlen(str); i++) {
        if (str[i] == '\\') str[i] = '/';
    }
}

static void sub_win_newlines(void **ptr, size_t *len) {
    char *str = (char*)(*ptr);
    unsigned int n = (unsigned int)(*len);
    unsigned int num_subs = 0;
    for (unsigned int i = 0; i < n; i++) {
        if (str[i] == '\n') num_subs++;
        if (str[i] == '\r') i++;
    }
    unsigned int new_n = n + num_subs;
    char *new_str = (char*)malloc(new_n);
    unsigned int j = 0;
    for (unsigned int i = 0; i < n; i++) {
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

static bool copy_text_file(const char *from_path, const char *to_path, char **substitutions) {
    char *data = (char*)am_read_file(from_path, NULL);
    if (data == NULL) return false;
    if (substitutions != NULL) {
        char *new_data = am_replace_strings(data, substitutions);
        free(data);
        data = new_data;
    }
    bool result = am_write_text_file(to_path, data);
    free(data);
    return result;
}

static bool copy_bin_file(const char *from_path, const char *to_path) {
    size_t len;
    void *data = am_read_file(from_path, &len);
    if (data == NULL) return false;
    bool result = am_write_bin_file(to_path, data, len);
    free(data);
    return result;
}

static bool copy_platform_bin_file(char *dir, export_config *conf, const char *file, const char *platform) {
    char *binpath = get_bin_path(conf, platform);
    if (binpath == NULL) return false;
    char *srcpath = am_format("%s/%s", binpath, file);
    char *destpath = am_format("%s/%s", dir, file);
    bool ok = copy_bin_file(srcpath, destpath);
    free(binpath);
    free(srcpath);
    free(destpath);
    return ok;
}

static bool add_files_to_pak(const char *zipfile, const char *rootdir, const char *dir, const char *pat, uint8_t platform) {
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
        int namelen = strlen(name);
        if (namelen > 1 && name[namelen-1] == '~') continue; // always ignore vim backup files
        const char *namesuffix = namelen > 4 ? name + namelen - 4 : "";
        // don't compress .png, .jpg or .ogg as they are already compressed
        bool compress = strcmp(namesuffix, ".png") != 0 && strcmp(namesuffix, ".jpg") != 0 && strcmp(namesuffix, ".ogg") != 0;
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, name, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, 0100644, platform)) {
            free(buf);
            fprintf(stderr, "Error: failed to add %s to archive %s\n", file, zipfile);
            return false;
        }
        free(buf);
        printf("Added %s\n", name);
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
        char *dirprefix;
        if (newdir == NULL) {
            dirprefix = am_format("%s", "");
        } else {
            dirprefix = am_format("%s/", newdir);
        }
        if (newname == NULL) {
            name = am_format("%s%s", dirprefix, file + strlen(dir) + 1);
        } else if (newext == NULL) {
            name = am_format("%s%s", dirprefix, newname);
        } else {
            name = am_format("%s%s%s", dirprefix, newname, newext);
        }
        free(dirprefix);
        if (!mz_zip_add_mem_to_archive_file_in_place(zipfile, name, buf, len, "", 0, compress ? MZ_BEST_COMPRESSION : 0, executable ? 0100755 : 0100644, platform)) {
            free(buf);
            free(name);
            fprintf(stderr, "Error: failed to add %s to archive %s\n", file, zipfile);
            return false;
        }
        free(name);
        free(buf);
    }
    return true;
}

static bool add_data_files_to_zip(export_config *conf, const char *zipfile, const char *rootdir, const char *dir) {
    if (conf->allfiles) {
        return add_files_to_pak(zipfile, rootdir, dir, "*", ZIP_PLATFORM_DOS);
    } else {
        return
            add_files_to_pak(zipfile, rootdir, dir, "*.lua", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.png", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.jpg", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.ogg", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.obj", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.json", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.frag", ZIP_PLATFORM_DOS) &&
            add_files_to_pak(zipfile, rootdir, dir, "*.vert", ZIP_PLATFORM_DOS) &&
            true;
    }
}

static bool build_data_pak_2(export_config *conf, int level, const char *rootdir, const char *dir, const char *pakfile) {
    if (level >= RECURSE_LIMIT) {
        fprintf(stderr, "Error: maximum directory recursion depth reached (%d)\n", RECURSE_LIMIT);
        return false;
    }
    if (!add_data_files_to_zip(conf, pakfile, rootdir, dir)) {
        return false;
    }
    if (conf->recurse) {
        CSimpleGlobTempl<char> glob(SG_GLOB_ONLYDIR | SG_GLOB_NODOT);
        char *pattern = am_format("%s%c*", dir, AM_PATH_SEP);
        glob.Add(pattern);
        free(pattern);
        for (int n = 0; n < glob.FileCount(); ++n) {
            char *subdir = glob.File(n);
            if (!build_data_pak_2(conf, level + 1, rootdir, subdir, pakfile)) {
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
    printf("Exporting project...\n");
    return build_data_pak_2(conf, 0, am_opt_data_dir, am_opt_data_dir, conf->pakfile);
}

static bool copy_files_to_dir(const char *rootdir, const char *dest_dir, const char *src_dir, const char *pat) {
    CSimpleGlobTempl<char> glob(SG_GLOB_ONLYFILE);
    char *pattern = am_format("%s%c%s", src_dir, AM_PATH_SEP, pat);
    glob.Add(pattern);
    free(pattern);
    for (int n = 0; n < glob.FileCount(); ++n) {
        char *file = glob.File(n);
        char *name = file + strlen(rootdir) + 1;
        char *dest_path = am_format("%s/%s", dest_dir, name);
        bool ok = copy_bin_file(file, dest_path);
        free(dest_path);
        if (!ok) return false;
        printf("Added %s\n", name);
    }
    return true;
}


static bool copy_data_files_to_dir(export_config *conf, const char *dest_dir, const char *rootdir, const char *src_dir) {
    if (strlen(rootdir) < strlen(src_dir)) {
        char *dest_subdir = am_format("%s/%s", dest_dir, src_dir + strlen(rootdir) + 1);
        am_make_dir(dest_subdir);
        free(dest_subdir);
    }
    if (conf->allfiles) {
        return copy_files_to_dir(rootdir, dest_dir, src_dir, "*");
    } else {
        return
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.lua") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.png") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.jpg") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.ogg") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.obj") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.json") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.frag") &&
            copy_files_to_dir(rootdir, dest_dir, src_dir, "*.vert") &&
            true;
    }
}

static bool copy_data_files_2(export_config *conf, int level, const char *rootdir, const char *src_dir, const char *dest_dir) {
    if (level >= RECURSE_LIMIT) {
        fprintf(stderr, "Error: maximum directory recursion depth reached (%d)\n", RECURSE_LIMIT);
        return false;
    }
    if (!copy_data_files_to_dir(conf, dest_dir, rootdir, src_dir)) {
        return false;
    }
    if (conf->recurse) {
        CSimpleGlobTempl<char> glob(SG_GLOB_ONLYDIR | SG_GLOB_NODOT);
        char *pattern = am_format("%s%c*", src_dir, AM_PATH_SEP);
        glob.Add(pattern);
        free(pattern);
        for (int n = 0; n < glob.FileCount(); ++n) {
            char *src_subdir = glob.File(n);
            if (!copy_data_files_2(conf, level + 1, rootdir, src_subdir, dest_dir)) {
                return false;
            }
        }
    }
    return true;
}

static bool copy_data_files(export_config *conf, const char *dir) {
    return copy_data_files_2(conf, 0, am_opt_data_dir, am_opt_data_dir, dir);
}

static const char *platform_luavm(const char *platform) {
    if (am_conf_luavm != NULL) return am_conf_luavm;
    if (strcmp(platform, "msvc32") == 0) return "luajit";
    if (strcmp(platform, "linux32") == 0) return "luajit";
    if (strcmp(platform, "linux64") == 0) return "luajit";
    if (strcmp(platform, "osx") == 0) return "luajit";
    return "lua51";
}

static char *get_base_path() {
    char *base = am_get_base_path();
    if (strcmp(base, "/usr/local/bin/") == 0) {
        free(base);
        return am_format("%s", "/usr/local/share/amulet/");
    } else if (strcmp(base, "/usr/bin/") == 0) {
        free(base);
        return am_format("%s", "/usr/share/amulet/");
    } else {
        return base;
    }
}

static char *get_bin_path(export_config *conf, const char *platform) {
    char *base_path = conf->basepath;
    const char *luavm = platform_luavm(platform);
    char *bin_path = am_format("%sbuilds/%s/%s/%s/bin", base_path, platform, luavm, conf->grade);
    if (!am_file_exists(bin_path)) {
        printf("WARNING: unable to find %s\n", bin_path);
        free(bin_path);
        return NULL;
    }
    return bin_path;
}

static char *get_template_path(export_config *conf) {
    char *base_path = conf->basepath;
    char *templ_path = am_format("%stemplates/", base_path);
    if (!am_file_exists(templ_path)) {
        printf("WARNING: unable to find %s\n", templ_path);
        free(templ_path);
        return NULL;
    }
    return templ_path;
}

static char *get_export_zip_name(export_config *conf, const char *platname) {
    if (conf->outpath != NULL) {
        return am_format("%s", conf->outpath);
    } else {
        return am_format("%s%s-%s-%s.zip", conf->outdir, conf->shortname, conf->version, platname);
    }
}

static bool gen_windows_export(export_config *conf, const char *zipsuffix, const char *platform) {
    char *zipname = get_export_zip_name(conf, zipsuffix);
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, platform);
    if (binpath == NULL) return true;
    const char *name = conf->shortname;
    const char *zipdir = conf->zipdir;
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, binpath, "amulet.exe", zipdir, name, ".exe", true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, binpath, "*.dll", zipdir, NULL, NULL, true, true, ZIP_PLATFORM_DOS) &&
        add_files_to_dist(zipname, ".", conf->pakfile, zipdir, "data.pak", NULL, false, false, ZIP_PLATFORM_DOS) &&
        true;
    if (ok) printf("Generated %s\n", zipname);
    free(zipname);
    free(binpath);
    return ok;
}

static bool gen_mac_export(export_config *conf, bool print_message) {
    char *zipname = get_export_zip_name(conf, "mac");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "osx");
    if (binpath == NULL) return true;
    if (!create_mac_info_plist(binpath, AM_TMP_DIR AM_PATH_SEP_STR "Info.plist", conf)) return false;
    bool icon_created = create_mac_icons(conf);
    const char *name = conf->title;
    const char *zipdir = conf->zipdir;
    char *resource_dir;
    char *exe_dir;
    if (zipdir != NULL) {
        resource_dir = am_format("%s/%s.app/Contents/Resources", zipdir, name);
        exe_dir = am_format("%s/%s.app/Contents/MacOS", zipdir, name);
    } else {
        resource_dir = am_format("%s.app/Contents/Resources", name);
        exe_dir = am_format("%s.app/Contents/MacOS", name);
    }
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet", exe_dir, NULL, NULL, true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "*.dylib", exe_dir, NULL, NULL, true, true, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, zipdir, name, ".app/Contents/Resources/data.pak", false, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, AM_TMP_DIR, "Info.plist", zipdir, name, ".app/Contents/Info.plist", true, false, ZIP_PLATFORM_UNIX) &&
        (
        (icon_created &&
            add_files_to_dist(zipname, AM_TMP_DIR, "icon.icns", zipdir, name, ".app/Contents/Resources/icon.icns", true, false, ZIP_PLATFORM_UNIX))
        ||
        ((!icon_created) &&
            add_files_to_dist(zipname, binpath, "amulet.icns", zipdir, name, ".app/Contents/Resources/icon.icns", true, false, ZIP_PLATFORM_UNIX))
        ) &&
        create_mac_lproj_dirs(zipname, resource_dir, conf) &&
        true;
    am_delete_file(AM_TMP_DIR AM_PATH_SEP_STR "Info.plist");
    if (print_message && ok) printf("Generated %s\n", zipname);
    free(zipname);
    free(binpath);
    free(resource_dir);
    free(exe_dir);
    return ok;
}

static bool gen_mac_app_store_export(export_config *conf) {
    if (am_conf_mac_application_cert_identity == NULL) {
        fprintf(stderr, "Error: please set mac_application_cert_identity in conf.lua\n");
        return false;
    }
    if (am_conf_mac_installer_cert_identity == NULL) {
        fprintf(stderr, "Error: please set mac_installer_cert_identity in conf.lua\n");
        return false;
    }
    if (!gen_mac_export(conf, false)) {
        return false;
    }
    am_execute_shell_cmd("rm -rf %s/*", AM_TMP_DIR);
    if (!create_mac_entitlements(conf)) return false;
    char *zipname = get_export_zip_name(conf, "mac");
    if (!am_execute_shell_cmd("unzip %s -d %s", zipname, AM_TMP_DIR)) {
        am_delete_file(zipname);
        free(zipname);
        return false;
    }
    am_delete_file(zipname);
    free(zipname);
    char *pkgfile;
    if (conf->outpath != NULL) {
        pkgfile = am_format("%s", conf->outpath);
    } else {
        pkgfile = am_format("%s%s-%s-mac-app-store.pkg", conf->outdir, conf->shortname, conf->version);
    }
    if (!am_execute_shell_cmd("cd %s/%s && codesign -f --deep -s '%s' --entitlements ../%s.entitlements '%s.app'",
        AM_TMP_DIR, conf->shortname, am_conf_mac_application_cert_identity, conf->shortname, conf->title)
    ) {
        return false;
    }
    const char *pkgdir = (pkgfile[0] == '/') ? "" : "../../";
    if (!am_execute_shell_cmd("cd %s/%s && productbuild --component '%s.app' /Applications --sign '%s' %s%s",
        AM_TMP_DIR, conf->shortname, conf->title, am_conf_mac_installer_cert_identity, pkgdir, pkgfile)
    ) {
        return false;
    }
    printf("Generated %s\n", pkgfile);
    free(pkgfile);
    return true;
}

static bool copy_ios_xcodeproj_file(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sios/project.pbxproj", template_dir);
    char *dest_path = am_format("%s/project.pbxproj", dir);
    char *lang_list = get_ios_xcodeproj_lang_list(conf);
    char *launchscreen_entries = get_ios_launchscreen_entries(conf);
    char *launchscreen_children = get_ios_launchscreen_children(conf);
    const char *luavm = platform_luavm("ios");
    char *launch_img_name = am_format("launchimg_%u.png", conf->launch_image_id);
    char *capabilities = am_format(
        "{\n"
        "    com.apple.GameCenter = {\n"
        "            enabled = %d;\n"
        "    };\n"
        "    com.apple.GameCenter.iOS = {\n"
        "            enabled = %d;\n"
        "    };\n"
        "    com.apple.iCloud = {\n"
        "            enabled = %d;\n"
        "    };\n"
        "}\n",
        am_conf_game_center_enabled ? 1 : 0, am_conf_game_center_enabled ? 1 : 0, am_conf_icloud_enabled ? 1 : 0);
    const char *subs[] = {
        "AM_APPNAME", conf->shortname,
        "AM_APPID", am_conf_app_id_ios,
        "AM_AUTHOR", am_conf_app_author,
        "AM_DEV_CERT_ID", am_conf_ios_dev_cert_identity,
        "AM_APPSTORE_CERT_ID", am_conf_ios_appstore_cert_identity,
        "AM_CODE_SIGN_IDENTITY", am_conf_ios_code_sign_identity,
        "AM_DEV_PROV_PROFILE_NAME", am_conf_ios_dev_prov_profile_name,
        "AM_DIST_PROV_PROFILE_NAME", am_conf_ios_dist_prov_profile_name,
        "AM_LANG_LIST", lang_list,
        "AM_DEV_REGION", conf->dev_region,
        "AM_LAUNCHSCREEN_STORYBOARD_STRINGS_ENTRIES", launchscreen_entries,
        "AM_LAUNCHSCREEN_STORYBOARD_CHILDREN", launchscreen_children,
        "AM_LUAVM", luavm,
        "AM_LAUNCH_IMG", launch_img_name,
        "AM_SYSTEM_CAPABILITIES", capabilities,
        NULL
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    free(lang_list);
    free(launchscreen_entries);
    free(launchscreen_children);
    free(launch_img_name);
    free(capabilities);
    return ok;
}

const char *get_ios_orientation_xml(export_config *conf) {
    switch (conf->orientation) {
        case AM_DISPLAY_ORIENTATION_ANY:
            return
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
        case AM_DISPLAY_ORIENTATION_PORTRAIT:
            return
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
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            return
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
        case AM_DISPLAY_ORIENTATION_HYBRID:
            return
                "<key>UISupportedInterfaceOrientations</key>"
                "<array>"
                "<string>UIInterfaceOrientationPortrait</string>"
                "</array>"
                "<key>UISupportedInterfaceOrientations~ipad</key>"
                "<array>"
		"<string>UIInterfaceOrientationLandscapeLeft</string>"
		"<string>UIInterfaceOrientationLandscapeRight</string>"
                "</array>";
    }
    return "";
}

static bool copy_ios_xcode_info_plist(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sios/Info.plist", template_dir);
    char *dest_path = am_format("%s/Info.plist", dir);
    const char *orientation_xml = get_ios_orientation_xml(conf);
    const char *subs[] = {
        "AM_APPDISPLAYNAME", conf->display_name,
        "AM_APPVERSION", conf->version,
        "AM_APPORIENTATIONS", orientation_xml,
        NULL,
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    return ok;
}

static bool copy_ios_entitlements(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sios/app.entitlements", template_dir);
    char *dest_path = am_format("%s/%s.entitlements", dir, conf->shortname);
    const char *icloud_xml = am_conf_icloud_enabled ? 
       "    <key>com.apple.developer.icloud-container-identifiers</key>\n"
       "    <array/>\n"
       "    <key>com.apple.developer.ubiquity-kvstore-identifier</key>\n"
       "    <string>$(TeamIdentifierPrefix)$(CFBundleIdentifier)</string>\n" : "";
    const char *subs[] = {
        "AM_ICLOUD_ENTITLEMENT", icloud_xml,
        NULL,
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    return ok;
}

static char *get_default_ios_launch_image_filename(export_config *conf) {
    char *templates_dir = get_template_path(conf);
    if (templates_dir == NULL) return NULL;
    char *filename = am_format("%sios/default_launch_image.png", templates_dir);
    free(templates_dir);
    return filename;
}

static char *get_ios_launch_image_filename(export_config *conf) {
    if (conf->launch_image == NULL) {
        return get_default_ios_launch_image_filename(conf);
    } else {
        if (am_file_exists(conf->launch_image)) {
            return am_format("%s", conf->launch_image);
        } else {
            fprintf(stderr, "Error: launch image %s not found\n", conf->launch_image);
            return NULL;
        }
    }
}

static bool read_image_first_pixel_and_size(char *filename, uint8_t *red, uint8_t *green, uint8_t *blue, int *w, int *h) {
    *red = 0;
    *green = 0;
    *blue = 0;
    size_t len;
    void *data = am_read_file(filename, &len);
    if (data == NULL) return false;
    int components = 4;
    stbi_set_flip_vertically_on_load(0);
    uint8_t *img_data = (uint8_t*)stbi_load_from_memory((stbi_uc const *)data, len, w, h, &components, 4);
    free(data);
    if (img_data == NULL) return false;
    *red = img_data[0];
    *green = img_data[1];
    *blue = img_data[2];
    free(img_data);
    return true;
}

static bool copy_ios_launchscreen_storyboard(char *projbase_dir, export_config *conf) {
    char *launch_img_filename = get_ios_launch_image_filename(conf);
    if (launch_img_filename == NULL) return false;
    uint8_t red, green, blue;
    int width, height;
    if (!read_image_first_pixel_and_size(launch_img_filename, &red, &green, &blue, &width, &height)) {
        free(launch_img_filename);
        return false;
    }
    char *red_str = am_format("%f", ((float)red) / 255.0f);
    char *green_str = am_format("%f", ((float)green) / 255.0f);
    char *blue_str = am_format("%f", ((float)blue) / 255.0f);
    char *width_str = am_format("%d", width);
    char *height_str = am_format("%d", height);
    char *launch_img_name = am_format("launchimg_%u.png", conf->launch_image_id);

    const char *subs[] = {
        "AM_LAUNCH_BG_RED", (const char*)red_str,
        "AM_LAUNCH_BG_GREEN", (const char*)green_str,
        "AM_LAUNCH_BG_BLUE", (const char*)blue_str,
        "AM_LAUNCH_IMG_W", (const char*)width_str,
        "AM_LAUNCH_IMG_H", (const char*)height_str,
        "AM_LAUNCH_IMG_NAME", (const char*)launch_img_name,
        NULL
    };

    // use a different name for the launch image each time,
    // to avoid a bug where the image is not uploaded on the device
    // if the same filename is used.
    char *launch_img_dest_path = am_format("%s/%s", projbase_dir, launch_img_name);

    char *templates_dir = get_template_path(conf);
    if (templates_dir == NULL) return false;
    char *storyboard_src_path = am_format("%sios/LaunchScreen.storyboard", templates_dir);
    char *base_lproj_dir = am_format("%s/Base.lproj", projbase_dir);
    char *storyboard_dest_path = am_format("%s/LaunchScreen.storyboard", base_lproj_dir);
    am_make_dir(base_lproj_dir);
    bool ok =
        am_execute_shell_cmd("rm -f %s/launchimg_*.png", projbase_dir) &&
        copy_text_file(storyboard_src_path, storyboard_dest_path, (char**)subs) &&
        copy_bin_file(launch_img_filename, launch_img_dest_path);

    free(red_str);
    free(green_str);
    free(blue_str);
    free(width_str);
    free(height_str);
    free(launch_img_filename);
    free(launch_img_name);
    free(launch_img_dest_path);
    free(templates_dir);
    free(storyboard_src_path);
    free(storyboard_dest_path);
    return ok;
}

static bool copy_ios_xcode_bin_file(char *dir, export_config *conf, const char *file) {
    char *binpath = get_bin_path(conf, "ios");
    if (binpath == NULL) return false;
    char *srcpath = am_format("%s/%s", binpath, file);
    char *destpath = am_format("%s/%s", dir, file);
    bool ok = copy_bin_file(srcpath, destpath);
    free(binpath);
    free(srcpath);
    free(destpath);
    return ok;
}

static bool copy_ios_xcode_template_file(char *dir, export_config *conf, const char *file) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *srcpath = am_format("%sios/%s", template_dir, file);
    char *destpath = am_format("%s/%s", dir, file);
    bool ok = copy_text_file(srcpath, destpath, NULL);
    free(template_dir);
    free(srcpath);
    free(destpath);
    return ok;
}

static bool copy_ios_xcode_pak_file(char *dir, export_config *conf) {
    const char *srcpath = conf->pakfile;
    char *destpath = am_format("%s/data.pak", dir);
    bool ok = copy_bin_file((char*)srcpath, destpath);
    free(destpath);
    return ok;
}

static bool gen_ios_xcode_proj(export_config *conf) {
    if (am_conf_app_icon_ios == NULL) {
        fprintf(stderr, "Error: please set icon_ios or icon in conf.lua\n");
        return false;
    }
    if (am_conf_ios_cert_identity == NULL) {
        fprintf(stderr, "Error: please set ios_cert_identity in conf.lua\n");
        return false;
    }
    if (am_conf_ios_dev_prov_profile_name == NULL) {
        fprintf(stderr, "Error: please set ios_dev_prov_profile_name in conf.lua\n");
        return false;
    }
    if (am_conf_ios_dist_prov_profile_name == NULL) {
        fprintf(stderr, "Error: please set ios_dist_prov_profile_name in conf.lua\n");
        return false;
    }
    char *templates_dir = get_template_path(conf);
    if (templates_dir == NULL) return false;
    char *projbase_dir;
    if (conf->outpath) {
        projbase_dir = am_format("%s", conf->outpath);
    } else {
        projbase_dir = am_format("%s%s-%s-ios", conf->outdir, conf->shortname, conf->version);
    }
    char *xcodeproj_dir = am_format("%s/%s.xcodeproj", projbase_dir, conf->shortname);
    char *assets_dir = am_format("%s/Assets.xcassets", projbase_dir);
    char *appicon_assets_dir = am_format("%s/AppIcon.appiconset", assets_dir);
    char *appicon_contents_src_path = am_format("%sios/AppIconContents.json", templates_dir);
    char *appicon_contents_dest_path = am_format("%s/Contents.json", appicon_assets_dir);
    const char *lua_a_file = am_format("%s.a", platform_luavm("ios"));
    am_make_dir(projbase_dir);
    am_make_dir(xcodeproj_dir);
    am_make_dir(assets_dir);
    am_make_dir(appicon_assets_dir);
    bool ok =
        copy_ios_xcodeproj_file(conf, xcodeproj_dir) &&
        copy_ios_xcode_info_plist(conf, projbase_dir) &&
        copy_ios_entitlements(conf, projbase_dir) &&
        create_ios_xcode_lproj_dirs(projbase_dir, conf) &&
        create_ios_xcode_icon_files(appicon_assets_dir, conf) &&
        copy_text_file(appicon_contents_src_path, appicon_contents_dest_path, NULL) &&
        copy_ios_launchscreen_storyboard(projbase_dir, conf) &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "amulet.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "glslopt.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "kissfft.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, lua_a_file) &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "stb.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "tinymt.a") &&
        copy_ios_xcode_template_file(projbase_dir, conf, "amulet_custom_init.mm") &&
        copy_ios_xcode_template_file(projbase_dir, conf, "amulet_simple_lua.h") &&
        copy_ios_xcode_pak_file(projbase_dir, conf) &&
        true;
    if (ok) printf("Generated %s\n", projbase_dir);
    free((char*)lua_a_file);
    free(templates_dir);
    free(projbase_dir);
    free(xcodeproj_dir);
    free(assets_dir);
    free(appicon_assets_dir);
    free(appicon_contents_src_path);
    free(appicon_contents_dest_path);
    return ok;
}

const char *get_android_orientation_str(export_config *conf) {
    switch (conf->orientation) {
        case AM_DISPLAY_ORIENTATION_ANY:
            return "fullUser";
        case AM_DISPLAY_ORIENTATION_PORTRAIT:
            return "portrait";
        case AM_DISPLAY_ORIENTATION_LANDSCAPE:
            return "landscape";
        case AM_DISPLAY_ORIENTATION_HYBRID:
            return "portrait";
    }
    return "portrait";
}

static bool copy_android_project_iml(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sandroid/Project.iml", template_dir);
    char *dest_path = am_format("%s/%s.iml", dir, conf->shortname);
    const char *subs[] = {
        "AM_SHORTNAME", conf->shortname,
        NULL
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    return ok;
}

static bool copy_android_app_build_gradle(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sandroid/app/build.gradle", template_dir);
    char *dest_path = am_format("%s/build.gradle", dir);
    const char *billing_dep = "";
    if (am_conf_android_needs_billing_permission) {
        billing_dep = "implementation 'com.android.billingclient:billing:2.1.0'";
    }
    const char *subs[] = {
        "AM_APPID", am_conf_app_id_android,
        "AM_APPVERSION_CODE", am_conf_android_app_version_code,
        "AM_TARGET_SDK_VER", am_conf_android_target_sdk_version,
        "AM_APPVERSION", conf->version,
        "AM_BILLING_DEPENDENCY", billing_dep,
        NULL
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    return ok;
}

static bool copy_android_manifest(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sandroid/app/src/main/AndroidManifest.xml", template_dir);
    char *dest_path = am_format("%s/AndroidManifest.xml", dir);
    const char *subs[] = {
        "AM_APPID", am_conf_app_id_android,
        "AM_APPVERSION_CODE", am_conf_android_app_version_code,
        "AM_APPVERSION", conf->version,
        "AM_TITLE", conf->display_name,
        "AM_ORIENTATION", get_android_orientation_str(conf),
        "AM_INTERNET_PERMISSION", am_conf_android_needs_internet_permission ? "<uses-permission android:name=\"android.permission.INTERNET\"/>" : "",
        "AM_BILLING_PERMISSION", am_conf_android_needs_billing_permission ? "<uses-permission android:name=\"com.android.vending.BILLING\" />" : "",
        NULL
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    return ok;
}

static bool copy_android_strings_xml(export_config *conf, char *dir) {
    char *template_dir = get_template_path(conf);
    if (template_dir == NULL) return false;
    char *src_path = am_format("%sandroid/app/src/main/res/values/strings.xml", template_dir);
    char *dest_path = am_format("%s/strings.xml", dir);
    const char *subs[] = {
        "AM_GOOGLE_PLAY_GAME_SERVICES_ID", am_conf_google_play_services_id,
        NULL
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    return ok;
}

static bool gen_android_studio_proj(export_config *conf) {
    if (am_conf_app_icon_android == NULL) {
        fprintf(stderr, "Error: please set android_icon or icon in conf.lua\n");
        return false;
    }
    if (am_conf_app_id_android == NULL) {
        fprintf(stderr, "Error: please set appid_android or appid in conf.lua\n");
        return false;
    }
    char *templates_dir = get_template_path(conf);
    if (templates_dir == NULL) return false;
    char *projbase_dir;
    if (conf->outpath) {
        projbase_dir = am_format("%s", conf->outpath);
    } else {
        projbase_dir = am_format("%s%s-%s-android", conf->outdir, conf->shortname, conf->version);
    }
    char *app_dir = am_format("%s/app", projbase_dir);
    char *src_dir = am_format("%s/app/src", projbase_dir);
    char *main_dir = am_format("%s/app/src/main", projbase_dir);
    char *res_dir = am_format("%s/app/src/main/res", projbase_dir);
    char *values_dir = am_format("%s/app/src/main/res/values", projbase_dir);
    char *java_dir = am_format("%s/app/src/main/java", projbase_dir);
    char *java_xyz_dir = am_format("%s/app/src/main/java/xyz", projbase_dir);
    char *java_xyz_amulet_dir = am_format("%s/app/src/main/java/xyz/amulet", projbase_dir);
    char *assets_dir = am_format("%s/app/src/main/assets", projbase_dir);
    char *jnilibs_dir = am_format("%s/app/src/main/jniLibs", projbase_dir);
    char *jnilibs_arm32_dir = am_format("%s/app/src/main/jniLibs/armeabi-v7a", projbase_dir);
    char *jnilibs_arm64_dir = am_format("%s/app/src/main/jniLibs/arm64-v8a", projbase_dir);
    char *jnilibs_x86_dir = am_format("%s/app/src/main/jniLibs/x86", projbase_dir);
    char *jnilibs_x86_64_dir = am_format("%s/app/src/main/jniLibs/x86_64", projbase_dir);

    char *proguard_rules_src_path = am_format("%sandroid/app/proguard-rules.pro", templates_dir);
    char *proguard_rules_dst_path = am_format("%s/app/proguard-rules.pro", projbase_dir);
    char *activity_java_src_path = am_format("%sandroid/app/src/main/java/xyz/amulet/AmuletActivity.java", templates_dir);
    char *activity_java_dst_path = am_format("%s/app/src/main/java/xyz/amulet/AmuletActivity.java", projbase_dir);
    char *app_iml_src_path = am_format("%sandroid/app.iml", templates_dir);
    char *app_iml_dst_path = am_format("%s/app.iml", projbase_dir);
    char *build_gradle_src_path = am_format("%sandroid/build.gradle", templates_dir);
    char *build_gradle_dst_path = am_format("%s/build.gradle", projbase_dir);
    char *gradle_properties_src_path = am_format("%sandroid/gradle.properties", templates_dir);
    char *gradle_properties_dst_path = am_format("%s/gradle.properties", projbase_dir);
    char *settings_gradle_src_path = am_format("%sandroid/settings.gradle", templates_dir);
    char *settings_gradle_dst_path = am_format("%s/settings.gradle", projbase_dir);

    const char *activity_billing_subs[] = {
        "//BILLING", "",
        NULL
    };

    char **activity_subs = am_conf_android_needs_billing_permission ? (char**)activity_billing_subs : NULL;

    am_make_dir(projbase_dir);
    am_make_dir(app_dir);
    am_make_dir(src_dir);
    am_make_dir(main_dir);
    am_make_dir(res_dir);
    am_make_dir(values_dir);
    am_make_dir(java_dir);
    am_make_dir(java_xyz_dir);
    am_make_dir(java_xyz_amulet_dir);
    am_make_dir(assets_dir);
    am_make_dir(jnilibs_dir);
    am_make_dir(jnilibs_arm32_dir);
    am_make_dir(jnilibs_arm64_dir);
    am_make_dir(jnilibs_x86_dir);
    am_make_dir(jnilibs_x86_64_dir);

    bool ok =
        copy_android_project_iml(conf, projbase_dir) &&
        copy_android_app_build_gradle(conf, app_dir) &&
        copy_android_manifest(conf, main_dir) &&
        copy_android_strings_xml(conf, values_dir) &&
        create_android_proj_lang_dirs(projbase_dir, conf) &&
        create_android_proj_icon_files(res_dir, conf) &&
        copy_text_file(proguard_rules_src_path, proguard_rules_dst_path, NULL) &&
        copy_text_file(activity_java_src_path, activity_java_dst_path, activity_subs) &&
        copy_text_file(app_iml_src_path, app_iml_dst_path, NULL) &&
        copy_text_file(build_gradle_src_path, build_gradle_dst_path, NULL) &&
        copy_text_file(gradle_properties_src_path, gradle_properties_dst_path, NULL) &&
        copy_text_file(settings_gradle_src_path, settings_gradle_dst_path, NULL) &&
        copy_platform_bin_file(jnilibs_arm32_dir, conf, "libamulet.so", "android_arm32") &&
        copy_platform_bin_file(jnilibs_arm64_dir, conf, "libamulet.so", "android_arm64") &&
        copy_platform_bin_file(jnilibs_x86_dir, conf, "libamulet.so", "android_x86") &&
        copy_platform_bin_file(jnilibs_x86_64_dir, conf, "libamulet.so", "android_x86_64") &&
        copy_data_files(conf, assets_dir) &&
        true;
    if (ok) printf("Generated %s\n", projbase_dir);

    free(templates_dir);
    free(projbase_dir);
    free(app_dir);
    free(src_dir);
    free(main_dir);
    free(res_dir);
    free(values_dir);
    free(java_dir);
    free(java_xyz_dir);
    free(java_xyz_amulet_dir);
    free(assets_dir);
    free(jnilibs_dir);
    free(jnilibs_arm32_dir);
    free(jnilibs_arm64_dir);
    free(jnilibs_x86_dir);
    free(jnilibs_x86_64_dir);

    free(proguard_rules_src_path);
    free(proguard_rules_dst_path);
    free(activity_java_src_path);
    free(activity_java_dst_path);
    free(app_iml_src_path);
    free(app_iml_dst_path);
    free(build_gradle_src_path);
    free(build_gradle_dst_path);
    free(gradle_properties_src_path);
    free(gradle_properties_dst_path);
    free(settings_gradle_src_path);
    free(settings_gradle_dst_path);

    return ok;
}

static bool gen_linux_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "linux");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath64 = get_bin_path(conf, "linux64");
    char *binpath32 = get_bin_path(conf, "linux32");
    if (binpath64 == NULL && binpath32 == NULL) return true;
    const char *name = conf->shortname;
    const char *zipdir = conf->zipdir;
    char *lib32_dir;
    char *lib64_dir;
    if (conf->zipdir != NULL) {
        lib32_dir = am_format("%s/lib.i686", zipdir);
        lib64_dir = am_format("%s/lib.x86_64", zipdir);
    } else {
        lib32_dir = am_format("%s", "lib.i686");
        lib64_dir = am_format("%s", "lib.x86_64");
    }
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath64, "amulet_license.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, zipdir, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        true;
    if (ok && binpath32 != NULL) {
        ok = add_files_to_dist(zipname, binpath32, "amulet", zipdir, name, ".i686", true, true, ZIP_PLATFORM_UNIX)
            && add_files_to_dist(zipname, binpath32, "*.so", lib32_dir, NULL, NULL, true, true, ZIP_PLATFORM_UNIX);
    }
    if (ok && binpath64 != NULL) {
        ok = add_files_to_dist(zipname, binpath64, "amulet", zipdir, name, ".x86_64", true, true, ZIP_PLATFORM_UNIX)
            && add_files_to_dist(zipname, binpath64, "amulet.sh", zipdir, name, "", true, true, ZIP_PLATFORM_UNIX)
            && add_files_to_dist(zipname, binpath64, "*.so", lib64_dir, NULL, NULL, true, true, ZIP_PLATFORM_UNIX);
    }
    if (ok) printf("Generated %s\n", zipname);
    free(zipname);
    if (binpath32 != NULL) free(binpath32);
    if (binpath64 != NULL) free(binpath64);
    free(lib32_dir);
    free(lib64_dir);
    return ok;
}

static bool gen_data_pak_export(export_config *conf) {
    char *dest_path;
    if (conf->outpath != NULL) {
        dest_path = am_format("%s", conf->outpath);
    } else {
        dest_path = am_format("%sdata.pak", conf->outdir);
    }
    bool ok = copy_bin_file(conf->pakfile, dest_path);
    if (ok) printf("Generated %s\n", dest_path);
    free(dest_path);
    return ok;
}

static bool gen_html_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "html");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "html");
    if (binpath == NULL) return true;
    const char *zipdir = conf->zipdir;
    bool ok =
        add_files_to_dist(zipname, am_opt_data_dir, "*.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet_license.txt", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet.js", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "amulet.wasm", zipdir, NULL, NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, binpath, "player.html", zipdir, "index.html", NULL, true, false, ZIP_PLATFORM_UNIX) &&
        add_files_to_dist(zipname, ".", conf->pakfile, zipdir, "data.pak", NULL, false, false, ZIP_PLATFORM_UNIX) &&
        true;
    if (ok) printf("Generated %s\n", zipname);
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

bool am_build_exports(am_export_flags *flags) {
    if (!file_exists("main.lua")) {
        fprintf(stderr, "Error: could not find main.lua in directory %s\n", am_opt_data_dir);
        return false;
    }
    am_make_dir(AM_TMP_DIR);
    if (!am_load_config()) return false;
    export_config conf;
    conf.basepath = get_base_path();
    conf.title = am_conf_app_title;
    conf.shortname = am_conf_app_shortname;
    if (flags->zipdir) {
        conf.zipdir = am_conf_app_shortname;
    } else {
        conf.zipdir = NULL;
    }
    conf.version = am_conf_app_version;
    conf.display_name = am_conf_app_display_name;
    conf.dev_region = am_conf_app_dev_region;
    conf.supported_languages = am_conf_app_supported_languages;
    conf.orientation = am_conf_app_display_orientation;
    conf.launch_image = am_conf_app_launch_image;
    conf.launch_image_id = (unsigned int)time(NULL);
    conf.grade = flags->debug ? "debug" : "release";
    conf.pakfile = AM_TMP_DIR AM_PATH_SEP_STR "data.pak";
    conf.mac_category = am_conf_mac_category;
    conf.recurse = flags->recurse;
    conf.allfiles = flags->allfiles;
    if (flags->outdir != NULL) {
        if (flags->outdir[strlen(flags->outdir) - 1] == AM_PATH_SEP) {
            conf.outdir = am_format("%s", flags->outdir);
        } else {
            conf.outdir = am_format("%s%s", flags->outdir, AM_PATH_SEP_STR);
        }
    } else {
        conf.outdir = am_format("%s", "");
    }
    conf.outpath = flags->outpath;
    if (!build_data_pak(&conf)) return false;
    bool ok =
        ((!(flags->export_windows))             || gen_windows_export(&conf, "windows", "msvc32")) &&
        ((!(flags->export_windows64))           || gen_windows_export(&conf, "windows64", "msvc64")) &&
        ((!(flags->export_mac))                 || gen_mac_export(&conf, true)) &&
        ((!(flags->export_mac_app_store))       || gen_mac_app_store_export(&conf)) &&
        ((!(flags->export_ios_xcode_proj))      || gen_ios_xcode_proj(&conf)) &&
        ((!(flags->export_android_studio_proj)) || gen_android_studio_proj(&conf)) &&
        ((!(flags->export_linux))               || gen_linux_export(&conf)) &&
        ((!(flags->export_html))                || gen_html_export(&conf)) &&
        ((!(flags->export_data_pak))            || gen_data_pak_export(&conf)) &&
        true;
    am_delete_file(conf.pakfile);
    am_delete_empty_dir(AM_TMP_DIR);
    free(conf.basepath);
    free(conf.outdir);
    return ok;
}

static bool create_mac_info_plist(const char *binpath, const char *filename, export_config *conf) {
    FILE *f = am_fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s\n", filename);
        return false;
    }
    char *template_filename = am_format("%s%c%s", binpath, AM_PATH_SEP, "Info.plist.tmpl");
    char *template_fmt = (char*)am_read_file(template_filename, NULL);
    free(template_filename);
    if (template_fmt == NULL) return false;

    fprintf(f, template_fmt,
        conf->title, // CFBundleName
        conf->display_name, // CFBundleDisplayName
        conf->dev_region, // CFBundleDevelopmentRegion
        conf->version, // CFBundleShortVersionString
        conf->version, // CFBundleVersion
        am_conf_app_id_mac, // CFBundleIdentifier
        conf->mac_category, // LSApplicationCategoryType
        am_conf_copyright_message // NSHumanReadableCopyright
    );
    free(template_fmt);
    fclose(f);

    return true;
}

static bool create_mac_lproj_dirs(const char *zipname, const char *dir, export_config *conf) {
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

static bool create_ios_xcode_lproj_dirs(const char *dir, export_config *conf) {
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
        char *ldir = am_format("%s/%s.lproj", dir, lang);
        char *lstrings = am_format("%s/LaunchScreen.strings", ldir);
        am_make_dir(ldir);
        if (!am_write_text_file(lstrings, (char*)" ")) return false;
        free(ldir);
        free(lstrings);
        while (*ptr == ',' || *ptr == ' ') ptr++;
    }
    return true;
}

static char* get_ios_xcodeproj_lang_list(export_config *conf) {
    char *res = am_format("%s", "");
    const char *ptr = conf->supported_languages;
    char lang[100];
    while (*ptr == ' ') ptr++;
    while (*ptr != 0) {
        int i = 0;
        while (*ptr != 0 && *ptr != ',') {
            if (i >= 100) {
                fprintf(stderr, "conf.lua: language id too long\n");
                return res;
            }
            lang[i] = *ptr;
            i++;
            ptr++;
        }
        lang[i] = 0;
        char *res2 = am_format("%s   \"%s\",\n", res, lang);
        free(res);
        res = res2;
        while (*ptr == ',' || *ptr == ' ') ptr++;
    }
    return res;
}

static char *get_android_lang_code(char *lang) {
    if (strcmp(lang, "zh-Hant") == 0) return am_format("%s", "zh-rTW");
    if (strcmp(lang, "zh-Hans") == 0) return am_format("%s", "zh-rCN");
    char *dash = strchr(lang, '-');
    if (dash == NULL) return am_format("%s", lang);
    char *part1 = lang;
    char *part2 = dash + 1;
    *dash = 0;
    char *code = am_format("%s-r%s", part1, part2);
    *dash = '-';
    return code;
}

static bool create_android_proj_lang_dirs(const char *dir, export_config *conf) {
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
        char *android_lang = get_android_lang_code(lang);
        char *ldir = am_format("%s/values-%s", dir, android_lang);
        am_make_dir(ldir);
        free(ldir);
        free(android_lang);
        while (*ptr == ',' || *ptr == ' ') ptr++;
    }
    return true;
}

static char* get_ios_launchscreen_entries(export_config *conf) {
    char *res = am_format("%s", "");
    const char *ptr = conf->supported_languages;
    char lang[100];
    while (*ptr == ' ') ptr++;
    int id = 0;
    while (*ptr != 0) {
        int i = 0;
        while (*ptr != 0 && *ptr != ',') {
            if (i >= 100) {
                fprintf(stderr, "conf.lua: language id too long\n");
                return res;
            }
            lang[i] = *ptr;
            i++;
            ptr++;
        }
        lang[i] = 0;
        char *res2 = am_format("%s\t\t3D8B60AC2024%04X0040055A /* %s */ = {isa = PBXFileReference; lastKnownFileType = text.plist.strings; name = \"%s\"; path = \"%s.lproj/LaunchScreen.strings\"; sourceTree = \"<group>\"; };\n", res, id, lang, lang, lang);
        free(res);
        res = res2;
        id++;
        while (*ptr == ',' || *ptr == ' ') ptr++;
    }
    return res;
}

static char* get_ios_launchscreen_children(export_config *conf) {
    char *res = am_format("%s", "");
    const char *ptr = conf->supported_languages;
    char lang[100];
    while (*ptr == ' ') ptr++;
    int id = 0;
    while (*ptr != 0) {
        int i = 0;
        while (*ptr != 0 && *ptr != ',') {
            if (i >= 100) {
                fprintf(stderr, "conf.lua: language id too long\n");
                return res;
            }
            lang[i] = *ptr;
            i++;
            ptr++;
        }
        lang[i] = 0;
        char *res2 = am_format("%s\t\t\t\t3D8B60AC2024%04X0040055A /* %s */,\n", res, id, lang);
        free(res);
        res = res2;
        id++;
        while (*ptr == ',' || *ptr == ' ') ptr++;
    }
    return res;
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
    FILE *f = am_fopen(path, "wb");
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

static bool create_ios_xcode_icon_files(const char *dir, export_config *conf) {
    size_t len;
    stbi_uc *img_data;
    int width, height;
    char *filename = (char*)am_conf_app_icon_ios;
    if (filename == NULL) return false;
    void *data = am_read_file(filename, &len);
    int components = 4;
    stbi_set_flip_vertically_on_load(0);
    img_data =
        stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
    free(data);
    if (img_data == NULL) return false;
    bool error = (!resize_image(img_data, width, height, dir, "iTunesArtwork.png", 1024, 1024)
        || !resize_image(img_data, width, height, dir, "icon120.png", 120, 120)
        || !resize_image(img_data, width, height, dir, "icon152.png", 152, 152)
        || !resize_image(img_data, width, height, dir, "icon167.png", 167, 167)
        || !resize_image(img_data, width, height, dir, "icon180.png", 180, 180)
        || !resize_image(img_data, width, height, dir, "icon20.png", 20, 20)
        || !resize_image(img_data, width, height, dir, "icon29.png", 29, 29)
        || !resize_image(img_data, width, height, dir, "icon40.png", 40, 40)
        || !resize_image(img_data, width, height, dir, "icon58.png", 58, 58)
        || !resize_image(img_data, width, height, dir, "icon60.png", 60, 60)
        || !resize_image(img_data, width, height, dir, "icon76.png", 76, 76)
        || !resize_image(img_data, width, height, dir, "icon80.png", 80, 80)
        || !resize_image(img_data, width, height, dir, "icon87.png", 87, 87)
        );
    free(img_data);
    return !error;
}

static bool create_android_proj_icon_files(const char *dir, export_config *conf) {
    size_t len;
    stbi_uc *img_data;
    int width, height;
    int width_fg, height_fg;
    int width_bg, height_bg;
    char *filename = (char*)am_conf_app_icon_android;
    if (filename == NULL) return false;
    void *data = am_read_file(filename, &len);
    if (data == NULL) return false;
    int components = 4;
    stbi_set_flip_vertically_on_load(0);
    img_data =
        stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
    free(data);
    if (img_data == NULL) return false;

    stbi_uc *fg_img_data = NULL;
    if (am_conf_android_adaptive_icon_fg != NULL) {
        data = am_read_file(am_conf_android_adaptive_icon_fg, &len);
        if (data == NULL) return false;
        int components = 4;
        stbi_set_flip_vertically_on_load(0);
        fg_img_data =
            stbi_load_from_memory((stbi_uc const *)data, len, &width_fg, &height_fg, &components, 4);
        free(data);
        if (fg_img_data == NULL) return false; // memory leak, but it doesn't matter
    }
    stbi_uc *bg_img_data = NULL;
    if (am_conf_android_adaptive_icon_bg != NULL) {
        data = am_read_file(am_conf_android_adaptive_icon_bg, &len);
        if (data == NULL) return false;
        int components = 4;
        stbi_set_flip_vertically_on_load(0);
        bg_img_data =
            stbi_load_from_memory((stbi_uc const *)data, len, &width_bg, &height_bg, &components, 4);
        free(data);
        if (bg_img_data == NULL) return false;  // memory leak, but it doesn't matter
    }

    char *templates_dir = get_template_path(conf);
    if (templates_dir == NULL) return false;
    char *mdpi_dir = am_format("%s/mipmap-mdpi", dir);
    char *hdpi_dir = am_format("%s/mipmap-hdpi", dir);
    char *xhdpi_dir = am_format("%s/mipmap-xhdpi", dir);
    char *xxhdpi_dir = am_format("%s/mipmap-xxhdpi", dir);
    char *xxxhdpi_dir = am_format("%s/mipmap-xxxhdpi", dir);
    char *anydpi_dir = am_format("%s/mipmap-anydpi-v26", dir);
    char *ic_launcher_xml_src = am_format("%sandroid/app/src/main/res/mipmap-anydpi-v26/ic_launcher.xml", templates_dir);
    char *ic_launcher_xml_dst = am_format("%s/mipmap-anydpi-v26/ic_launcher.xml", dir);
    am_make_dir(mdpi_dir);
    am_make_dir(hdpi_dir);
    am_make_dir(xhdpi_dir);
    am_make_dir(xxhdpi_dir);
    am_make_dir(xxxhdpi_dir);
    am_make_dir(anydpi_dir);

    bool error = (
           !resize_image(img_data, width, height, mdpi_dir, "ic_launcher.png", 48, 48)
        || !resize_image(img_data, width, height, hdpi_dir, "ic_launcher.png", 72, 72)
        || !resize_image(img_data, width, height, xhdpi_dir, "ic_launcher.png", 96, 96)
        || !resize_image(img_data, width, height, xxhdpi_dir, "ic_launcher.png", 144, 144)
        || !resize_image(img_data, width, height, xxxhdpi_dir, "ic_launcher.png", 192, 192)
        );

    if (!error && fg_img_data != NULL && bg_img_data != NULL) {
        error = (
           !resize_image(fg_img_data, width_fg, height_fg, mdpi_dir, "ic_launcher_foreground.png", 108, 108)
        || !resize_image(fg_img_data, width_fg, height_fg, hdpi_dir, "ic_launcher_foreground.png", 162, 162)
        || !resize_image(fg_img_data, width_fg, height_fg, xhdpi_dir, "ic_launcher_foreground.png", 216, 216)
        || !resize_image(fg_img_data, width_fg, height_fg, xxhdpi_dir, "ic_launcher_foreground.png", 324, 324)
        || !resize_image(fg_img_data, width_fg, height_fg, xxxhdpi_dir, "ic_launcher_foreground.png", 432, 432)
        || !resize_image(bg_img_data, width_bg, height_bg, mdpi_dir, "ic_launcher_background.png", 108, 108)
        || !resize_image(bg_img_data, width_bg, height_bg, hdpi_dir, "ic_launcher_background.png", 162, 162)
        || !resize_image(bg_img_data, width_bg, height_bg, xhdpi_dir, "ic_launcher_background.png", 216, 216)
        || !resize_image(bg_img_data, width_bg, height_bg, xxhdpi_dir, "ic_launcher_background.png", 324, 324)
        || !resize_image(bg_img_data, width_bg, height_bg, xxxhdpi_dir, "ic_launcher_background.png", 432, 432)
        || !copy_text_file(ic_launcher_xml_src, ic_launcher_xml_dst, NULL)
        );
    }

    free(img_data);
    if (fg_img_data != NULL) free(fg_img_data);
    if (bg_img_data != NULL) free(bg_img_data);
    free(templates_dir);
    free(mdpi_dir);
    free(hdpi_dir);
    free(xhdpi_dir);
    free(xxhdpi_dir);
    free(xxxhdpi_dir);
    free(anydpi_dir);
    free(ic_launcher_xml_src);
    free(ic_launcher_xml_dst);

    return !error;
}

static bool create_mac_entitlements(export_config *conf) {
    char *filename = am_format("%s/%s.entitlements", AM_TMP_DIR, conf->shortname);
    FILE *f = am_fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s\n", filename);
        free(filename);
        return false;
    }
    free(filename);
    fprintf(f,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
"<plist version=\"1.0\">"
"<dict>"
"<key>com.apple.security.app-sandbox</key> <true/>"
"</dict>"
"</plist>");
    fclose(f);

    return true;
}

static bool create_mac_icons(export_config *conf) {
    if (am_conf_app_icon_mac == NULL) return false;
    if (!am_file_exists("/usr/bin/iconutil")) return false;

    size_t len;
    stbi_uc *img_data;
    int width, height;
    void *data = am_read_file(am_conf_app_icon_mac, &len);
    int components = 4;
    stbi_set_flip_vertically_on_load(0);
    img_data =
        stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
    free(data);
    if (img_data == NULL) return false;

    char *iconset_dir = am_format("%s%c%s", AM_TMP_DIR, AM_PATH_SEP, "icon.iconset");
    am_make_dir(iconset_dir);

    if (!resize_image(img_data, width, height, iconset_dir,    "icon_16x16.png", 16, 16)
        || !resize_image(img_data, width, height, iconset_dir, "icon_16x16@2x.png", 32, 32)
        || !resize_image(img_data, width, height, iconset_dir, "icon_32x32.png", 32, 32)
        || !resize_image(img_data, width, height, iconset_dir, "icon_32x32@2x.png", 64, 64)
        || !resize_image(img_data, width, height, iconset_dir, "icon_64x64.png", 64, 64)
        || !resize_image(img_data, width, height, iconset_dir, "icon_64x64@2x.png", 128, 128)
        || !resize_image(img_data, width, height, iconset_dir, "icon_128x128.png", 128, 128)
        || !resize_image(img_data, width, height, iconset_dir, "icon_128x128@2x.png", 256, 256)
        || !resize_image(img_data, width, height, iconset_dir, "icon_256x256.png", 256, 256)
        || !resize_image(img_data, width, height, iconset_dir, "icon_256x256@2x.png", 512, 512)
        || !resize_image(img_data, width, height, iconset_dir, "icon_512x512.png", 512, 512)
        || !resize_image(img_data, width, height, iconset_dir, "icon_512x512@2x.png", 1024, 1024)
    ) {
	free(img_data);
        free(iconset_dir);
	return false;
    }
    free(img_data);

    if (!am_execute_shell_cmd("/usr/bin/iconutil -c icns %s", iconset_dir)) {
        free(iconset_dir);
        return false;
    }

    free(iconset_dir);
    return true;
}

#endif
