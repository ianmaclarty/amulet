#define AM_UNUSED(x)   ((void)(x))

typedef struct {union {void* p; double d; long long l;} u;} am_align_struct;
#define AM_ALIGN_MASK (sizeof(am_align_struct) - 1)

#define am_align_size(sz) \
    while (sz & AM_ALIGN_MASK) { sz++; }

#define am_is_power_of_two(n) ((((n)-1) & (n)) == 0)

#ifdef AM_WINDOWS
#define AM_PATH_SEP '\\'
#define AM_PATH_SEP_STR "\\"
#else
#define AM_PATH_SEP '/'
#define AM_PATH_SEP_STR "/"
#endif

#define AM_PI 3.14159265358979323846
#define AM_2PI (2.0 * 3.14159265358979323846)

#define AM_CONCAT_(a, b) a##b
#define AM_CONCAT(a, b) AM_CONCAT_(a, b)
#define AM_CONCAT3_(a, b, c) a##b##c
#define AM_CONCAT3(a, b, c) AM_CONCAT3_(a, b, c)
#define AM_STR_(a) #a
#define AM_STR(a) AM_STR_(a)

#ifndef AM_MSVC
#define ct_assert(e) enum { AM_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#define ct_check_array_size(arr, sz) ct_assert(sizeof(arr) == sz * sizeof(arr[0]))
#else
#define ct_assert(e)
#define ct_check_array_size(arr, sz)
#endif

#ifndef AM_MSVC
#define AM_RESTRICT __restrict__
#else
#define AM_RESTRICT __restrict
#endif

template<typename T>
static inline T am_min(T x, T y) {
    return x < y ? x : y;
}

template<typename T>
static inline T am_max(T x, T y) {
    return x > y ? x : y;
}

template<typename T>
static inline T am_clamp(T x, T lo, T hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

// returned string should be freed with free()
char *am_format(const char *fmt, ...);

bool am_file_exists(const char *filename);

void am_replchr(char *str, char c0, char c);

void am_delete_file(const char *file);
void am_make_dir(const char* dir);
void am_delete_empty_dir(const char* dir);

void *am_read_file(const char *filename, size_t *len);

#ifdef AM_BUILD_TOOLS
void am_expand_args(int *argc_ptr, char ***argv_ptr);
void am_free_expanded_args(int argc, char **argv);
#endif
