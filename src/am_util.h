#define AM_UNUSED(x)   ((void)(x))

typedef struct {union {void* p; double d; long long l;} u;} am_align_struct;
#define AM_ALIGN_MASK (sizeof(am_align_struct) - 1)

#define am_align_size(sz) \
    while (sz & AM_ALIGN_MASK) { sz++; }

#define am_is_power_of_two(n) ((((n)-1) & (n)) == 0)

#ifdef AM_WIN32
#define AM_PATH_SEP '\\'
#else
#define AM_PATH_SEP '/'
#endif

#define am_min(x, y) ((x) < (y) ? (x) : (y))
#define am_max(x, y) ((x) > (y) ? (x) : (y))

#define AM_PI 3.14159265358979323846

// returned string should be freed with free()
char *am_format(const char *fmt, ...);
