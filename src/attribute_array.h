struct am_attribute_info {
    int                 offset;
    int                 size;
    am_attribute_client_type type;
    bool                normalized;
};

struct am_attribute_array {
    am_buffer_id        vbo;
    am_buffer_usage     usage;
    int                 dirty_offset;
    int                 dirty_size;
    int                 size;
    int                 stride;
    int                 num_attributes;
    am_attribute_info   *attribute_infos;
    char                *data;

    am_attribute_array();
    void bind_attribute(am_render_state *rstate, int index, int pos);
    void destroy();
};
