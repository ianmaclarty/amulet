#ifdef AM_EXPORT

struct am_export_flags {
    bool export_windows;
    bool export_windows64;
    bool export_mac;
    bool export_mac_app_store;
    bool export_linux;
    bool export_html;
    bool export_ios_xcode_proj;
    bool export_android_studio_proj;
    bool export_data_pak;
    bool debug;
    bool recurse;
    bool allfiles;;
    bool zipdir;
    const char *outdir;
    const char *outpath;

    am_export_flags() {
        export_windows = false;
        export_windows64 = false;
        export_mac = false;
        export_mac_app_store = false;
        export_linux = false;
        export_html = false;
        export_ios_xcode_proj = false;
        export_android_studio_proj = false;
        export_data_pak = false;
        debug = false;
        recurse = false;
        allfiles = false;
        zipdir = true;
        outdir = NULL;
        outpath = NULL;
    }

    int num_platforms() {
        return (export_windows ? 1 : 0)
             + (export_windows64 ? 1 : 0)
             + (export_mac ? 1 : 0)
             + (export_mac_app_store ? 1 : 0)
             + (export_linux ? 1 : 0)
             + (export_html ? 1 : 0)
             + (export_ios_xcode_proj ? 1 : 0)
             + (export_android_studio_proj ? 1 : 0)
             + (export_data_pak ? 1 : 0);
    }
};

bool am_build_exports(am_export_flags *flags);

#endif
