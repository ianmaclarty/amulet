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

static bool gen_windows_export(export_config *conf) {
    char *zipname = get_export_zip_name(conf, "windows");
    if (am_file_exists(zipname)) am_delete_file(zipname);
    char *binpath = get_bin_path(conf, "msvc32");
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
    if (!am_execute_shell_cmd("cd %s/%s && codesign -f --deep -s '%s' --entitlements ../%s.entitlements '%s.app'",
        AM_TMP_DIR, conf->shortname, am_conf_mac_application_cert_identity, conf->shortname, conf->title)
    ) {
        return false;
    }
    if (!am_execute_shell_cmd("cd %s/%s && productbuild --component '%s.app' /Applications --sign '%s' ../../%s-%s-mac-app-store.pkg",
        AM_TMP_DIR, conf->shortname, conf->title, am_conf_mac_installer_cert_identity, conf->shortname, conf->version)
    ) {
        return false;
    }
    printf("Generated %s-%s-mac-app-store.pkg\n", conf->shortname, conf->version);
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
    const char *subs[] = {
        "AM_APPNAME", conf->shortname,
        "AM_APPID", am_conf_app_id_ios,
        "AM_AUTHOR", am_conf_app_author,
        "AM_CERT_ID", am_conf_ios_cert_identity,
        "AM_DEV_PROV_PROFILE_NAME", am_conf_ios_dev_prov_profile_name,
        "AM_DIST_PROV_PROFILE_NAME", am_conf_ios_dist_prov_profile_name,
        "AM_LANG_LIST", lang_list,
        "AM_DEV_REGION", conf->dev_region,
        "AM_LAUNCHSCREEN_STORYBOARD_STRINGS_ENTRIES", launchscreen_entries,
        "AM_LAUNCHSCREEN_STORYBOARD_CHILDREN", launchscreen_children,
        "AM_LUAVM", luavm,
        "AM_LAUNCH_IMG", launch_img_name,
        NULL
    };
    bool ok = copy_text_file(src_path, dest_path, (char**)subs);
    free(template_dir);
    free(src_path);
    free(dest_path);
    free(lang_list);
    free(launchscreen_entries);
    free(launchscreen_children);
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
    if (am_conf_mac_installer_cert_identity == NULL) {
        fprintf(stderr, "Error: please set mac_installer_cert_identity in conf.lua\n");
        return false;
    }
    char *templates_dir = get_template_path(conf);
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
    char *entitlements_src_path = am_format("%sios/app.entitlements", templates_dir);
    char *entitlements_dest_path = am_format("%s/%s.entitlements", projbase_dir, conf->shortname);
    const char *lua_a_file = am_format("%s.a", platform_luavm("ios"));
    am_make_dir(projbase_dir);
    am_make_dir(xcodeproj_dir);
    am_make_dir(assets_dir);
    am_make_dir(appicon_assets_dir);
    bool ok =
        copy_ios_xcodeproj_file(conf, xcodeproj_dir) &&
        copy_ios_xcode_info_plist(conf, projbase_dir) &&
        create_ios_xcode_lproj_dirs(projbase_dir, conf) &&
        create_ios_xcode_icon_files(appicon_assets_dir, conf) &&
        copy_text_file(appicon_contents_src_path, appicon_contents_dest_path, NULL) &&
        copy_text_file(entitlements_src_path, entitlements_dest_path, NULL) &&
        copy_ios_launchscreen_storyboard(projbase_dir, conf) &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "amulet.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "glslopt.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "kissfft.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, lua_a_file) &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "stb.a") &&
        copy_ios_xcode_bin_file(projbase_dir, conf, "tinymt.a") &&
        copy_ios_xcode_template_file(projbase_dir, conf, "Dummy.m") &&
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
    free(entitlements_src_path);
    free(entitlements_dest_path);
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
    copy_bin_file(conf->pakfile, am_format("%sdata.pak", conf->outdir));
    return true;
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
    conf.grade = "release";
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
        ((!(flags->export_windows))        || gen_windows_export(&conf)) &&
        ((!(flags->export_mac))            || gen_mac_export(&conf, true)) &&
        ((!(flags->export_mac_app_store))  || gen_mac_app_store_export(&conf)) &&
        ((!(flags->export_ios_xcode_proj)) || gen_ios_xcode_proj(&conf)) &&
        ((!(flags->export_linux))          || gen_linux_export(&conf)) &&
        ((!(flags->export_html))           || gen_html_export(&conf)) &&
        ((!(flags->export_data_pak))       || gen_data_pak_export(&conf)) &&
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
        fprintf(stderr, "Error: unable to create file %s", filename);
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
    if (filename == NULL) filename = get_default_ios_launch_image_filename(conf);
    if (filename == NULL) return false;
    void *data = am_read_file(am_conf_app_icon_ios, &len);
    int components = 4;
    stbi_set_flip_vertically_on_load(0);
    img_data =
        stbi_load_from_memory((stbi_uc const *)data, len, &width, &height, &components, 4);
    free(data);
    if (img_data == NULL) return false;
    if (!resize_image(img_data, width, height, dir, "iTunesArtwork.png", 1024, 1024)
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
        ) return false;
    free(img_data);
    return true;
}

static bool create_mac_entitlements(export_config *conf) {
    char *filename = am_format("%s/%s.entitlements", AM_TMP_DIR, conf->shortname);
    FILE *f = am_fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Error: unable to create file %s", filename);
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
