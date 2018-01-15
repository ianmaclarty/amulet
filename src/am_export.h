#ifdef AM_EXPORT

#define AM_EXPORT_FLAG_WINDOWS     1
#define AM_EXPORT_FLAG_OSX         2
#define AM_EXPORT_FLAG_LINUX       4
#define AM_EXPORT_FLAG_HTML        8
#define AM_EXPORT_FLAG_IOS        16
#define AM_EXPORT_FLAG_IOSSIM     32
#define AM_EXPORT_FLAG_RECURSE    64

bool am_build_exports(uint32_t flags);

#endif
