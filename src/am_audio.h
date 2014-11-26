#define AM_MAX_CHANNELS 2

struct am_audio_node;

struct am_audio_context {
    int sample_rate;
    int sync_id;
    int render_id;
    std::vector<am_audio_node*> roots;
};

// each channel is a contiguous chunk of memory
// (i.e. not interleaved)
struct am_audio_bus {
    int num_channels;
    int num_samples; // per channel
    float *channel_data[AM_MAX_CHANNELS]; // these point ...
    float *buffer;                        // ... into this
};

struct am_audio_node_child {
    int ref;
    am_audio_node *child;
};

template<typename T>
struct am_audio_param {
    T pending_value;
    T target_value;
    T current_value;

    am_audio_param(T val) {
        pending_value = val;
        target_value = val;
        current_value = val;
    }

    void update_target() {
        target_value = pending_value;
    }

    void update_current() {
        current_value = target_value;
    }

    void set_immediate(T val) {
        pending_value = val;
        target_value = val;
        current_value = val;
    }
};

struct am_audio_node : am_nonatomic_userdata {
    am_lua_array<am_audio_node_child> pending_children;
    am_lua_array<am_audio_node_child> live_children;
    int last_sync;
    int last_render;
    uint32_t flags;
    int recursion_limit;

    am_audio_node();

    void sync(lua_State *L, am_audio_context *context, int num_samples);
    void render_children(am_audio_context *context, am_audio_bus *bus);

    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
};

struct am_gain_node : am_audio_node {
    am_audio_param<float> gain;

    am_gain_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
};

struct am_queued_buffer {
    am_buffer *buffer;
    int ref;
};

struct am_audio_track_node : am_audio_node {
    am_lua_array<am_queued_buffer> buffer_queue;
    int fill_func_ref;
    int num_channels;
    int sample_rate;
    float playback_speed;
    bool loop;
    int position;

    am_audio_track_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
}

enum am_waveform {
    AM_WAVEFORM_SINE,
    AM_WAVEFORM_SQUARE,
    AM_WAVEFORM_SAWTOOTH,
    AM_WAVEFORM_TRIANGLE,
};

struct am_oscillator_node : am_audio_node {
    am_audio_param<float> phase;
    am_audio_param<float> freq;
    am_audio_param<am_waveform> waveform;
    int offset;

    am_oscillator_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
};

void am_open_audio_module(lua_State *L);

void am_init_audio(int sample_rate);
void am_fill_audio_bus(am_audio_bus *bus);
void am_sync_audio_graph(lua_State *L);
