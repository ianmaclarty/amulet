#define AM_MAX_CHANNELS 2
#define AM_MIN_FFT_SIZE 64
#define AM_MAX_FFT_SIZE 2048
#define AM_MAX_FFT_BINS (AM_MAX_FFT_SIZE / 2 + 1)

struct am_audio_node;

struct am_audio_context {
    int sample_rate;
    int sync_id;
    int render_id;
    am_audio_node* root;
};

// each channel is a contiguous chunk of memory
// (i.e. not interleaved)
struct am_audio_bus {
    int num_channels;
    int num_samples; // per channel
    float *channel_data[AM_MAX_CHANNELS]; // these point ...
    float *buffer;                        // ... into this
    bool owns_buffer;

    am_audio_bus(int num_channels, int num_samples, float *buffer);
    am_audio_bus(am_audio_bus *bus);
    ~am_audio_bus();
};

enum am_audio_node_child_state {
    AM_AUDIO_NODE_CHILD_STATE_NEW,
    AM_AUDIO_NODE_CHILD_STATE_OLD,
    AM_AUDIO_NODE_CHILD_STATE_REMOVED, // marked for removal
    AM_AUDIO_NODE_CHILD_STATE_DONE, // can now remove
};

struct am_audio_node_child {
    int ref;
    am_audio_node *child;
    am_audio_node_child_state state;
    am_audio_node_child() {
        ref = LUA_NOREF;
        child = NULL;
        state = AM_AUDIO_NODE_CHILD_STATE_NEW;
    }
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

    inline T interpolate_linear(int sample) {
        if (sample < am_conf_audio_interpolate_samples) {
            return current_value + ((T)sample / (T)am_conf_audio_interpolate_samples) * (target_value - current_value);
        } else {
            return target_value;
        }
    }
};

struct am_audio_buffer : am_nonatomic_userdata {
    int num_channels;
    int sample_rate;
    am_buffer *buffer;
    int buffer_ref;
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
    virtual bool finished();
};

struct am_gain_node : am_audio_node {
    am_audio_param<float> gain;

    am_gain_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
};

struct am_biquad_filter_coeffs {
    double b0;
    double b1;
    double b2;
    double a1;
    double a2;
};

struct am_biquad_filter_state {
    double x1;
    double x2;
    double y1;
    double y2;
};

struct am_biquad_filter_node : am_audio_node {
    am_biquad_filter_state current_state[AM_MAX_CHANNELS];
    am_biquad_filter_state next_state[AM_MAX_CHANNELS];
    am_biquad_filter_coeffs coeffs;

    am_biquad_filter_node();

    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);

    void set_lowpass_params(double cutoff, double resonance);
    void set_highpass_params(double cutoff, double resonance);
    void set_normalized_coeffs(double b0, double b1, double b2, double a0, double a1, double a2);
};

struct am_lowpass_filter_node : am_biquad_filter_node {
    am_audio_param<float> cutoff;
    am_audio_param<float> resonance;
    
    am_lowpass_filter_node();
    virtual void sync_params();
};

struct am_highpass_filter_node : am_biquad_filter_node {
    am_audio_param<float> cutoff;
    am_audio_param<float> resonance;
    
    am_highpass_filter_node();
    virtual void sync_params();
};

struct am_audio_track_node : am_audio_node {
    am_audio_buffer *audio_buffer;
    int audio_buffer_ref;
    float sample_rate_ratio;
    am_audio_param<float> playback_speed;
    am_audio_param<float> gain;
    bool loop;
    bool needs_reset;
    bool done_server;
    bool done_client;

    double current_position;
    double next_position;
    double reset_position;

    am_audio_track_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
    virtual bool finished();
};

struct am_audio_stream_node : am_audio_node {
    am_buffer *buffer;
    int buffer_ref;
    void *handle;
    int num_channels;
    int sample_rate;
    float sample_rate_ratio;
    am_audio_param<float> playback_speed;
    bool loop;
    bool done_server;
    bool done_client;

    double current_position;
    double next_position;

    am_audio_stream_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
    virtual bool finished();
};

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
    virtual bool finished();
};

struct am_spectrum_node : am_audio_node {
    int fftsize;
    int num_bins;
    float bin_data[AM_MAX_FFT_BINS];
    kiss_fftr_cfg cfg;
    am_audio_param<float> smoothing;
    am_buffer_view *arr;
    int arr_ref;
    bool done;
    
    am_spectrum_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
};

struct am_capture_node : am_audio_node {
    am_capture_node();
    virtual void sync_params();
    virtual void render_audio(am_audio_context *context, am_audio_bus *bus);
    virtual void post_render(am_audio_context *context, int num_samples);
    virtual bool finished();
};

void am_open_audio_module(lua_State *L);

void am_destroy_audio();
void am_fill_audio_bus(am_audio_bus *bus);
void am_sync_audio_graph(lua_State *L);

void am_interleave_audio(float* AM_RESTRICT dest, float* AM_RESTRICT src,
    int num_channels, int num_samples, int sample_offset, int count);
void am_interleave_audio16(int16_t* AM_RESTRICT dest, float* AM_RESTRICT src,
    int num_channels, int num_samples, int sample_offset, int count);
void am_uninterleave_audio(float* AM_RESTRICT dest, float* AM_RESTRICT src,
    int num_channels, int num_samples);
