#ifndef VIEW
#error VIEW undefined
#endif

#ifndef BODY
#error BODY undefined
#endif

{
    int stride = (VIEW)->stride;
    am_buffer *buf = (VIEW)->buffer;
    #ifndef START
    int start = 0;
    #else
    int start = START;
    #endif
    int start_offset = (VIEW)->offset + ((start) - 1) * stride;
    #ifdef SIZE
    int clamped_size = am_min((SIZE), VIEW->size - start + 1);
    #else
    int clamped_size = VIEW->size - start + 1;
    #endif
    uint8_t *ptr = buf->data + start_offset;
    uint8_t *end = ptr + stride * clamped_size;
    switch (VIEW->type) {
        case AM_VIEW_TYPE_FLOAT: {
            while (ptr < end) {
                BODY(float, (float*)ptr);
                ptr += stride;
            }
            break;
        }
        case AM_VIEW_TYPE_FLOAT2: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_FLOAT3: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_FLOAT4: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_UBYTE: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_BYTE: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_UBYTE_NORM: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_BYTE_NORM: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_USHORT: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_SHORT: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_USHORT_ELEM: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_USHORT_NORM: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_SHORT_NORM: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_UINT: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_INT: {
            // NYI
            break;
        }
        case AM_VIEW_TYPE_UINT_ELEM: {
            // NYI
            break;
        }
        case AM_NUM_VIEW_TYPES: {
            assert(false);
            break;
        }
    }
}

#undef VIEW
#undef BODY
#ifdef START
#undef START
#endif
#ifdef SIZE
#undef SIZE
#endif
