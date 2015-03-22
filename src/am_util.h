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

#define AM_CONCAT_(a, b) a##b
#define AM_CONCAT(a, b) AM_CONCAT_(a, b)
#define AM_CONCAT3_(a, b, c) a##b##c
#define AM_CONCAT3(a, b, c) AM_CONCAT3_(a, b, c)
#define AM_STR_(a) #a
#define AM_STR(a) AM_STR_(a)

#ifndef AM_WIN32 
#define ct_assert(e) enum { AM_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#define ct_check_array_size(arr, sz) ct_assert(sizeof(arr) == sz * sizeof(arr[0]))
#else
#define ct_assert(e)
#define ct_check_array_size(arr, sz)
#endif

// returned string should be freed with free()
char *am_format(const char *fmt, ...);
