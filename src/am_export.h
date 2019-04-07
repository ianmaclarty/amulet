#ifdef AM_EXPORT

struct am_export_flags {
    bool windows;
    bool mac;
    bool mac_app_store;
    bool linux;
    bool html;
    bool ios_xcode_proj;
    bool recurse;
    bool zipdir;

    am_export_flags() {
        windows = false;
        mac = false;
        mac_app_store = false;
        linux = false;
        html = false;
        ios_xcode_proj = false;
        recurse = false;
        zipdir = true;
    }
};

bool am_build_exports(am_export_flags *flags);

#endif
