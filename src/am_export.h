#ifdef AM_EXPORT

struct am_export_flags {
    bool export_windows;
    bool export_mac;
    bool export_mac_app_store;
    bool export_linux;
    bool export_html;
    bool export_ios_xcode_proj;
    bool recurse;
    bool zipdir;

    am_export_flags() {
        export_windows = false;
        export_mac = false;
        export_mac_app_store = false;
        export_linux = false;
        export_html = false;
        export_ios_xcode_proj = false;
        recurse = false;
        zipdir = true;
    }
};

bool am_build_exports(am_export_flags *flags);

#endif
