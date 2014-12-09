#define AM_UNUSED(x)   ((void)(x))

typedef struct {union {void* p; double d; long long l;} u;} am_align_struct;
#define AM_ALIGN_MASK (sizeof(am_align_struct) - 1)

#define am_align_size(sz) \
    while (sz & AM_ALIGN_MASK) { sz++; }

#define am_is_power_of_two(n) ((((n)-1) & (n)) == 0)
