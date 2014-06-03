#include "amulet.h"

am_attribute_array::am_attribute_array() {
    vbo = 0;
    usage = AM_BUFFER_USAGE_STATIC_DRAW;
    dirty_offset = 0;
    dirty_size = 0;
    size = 0;
    stride = 0;
    num_attributes = 0;
    attribute_infos = NULL;
    data = NULL;
}

void am_attribute_array::bind_attribute(am_render_state *rstate, int index, int pos) {
    am_set_attribute_array_enabled(index, true);
    am_attribute_info *attr = &attribute_infos[pos];
    if (vbo == 0) {
        // first time using this vbo, initialize it
        vbo = am_create_buffer();
        am_bind_buffer(AM_ARRAY_BUFFER, vbo);
        am_set_buffer_data(AM_ARRAY_BUFFER, size * stride, data, usage);
        dirty_size = 0;
    } else {
        am_bind_buffer(AM_ARRAY_BUFFER, vbo);
    }
    if (dirty_size > 0) {
        // vbo data changed since last use
        int byte_offset = attr->offset + dirty_offset * stride;
        am_set_buffer_sub_data(AM_ARRAY_BUFFER, byte_offset, dirty_size * stride, data + byte_offset);
        dirty_size = 0;
    }
    am_set_attribute_pointer(index, attr->size, attr->type, attr->normalized, stride, attr->offset);
}

void am_attribute_array::destroy() {
    if (vbo) {
        am_delete_buffer(vbo);
        vbo = 0;
    }
    if (data != NULL) {
        free(data);
        data = NULL;
    }
    if (attribute_infos != NULL) {
        free(attribute_infos);
        attribute_infos = NULL;
    }
}
