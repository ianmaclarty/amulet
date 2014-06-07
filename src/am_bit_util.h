#define AM_ALIGN_MASK (sizeof(void*) - 1)

#define am_pointer_align_size(sz) \
    while (sz & AM_ALIGN_MASK) { sz++; }

