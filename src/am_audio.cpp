#include "amulet.h"

#define AM_AUDIO_NODE_FLAG_MARK           ((uint32_t)1)
#define AM_AUDIO_NODE_FLAG_CHILDREN_DIRTY ((uint32_t)2)

#define node_marked(node)           (node->flags & AM_AUDIO_NODE_FLAG_MARK)
#define mark_node(node)             node->flags |= AM_AUDIO_NODE_FLAG_MARK
#define unmark_node(node)           node->flags &= ~AM_AUDIO_NODE_FLAG_MARK

#define children_dirty(node)        (node->flags & AM_AUDIO_NODE_FLAG_CHILDREN_DIRTY)
#define set_children_dirty(node)    node->flags |= AM_AUDIO_NODE_FLAG_CHILDREN_DIRTY
#define clear_children_dirty(node)  node->flags &= ~AM_AUDIO_NODE_FLAG_CHILDREN_DIRTY

static am_audio_context audio_context;

// Audio Bus

// all buffers in the pool have the same size
// current_pool_bufsize is the size of each buffer in the
// pool, in bytes.
static std::vector<void*> buffer_pool;
static int current_pool_bufsize = 0;
static unsigned int bufpool_top = 0;

static void clear_buffer_pool() {
    for (unsigned int i = 0; i < buffer_pool.size(); i++) {
        free(buffer_pool[i]);
    }
    buffer_pool.clear();
    bufpool_top = 0;
    current_pool_bufsize = 0;
}

static void* push_buffer(int size) {
    if (size != current_pool_bufsize) {
        // size of audio buffer has changed, clear pool
        clear_buffer_pool();
        current_pool_bufsize = size;
    }
    assert(bufpool_top <= buffer_pool.size());
    if (bufpool_top == buffer_pool.size()) {
        buffer_pool.push_back(malloc(size));
    }
    void *buf = buffer_pool[bufpool_top++];
    memset(buf, 0, size);
    return buf;
}

static void pop_buffer(void *buf) {
    bufpool_top--;
    assert(bufpool_top >= 0);
    assert(buffer_pool.size() > bufpool_top);
    assert(buffer_pool[bufpool_top] == buf);
}

static void setup_channels(am_audio_bus *bus) {
    for (int i = 0; i < bus->num_channels; i++) {
        bus->channel_data[i] = bus->buffer + i * bus->num_samples;
    }
}

am_audio_bus::am_audio_bus(int nchannels, int nsamples, float* buf) {
    num_channels = nchannels;
    num_samples = nsamples;
    buffer = buf;
    owns_buffer = false;
    setup_channels(this);
}

am_audio_bus::am_audio_bus(am_audio_bus *bus) {
    num_channels = bus->num_channels;
    num_samples = bus->num_samples;
    buffer = (float*)push_buffer(num_channels * num_samples * sizeof(float));
    owns_buffer = true;
    setup_channels(this);
}

am_audio_bus::~am_audio_bus() {
    if (owns_buffer) {
        pop_buffer(buffer);
    }
}

// Audio Param

inline static float linear_incf(am_audio_param<float> param, int samples) {
    return (param.target_value - param.current_value) / (float)samples;
}

// Audio Node

am_audio_node::am_audio_node() {
    pending_children.owner = this;
    live_children.owner = this;
    last_sync = 0;
    last_render = 0;
    flags = 0;
    recursion_limit = 0;
    root_ref = LUA_NOREF;
}

void am_audio_node::sync_params() {
}

void am_audio_node::post_render(am_audio_context *context, int num_samples) {
}

void am_audio_node::render_audio(am_audio_context *context, am_audio_bus *bus) {
    render_children(context, bus);
}

static void mix_bus(am_audio_bus * AM_RESTRICT dest, am_audio_bus * AM_RESTRICT src) {
    for (int c = 0; c < am_min(dest->num_channels, src->num_channels); c++) {
        int n = am_min(dest->num_samples, src->num_samples);
        float * AM_RESTRICT dest_data = dest->channel_data[c];
        float * AM_RESTRICT src_data = src->channel_data[c];
        for (int s = 0; s < n; s++) {
            dest_data[s] += src_data[s];
        }
    }
}

static void apply_fadein(am_audio_bus *bus) {
    int n = am_conf_audio_interpolate_samples;
    float inc = 1.0f / (float)n;
    for (int c = 0; c < bus->num_channels; c++) {
        float *data = bus->channel_data[c];
        float scale = 0.0f;
        for (int s = 0; s < n; s++) {
            data[s] *= scale;
            scale += inc;
        }
    }
}

static void apply_fadeout(am_audio_bus *bus) {
    int n = am_conf_audio_interpolate_samples;
    int start = bus->num_samples - n;
    int end = bus->num_samples;
    float inc = -1.0f / (float)n;
    for (int c = 0; c < bus->num_channels; c++) {
        float *data = bus->channel_data[c];
        float scale = 1.0f;
        for (int s = start; s < end; s++) {
            data[s] *= scale;
            scale += inc;
        }
    }
}

void am_audio_node::render_children(am_audio_context *context, am_audio_bus *bus) {
    if (recursion_limit < 0) return;
    recursion_limit--;
    for (int i = 0; i < live_children.size; i++) {
        am_audio_node_child *child = &live_children.arr[i];
        switch (child->state) {
            case AM_AUDIO_NODE_CHILD_STATE_NEW: {
                am_audio_bus tmp(bus);
                child->child->render_audio(context, &tmp);
                apply_fadein(&tmp);
                mix_bus(bus, &tmp);
                break;
            }
            case AM_AUDIO_NODE_CHILD_STATE_OLD: {
                child->child->render_audio(context, bus);
                break;
            }
            case AM_AUDIO_NODE_CHILD_STATE_REMOVED: {
                am_audio_bus tmp(bus);
                child->child->render_audio(context, &tmp);
                apply_fadeout(&tmp);
                mix_bus(bus, &tmp);
                break;
            }
            case AM_AUDIO_NODE_CHILD_STATE_DONE: {
                break;
            }
        }
    }
    recursion_limit++;
}

// Gain Node

am_gain_node::am_gain_node() : gain(0) {
    gain.pending_value = 1.0f;
}

void am_gain_node::sync_params() {
    gain.update_target();
}

void am_gain_node::post_render(am_audio_context *context, int num_samples) {
    gain.update_current();
}

void am_gain_node::render_audio(am_audio_context *context, am_audio_bus *bus) {
    am_audio_bus tmp(bus);
    render_children(context, &tmp);
    int num_channels = bus->num_channels;
    int num_samples = bus->num_samples;
    for (int s = 0; s < num_samples; s++) {
        for (int c = 0; c < num_channels; c++) {
            bus->channel_data[c][s] += tmp.channel_data[c][s] * gain.interpolate_linear(s);
        }
    }
}

// Biquad filters. Most code here adapted from http://www.chromium.org/blink

am_biquad_filter_node::am_biquad_filter_node() {
    for (int c = 0; c < AM_MAX_CHANNELS; c++) {
        memset(&current_state[c], 0, sizeof(am_biquad_filter_state));
        next_state[c] = current_state[c];
    }
}

void am_biquad_filter_node::post_render(am_audio_context *context, int num_samples) {
    for (int i = 0; i < AM_MAX_CHANNELS; i++) {
        current_state[i] = next_state[i];
    }
}

void am_biquad_filter_node::render_audio(am_audio_context *context, am_audio_bus *bus) {
    am_audio_bus tmp(bus);
    render_children(context, &tmp);
    int num_channels = bus->num_channels;
    int num_samples = bus->num_samples;
    for (int c = 0; c < num_channels; c++) {
        float *source = &tmp.channel_data[c][0];
        float *dest = &bus->channel_data[c][0];
        int n = num_samples;

        double x1 = current_state[c].x1;
        double x2 = current_state[c].x2;
        double y1 = current_state[c].y1;
        double y2 = current_state[c].y2;

        double b0 = coeffs.b0;
        double b1 = coeffs.b1;
        double b2 = coeffs.b2;
        double a1 = coeffs.a1;
        double a2 = coeffs.a2;

        while (n--) {
            float x = *source++;
            float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;

            *dest++ += y;

            x2 = x1;
            x1 = x;
            y2 = y1;
            y1 = y;
        }

        next_state[c].x1 = x1;
        next_state[c].x2 = x2;
        next_state[c].y1 = y1;
        next_state[c].y2 = y2;
    }
}

void am_biquad_filter_node::set_lowpass_params(double cutoff, double resonance) {
    // normalize cutoff 0->1
    double nyquist = (double)am_conf_audio_sample_rate * 0.5;
    cutoff = cutoff / nyquist;
    cutoff = am_max(0, am_min(1, cutoff));
    if (cutoff == 1) {
        // When cutoff is 1, the z-transform is 1.
        set_normalized_coeffs(1, 0, 0,
                              1, 0, 0);
    } else if (cutoff > 0) {
        // Compute biquad coefficients for lowpass filter
        resonance = am_max(0.0, resonance); // can't go negative
        double g = pow(10.0, 0.05 * resonance);
        double d = sqrt((4 - sqrt(16 - 16 / (g * g))) / 2);

        double theta = AM_PI * cutoff;
        double sn = 0.5 * d * sin(theta);
        double beta = 0.5 * (1 - sn) / (1 + sn);
        double gamma = (0.5 + beta) * cos(theta);
        double alpha = 0.25 * (0.5 + beta - gamma);

        double b0 = 2 * alpha;
        double b1 = 2 * 2 * alpha;
        double b2 = 2 * alpha;
        double a1 = 2 * -gamma;
        double a2 = 2 * beta;

        set_normalized_coeffs(b0, b1, b2, 1, a1, a2);
    } else {
        // When cutoff is zero, nothing gets through the filter, so set
        // coefficients up correctly.
        set_normalized_coeffs(0, 0, 0,
                              1, 0, 0);
    }
}

void am_biquad_filter_node::set_highpass_params(double cutoff, double resonance) {
    // normalize cutoff 0->1
    double nyquist = (double)am_conf_audio_sample_rate * 0.5;
    cutoff = cutoff / nyquist;
    cutoff = am_max(0, am_min(1, cutoff));
    if (cutoff == 1) {
        // The z-transform is 0.
        set_normalized_coeffs(0, 0, 0,
                              1, 0, 0);
    } else if (cutoff > 0) {
        // Compute biquad coefficients for highpass filter
        resonance = am_max(0.0, resonance); // can't go negative
        double g = pow(10.0, 0.05 * resonance);
        double d = sqrt((4 - sqrt(16 - 16 / (g * g))) / 2);

        double theta = AM_PI * cutoff;
        double sn = 0.5 * d * sin(theta);
        double beta = 0.5 * (1 - sn) / (1 + sn);
        double gamma = (0.5 + beta) * cos(theta);
        double alpha = 0.25 * (0.5 + beta + gamma);

        double b0 = 2 * alpha;
        double b1 = 2 * -2 * alpha;
        double b2 = 2 * alpha;
        double a1 = 2 * -gamma;
        double a2 = 2 * beta;

        set_normalized_coeffs(b0, b1, b2, 1, a1, a2);
    } else {
        // When cutoff is zero, we need to be careful because the above
        // gives a quadratic divided by the same quadratic, with poles
        // and zeros on the unit circle in the same place. When cutoff
        // is zero, the z-transform is 1.
        set_normalized_coeffs(1, 0, 0,
                              1, 0, 0);
    }
}

void am_biquad_filter_node::set_normalized_coeffs(double b0, double b1, double b2, double a0, double a1, double a2) {
    double a0_inv = 1.0 / a0;
    coeffs.b0 = b0 * a0_inv;
    coeffs.b1 = b1 * a0_inv;
    coeffs.b2 = b2 * a0_inv;
    coeffs.a1 = a1 * a0_inv;
    coeffs.a2 = a2 * a0_inv;
}

am_lowpass_filter_node::am_lowpass_filter_node() : cutoff(0), resonance(0) {
}

void am_lowpass_filter_node::sync_params() {
    cutoff.update_target();
    resonance.update_target();
    if (cutoff.current_value != cutoff.target_value
        || resonance.current_value != resonance.target_value)
    {
        cutoff.update_current();
        resonance.update_current();
        set_lowpass_params(cutoff.current_value, resonance.current_value);
    }
}

am_highpass_filter_node::am_highpass_filter_node() : cutoff(0), resonance(0) {
}

void am_highpass_filter_node::sync_params() {
    cutoff.update_target();
    resonance.update_target();
    if (cutoff.current_value != cutoff.target_value
        || resonance.current_value != resonance.target_value)
    {
        cutoff.update_current();
        resonance.update_current();
        set_highpass_params(cutoff.current_value, resonance.current_value);
    }
}

// Audio track node

am_audio_track_node::am_audio_track_node() 
    : playback_speed(1.0f)
{
    buffer = NULL;
    buffer_ref = LUA_NOREF;
    num_channels = 2;
    sample_rate = 44100;
    current_position = 0.0f;
    next_position = 0.0f;
    loop = false;
}

void am_audio_track_node::sync_params() {
    playback_speed.update_target();
}

static inline bool track_resample_required(am_audio_track_node *node) {
    return (node->sample_rate != am_conf_audio_sample_rate)
        || (node->playback_speed.current_value != node->playback_speed.target_value)
        || (fabs(node->playback_speed.current_value - 1.0f) > 0.00001f);
}

static inline bool is_paused(float playback_speed) {
    return playback_speed < 0.00001f;
}

void am_audio_track_node::render_audio(am_audio_context *context, am_audio_bus *bus) {
    if (is_paused(playback_speed.current_value)) return;
    int buf_num_channels = num_channels;
    int buf_num_samples = buffer->size / (buf_num_channels * sizeof(float));
    int bus_num_samples = bus->num_samples;
    int bus_num_channels = bus->num_channels;
    if (!loop && current_position >= buf_num_samples) {
        return;
    }
    if (!track_resample_required(this)) {
        // optimise common case where no resampling is required
        for (int c = 0; c < bus_num_channels; c++) {
            float *bus_data = bus->channel_data[c];
            float *buf_data = ((float*)buffer->data) + c * buf_num_samples;
            if (c < buf_num_channels) {
                int buf_pos = (int)current_position;
                assert(buf_pos < buf_num_samples);
                for (int bus_pos = 0; bus_pos < bus_num_samples; bus_pos++) {
                    bus_data[bus_pos] += buf_data[buf_pos++];
                    if (buf_pos >= buf_num_samples) {
                        if (loop) {
                            buf_pos = 0;
                        } else {
                            break;
                        }
                    }
                }
            } else {
                // less channels in buffer than bus, so duplicate previous channels
                assert(c > 0);
                memcpy(bus_data, bus->channel_data[c-1], bus_num_samples * sizeof(float));
            }
        }
        next_position = current_position + (float)bus_num_samples;
        if (loop && next_position >= (float)buf_num_samples) {
            next_position = fmodf(next_position, (float)buf_num_samples);
        }
    } else {
        // resample
        for (int c = 0; c < bus_num_channels; c++) {
            float *bus_data = bus->channel_data[c];
            float *buf_data = ((float*)buffer->data) + c * buf_num_samples;
            if (c < buf_num_channels) {
                float pos = current_position;
                for (int write_index = 0; write_index < bus_num_samples; write_index++) {
                    int read_index1 = (int)floorf(pos);
                    int read_index2 = read_index1 + 1;
                    if (read_index2 >= buf_num_samples) {
                        if (loop) {
                            read_index2 = 0;
                        } else {
                            break;
                        }
                    }
                    float interpolation_factor = (float)(pos - (float)read_index1);
                    float sample1 = buf_data[read_index1];
                    float sample2 = buf_data[read_index2];
                    float interpolated_sample = (1.0f - interpolation_factor) * sample1 + interpolation_factor * sample2;
                    bus_data[write_index] += interpolated_sample;
                    pos += playback_speed.interpolate_linear(write_index) * sample_rate_ratio;
                    if (pos >= (float)buf_num_samples) {
                        if (loop) {
                            pos = fmodf(pos, (float)buf_num_samples);
                        } else {
                            break;
                        }
                    }
                }
                next_position = pos;
            } else {
                // less channels in buffer than bus, so duplicate previous channels
                assert(c > 0);
                memcpy(bus_data, bus->channel_data[c-1], bus_num_samples * sizeof(float));
            }
        }
    }
}

void am_audio_track_node::post_render(am_audio_context *context, int num_samples) {
    playback_speed.update_current();
    current_position = next_position;
}

// Audio stream node

am_audio_stream_node::am_audio_stream_node()
    : playback_speed(1.0f)
{
    buffer = NULL;
    buffer_ref = LUA_NOREF;
    handle = NULL;
    num_channels = 2;
    sample_rate = 44100;
    sample_rate_ratio = 1.0f;
    loop = false;
    done = false;
    current_position = 0.0f;
    next_position = 0.0f;
}

void am_audio_stream_node::sync_params() {
    playback_speed.update_target();
}

void am_audio_stream_node::render_audio(am_audio_context *context, am_audio_bus *bus) {
    if (!loop && done) {
        return;
    }
    int bus_num_samples = bus->num_samples;
    int bus_num_channels = bus->num_channels;
    stb_vorbis *f = (stb_vorbis*)handle;
    int n = 0;
    float *channel_data[AM_MAX_CHANNELS];
    am_audio_bus tmp(bus);
    for (int i = 0; i < bus_num_channels; i++) {
        channel_data[i] = tmp.channel_data[i];
    }
    while (n < bus_num_samples && !done) {
        int m = stb_vorbis_get_samples_float(f, bus_num_channels, channel_data, bus_num_samples - n);
        if (m == 0) {
            if (loop) {
                stb_vorbis_seek_start(f);
            } else {
                done = true;
            }
        } else {
            n += m;
            for (int i = 0; i < bus_num_channels; i++) {
                channel_data[i] += m;
            }
        }
    }
    // fill in missing channels
    for (int c = num_channels; c < bus_num_channels; c++) {
        assert(c > 0);
        memcpy(tmp.channel_data[c], tmp.channel_data[c-1], bus_num_samples * sizeof(float));
    }
    mix_bus(bus, &tmp);
    /*
    next_position = current_position + (float)bus_num_samples;
    if (loop && next_position >= (float)buf_num_samples) {
        next_position = fmodf(next_position, (float)buf_num_samples);
    }
    */
}

void am_audio_stream_node::post_render(am_audio_context *context, int num_samples) {
    playback_speed.update_current();
    current_position = next_position;
}

// Oscillator Node

am_oscillator_node::am_oscillator_node()
    : phase(0)
    , freq(440)
    , waveform(AM_WAVEFORM_SINE)
{
    offset = 0;
}

void am_oscillator_node::sync_params() {
    phase.update_target();
    freq.update_target();
}

void am_oscillator_node::post_render(am_audio_context *context, int num_samples) {
    phase.update_current();
    freq.update_current();
    offset += num_samples;
}

void am_oscillator_node::render_audio(am_audio_context *context, am_audio_bus *bus) {
    double t = (double)offset / (double)context->sample_rate;
    double dt = 1.0/(double)context->sample_rate;
    int num_channels = bus->num_channels;
    int num_samples = bus->num_samples;
    float phase_inc = linear_incf(phase, num_samples);
    float freq_inc = linear_incf(freq, num_samples);
    float phase_val = phase.current_value;
    float freq_val = freq.current_value;
    switch (waveform.current_value) {
        case AM_WAVEFORM_SINE: {
            for (int i = 0; i < num_samples; i++) {
                float val = sinf(AM_PI*2.0f*freq_val*(float)t+phase_val);
                for (int c = 0; c < num_channels; c++) {
                    bus->channel_data[c][i] += val;
                }
                phase_val += phase_inc;
                freq_val += freq_inc;
                t += dt;
            }
        }
        default: {}
    }
}

//-------------------------------------------------------------------------
// Lua bindings.

static int search_uservalues(lua_State *L, am_audio_node *node) {
    if (node_marked(node)) return 0; // cycle
    node->pushuservalue(L); // push uservalue table of node
    lua_pushvalue(L, 2); // push field
    lua_rawget(L, -2); // lookup field in uservalue table
    if (!lua_isnil(L, -1)) {
        // found it
        lua_remove(L, -2); // remove uservalue table
        return 1;
    }
    lua_pop(L, 2); // pop nil, uservalue table
    if (node->pending_children.size != 1) return 0;
    mark_node(node);
    am_audio_node *child = node->pending_children.arr[0].child;
    child->push(L);
    lua_replace(L, 1); // child is now at index 1
    int r = search_uservalues(L, child);
    unmark_node(node);
    return r;
}

int am_audio_node_index(lua_State *L) {
    am_audio_node *node = (am_audio_node*)lua_touserdata(L, 1);
    am_default_index_func(L); // check metatable
    if (!lua_isnil(L, -1)) return 1;
    lua_pop(L, 1); // pop nil
    return search_uservalues(L, node);
}

static int add_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_audio_node *parent = am_get_userdata(L, am_audio_node, 1);
    am_audio_node *child = am_get_userdata(L, am_audio_node, 2);
    am_audio_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 2); // ref from parent to child
    child_slot.state = AM_AUDIO_NODE_CHILD_STATE_NEW; // just to avoid gcc warning.

    // keep list sorted (required for sync_children_list below)
    int n = parent->pending_children.size;
    for (int i = 0; i <= n; i++) {
        if (i == n || child < parent->pending_children.arr[i].child) {
            parent->pending_children.insert(L, i, child_slot);
            break;
        }
    }

    set_children_dirty(parent);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_audio_node *parent = am_get_userdata(L, am_audio_node, 1);
    am_audio_node *child = am_get_userdata(L, am_audio_node, 2);
    for (int i = 0; i < parent->pending_children.size; i++) {
        if (parent->pending_children.arr[i].child == child) {
            parent->unref(L, parent->pending_children.arr[i].ref);
            parent->pending_children.remove(i);
            set_children_dirty(parent);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_all_children(lua_State *L) {
    am_check_nargs(L, 1);
    am_audio_node *parent = am_get_userdata(L, am_audio_node, 1);
    for (int i = parent->pending_children.size-1; i >= 0; i--) {
        parent->unref(L, parent->pending_children.arr[i].ref);
    }
    parent->pending_children.clear();
    set_children_dirty(parent);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

void am_set_audio_node_child(lua_State *L, am_audio_node *parent) {
    if (lua_isnil(L, 1)) {
        return;
    }
    am_audio_node *child = am_get_userdata(L, am_audio_node, 1);
    am_audio_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 1); // ref from parent to child
    child_slot.state = AM_AUDIO_NODE_CHILD_STATE_NEW; // just to avoid gcc warning.
    parent->pending_children.push_back(L, child_slot);
    set_children_dirty(parent);
}

static int child_pair_next(lua_State *L) {
    am_check_nargs(L, 2);
    am_audio_node *node = am_get_userdata(L, am_audio_node, 1);
    int i = luaL_checkinteger(L, 2);
    if (i >= 0 && i < node->pending_children.size) {
        lua_pushinteger(L, i+1);
        node->pending_children.arr[i].child->push(L);
        return 2;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int child_pairs(lua_State *L) {
    lua_pushcclosure(L, child_pair_next, 0);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);
    return 3;
}

static int get_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_audio_node *node = am_get_userdata(L, am_audio_node, 1);
    int i = luaL_checkinteger(L, 2);
    if (i >= 1 && i <= node->pending_children.size) {
        node->pending_children.arr[i-1].child->push(L);
        return 1;
    } else {
        return 0;
    }
}

static inline void check_alias(lua_State *L) {
    am_default_index_func(L);
    if (!lua_isnil(L, -1)) goto error;
    lua_pop(L, 1);
    return;
    error:
    luaL_error(L,
        "alias '%s' is already used for something else",
        lua_tostring(L, 2));
}

static int alias(lua_State *L) {
    int nargs = am_check_nargs(L, 2);
    am_audio_node *node = am_get_userdata(L, am_audio_node, 1);
    node->pushuservalue(L);
    int userval_idx = am_absindex(L, -1);
    if (lua_istable(L, 2)) {
        // create multiple aliases - one for each key/value pair
        lua_pushvalue(L, 2); // push table, as we need position 2 for check_alias
        int tbl_idx = am_absindex(L, -1);
        lua_pushnil(L);
        while (lua_next(L, tbl_idx)) {
            lua_pushvalue(L, -2); // key
            lua_replace(L, 2); // check_alias expects key in position 2
            check_alias(L);
            lua_pushvalue(L, -2); // key
            lua_pushvalue(L, -2); // value
            lua_rawset(L, userval_idx); // uservalue[key] = value
            lua_pop(L, 1); // value
        }
        lua_pop(L, 1); // table
    } else if (lua_isstring(L, 2)) {
        check_alias(L);
        lua_pushvalue(L, 2);
        if (nargs > 2) {
            lua_pushvalue(L, 3);
        } else {
            lua_pushvalue(L, 1);
        }
        lua_rawset(L, userval_idx);
    } else {
        return luaL_error(L, "expecting a string or table at position 2");
    }
    lua_pop(L, 1); // uservalue
    lua_pushvalue(L, 1);
    return 1;
}

// Gain node lua bindings

static int create_gain_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_gain_node *node = am_new_userdata(L, am_gain_node);
    am_set_audio_node_child(L, node);
    node->gain.set_immediate(luaL_checknumber(L, 2));
    return 1;
}

static void get_gain(lua_State *L, void *obj) {
    am_gain_node *node = (am_gain_node*)obj;
    lua_pushnumber(L, node->gain.pending_value);
}

static void set_gain(lua_State *L, void *obj) {
    am_gain_node *node = (am_gain_node*)obj;
    node->gain.pending_value = luaL_checknumber(L, 3);
}

static am_property gain_property = {get_gain, set_gain};

static void register_gain_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);

    am_register_property(L, "value", &gain_property);

    am_register_metatable(L, "gain", MT_am_gain_node, MT_am_audio_node);
}

// Lowpass biquad filter node lua bindings

static int create_lowpass_filter_node(lua_State *L) {
    am_check_nargs(L, 3);
    am_lowpass_filter_node *node = am_new_userdata(L, am_lowpass_filter_node);
    am_set_audio_node_child(L, node);
    node->cutoff.set_immediate(luaL_checknumber(L, 2));
    node->resonance.set_immediate(luaL_checknumber(L, 3));
    node->set_lowpass_params(node->cutoff.current_value, node->resonance.current_value);
    return 1;
}

static void get_lowpass_cutoff(lua_State *L, void *obj) {
    am_lowpass_filter_node *node = (am_lowpass_filter_node*)obj;
    lua_pushnumber(L, node->cutoff.pending_value);
}

static void set_lowpass_cutoff(lua_State *L, void *obj) {
    am_lowpass_filter_node *node = (am_lowpass_filter_node*)obj;
    node->cutoff.pending_value = am_clamp(luaL_checknumber(L, 3), 1.0, 22050.0);
}

static void get_lowpass_resonance(lua_State *L, void *obj) {
    am_lowpass_filter_node *node = (am_lowpass_filter_node*)obj;
    lua_pushnumber(L, node->resonance.pending_value);
}

static void set_lowpass_resonance(lua_State *L, void *obj) {
    am_lowpass_filter_node *node = (am_lowpass_filter_node*)obj;
    node->resonance.pending_value = am_clamp(luaL_checknumber(L, 3), 0.0, 1000.0);
}

static am_property lowpass_cutoff_property = {get_lowpass_cutoff, set_lowpass_cutoff};
static am_property lowpass_resonance_property = {get_lowpass_resonance, set_lowpass_resonance};

static void register_lowpass_filter_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);

    am_register_property(L, "cutoff", &lowpass_cutoff_property);
    am_register_property(L, "resonance", &lowpass_resonance_property);

    am_register_metatable(L, "lowpass_filter", MT_am_lowpass_filter_node, MT_am_audio_node);
}

// Highpass biquad filter node lua bindings

static int create_highpass_filter_node(lua_State *L) {
    am_check_nargs(L, 3);
    am_highpass_filter_node *node = am_new_userdata(L, am_highpass_filter_node);
    am_set_audio_node_child(L, node);
    node->cutoff.set_immediate(luaL_checknumber(L, 2));
    node->resonance.set_immediate(luaL_checknumber(L, 3));
    node->set_highpass_params(node->cutoff.current_value, node->resonance.current_value);
    return 1;
}

static void get_highpass_cutoff(lua_State *L, void *obj) {
    am_highpass_filter_node *node = (am_highpass_filter_node*)obj;
    lua_pushnumber(L, node->cutoff.pending_value);
}

static void set_highpass_cutoff(lua_State *L, void *obj) {
    am_highpass_filter_node *node = (am_highpass_filter_node*)obj;
    node->cutoff.pending_value = am_clamp(luaL_checknumber(L, 3), 1.0, 22050.0);
}

static void get_highpass_resonance(lua_State *L, void *obj) {
    am_highpass_filter_node *node = (am_highpass_filter_node*)obj;
    lua_pushnumber(L, node->resonance.pending_value);
}

static void set_highpass_resonance(lua_State *L, void *obj) {
    am_highpass_filter_node *node = (am_highpass_filter_node*)obj;
    node->resonance.pending_value = am_clamp(luaL_checknumber(L, 3), 0.0, 1000.0);
}

static am_property highpass_cutoff_property = {get_highpass_cutoff, set_highpass_cutoff};
static am_property highpass_resonance_property = {get_highpass_resonance, set_highpass_resonance};

static void register_highpass_filter_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);

    am_register_property(L, "cutoff", &highpass_cutoff_property);
    am_register_property(L, "resonance", &highpass_resonance_property);

    am_register_metatable(L, "highpass_filter", MT_am_highpass_filter_node, MT_am_audio_node);
}

// Audio track node lua bindings

static int create_audio_track_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_audio_track_node *node = am_new_userdata(L, am_audio_track_node);
    node->buffer = am_get_userdata(L, am_buffer, 1);
    node->buffer_ref = node->ref(L, 1);
    if (nargs > 1) {
        node->loop = lua_toboolean(L, 2);
    }
    if (nargs > 2) {
        node->playback_speed.set_immediate(luaL_checknumber(L, 3));
    }
    if (nargs > 3) {
        node->num_channels = luaL_checkinteger(L, 4);
        if (node->num_channels < 1) {
            return luaL_error(L, "audio must have at least one channel");
        }
    }
    node->sample_rate_ratio = (float)node->sample_rate / (float)am_conf_audio_sample_rate;
    return 1;
}

static void get_track_playback_speed(lua_State *L, void *obj) {
    am_audio_track_node *node = (am_audio_track_node*)obj;
    lua_pushnumber(L, node->playback_speed.pending_value);
}

static void set_track_playback_speed(lua_State *L, void *obj) {
    am_audio_track_node *node = (am_audio_track_node*)obj;
    node->playback_speed.pending_value = luaL_checknumber(L, 3);
}

static am_property track_playback_speed_property = {get_track_playback_speed, set_track_playback_speed};

static void register_audio_track_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);

    am_register_property(L, "playback_speed", &track_playback_speed_property);

    am_register_metatable(L, "track", MT_am_audio_track_node, MT_am_audio_node);
}

// Audio stream node lua bindings

static int create_audio_stream_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_audio_stream_node *node = am_new_userdata(L, am_audio_stream_node);
    node->buffer = am_get_userdata(L, am_buffer, 1);
    node->buffer_ref = node->ref(L, 1);
    if (nargs > 1) {
        node->loop = lua_toboolean(L, 2);
    }
    if (nargs > 2) {
        node->playback_speed.set_immediate(luaL_checknumber(L, 3));
    }
    int err = 0;
    node->handle = stb_vorbis_open_memory(
        (const unsigned char *)node->buffer->data, node->buffer->size, &err, NULL);
    if (node->handle == NULL) {
        return luaL_error(L, "buffer '%s' is not valid ogg vorbis data", node->buffer->origin);
    }
    stb_vorbis_info info = stb_vorbis_get_info((stb_vorbis*)node->handle);
    node->sample_rate = info.sample_rate;
    if (node->sample_rate != am_conf_audio_sample_rate) {
        am_log0("WARNING: buffer '%s' has sample rate of %dHz, but will play at %dHz",
            node->buffer->origin, node->sample_rate, am_conf_audio_sample_rate);
    }
    node->num_channels = info.channels;
    node->sample_rate_ratio = (float)node->sample_rate / (float)am_conf_audio_sample_rate;
    return 1;
}

static int audio_stream_gc(lua_State *L) {
    am_audio_stream_node *node = (am_audio_stream_node*)lua_touserdata(L, 1);
    if (node->handle != NULL) {
        stb_vorbis_close((stb_vorbis*)node->handle);
        node->handle = NULL;
    }
    return 0;
}

static void get_stream_playback_speed(lua_State *L, void *obj) {
    am_audio_stream_node *node = (am_audio_stream_node*)obj;
    lua_pushnumber(L, node->playback_speed.pending_value);
}

static void set_stream_playback_speed(lua_State *L, void *obj) {
    am_audio_stream_node *node = (am_audio_stream_node*)obj;
    node->playback_speed.pending_value = luaL_checknumber(L, 3);
}

static am_property stream_playback_speed_property = {get_stream_playback_speed, set_stream_playback_speed};

static void register_audio_stream_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);
    lua_pushcclosure(L, audio_stream_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_property(L, "playback_speed", &stream_playback_speed_property);

    am_register_metatable(L, "audio_stream", MT_am_audio_stream_node, MT_am_audio_node);
}

// Oscillator node lua bindings

static int create_oscillator_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_oscillator_node *node = am_new_userdata(L, am_oscillator_node);
    node->freq.set_immediate(luaL_checknumber(L, 1));
    if (nargs > 1) {
        node->phase.set_immediate(luaL_checknumber(L, 2));
    }
    return 1;
}

static void get_phase(lua_State *L, void *obj) {
    am_oscillator_node *node = (am_oscillator_node*)obj;
    lua_pushnumber(L, node->phase.pending_value);
}

static void set_phase(lua_State *L, void *obj) {
    am_oscillator_node *node = (am_oscillator_node*)obj;
    node->phase.pending_value = luaL_checknumber(L, 3);
}

static am_property phase_property = {get_phase, set_phase};

static void get_freq(lua_State *L, void *obj) {
    am_oscillator_node *node = (am_oscillator_node*)obj;
    lua_pushnumber(L, node->freq.pending_value);
}

static void set_freq(lua_State *L, void *obj) {
    am_oscillator_node *node = (am_oscillator_node*)obj;
    node->freq.pending_value = luaL_checknumber(L, 3);
}

static am_property freq_property = {get_freq, set_freq};

static void register_oscillator_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);

    am_register_property(L, "phase", &phase_property);
    am_register_property(L, "freq", &freq_property);

    am_register_metatable(L, "oscillator", MT_am_oscillator_node, MT_am_audio_node);
}

// Audio node lua bindings

static int create_audio_node(lua_State *L) {
    am_new_userdata(L, am_audio_node);
    return 1;
}

static int get_root_audio_node(lua_State *L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_ROOT_AUDIO_NODE);
    return 1;
}

static void register_audio_node_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, am_audio_node_index, 0);
    lua_setfield(L, -2, "__index");
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, child_pairs, 0);
    lua_setfield(L, -2, "children");
    lua_pushcclosure(L, get_child, 0);
    lua_setfield(L, -2, "child");

    lua_pushcclosure(L, alias, 0);
    lua_setfield(L, -2, "alias");

    lua_pushcclosure(L, add_child, 0);
    lua_setfield(L, -2, "add");
    lua_pushcclosure(L, remove_child, 0);
    lua_setfield(L, -2, "remove");
    lua_pushcclosure(L, remove_all_children, 0);
    lua_setfield(L, -2, "remove_all");
    
    lua_pushcclosure(L, create_gain_node, 0);
    lua_setfield(L, -2, "gain");
    lua_pushcclosure(L, create_lowpass_filter_node, 0);
    lua_setfield(L, -2, "lowpass_filter");
    lua_pushcclosure(L, create_highpass_filter_node, 0);
    lua_setfield(L, -2, "highpass_filter");

    am_register_metatable(L, "audio_node", MT_am_audio_node, 0);
}

//-------------------------------------------------------------------------

static int decode_ogg(lua_State *L) {
    am_buffer *source_buf = am_get_userdata(L, am_buffer, 1);
    int num_channels;
    int sample_rate;
    short *tmp_data;
    int num_samples = stb_vorbis_decode_memory((unsigned char*)source_buf->data,
        source_buf->size, &num_channels, &sample_rate, &tmp_data);
    if (num_samples <= 0) {
        return luaL_error(L, "error decoding ogg '%s'", source_buf->origin);
    }
    am_buffer *dest_buf;
    float *dest_data;
    int copy_channels = num_channels > am_conf_audio_channels ? am_conf_audio_channels : num_channels;
    int dest_samples;
    if (sample_rate != am_conf_audio_sample_rate) {
        // resample required
        am_log0("WARNING: resampling buffer '%s' from %dHz to %dHz",
            source_buf->origin, sample_rate, am_conf_audio_sample_rate);
        double sample_rate_ratio = (double)sample_rate / (double)am_conf_audio_sample_rate;
        dest_samples = floor((double)num_samples / sample_rate_ratio);
        dest_buf = am_new_userdata(L, am_buffer, dest_samples * am_conf_audio_channels * 4);
        dest_data = (float*)dest_buf->data;
        for (int c = 0; c < copy_channels; c++) {
            double pos = 0.0f;
            for (int write_index = 0; write_index < dest_samples; write_index++) {
                int read_index1 = (int)floor(pos);
                int read_index2 = read_index1 + 1;
                if (read_index2 >= num_samples) {
                    break;
                }
                float interpolation_factor = (float)(pos - (float)read_index1);
                float sample1 = (float)tmp_data[read_index1 * num_channels + c] / (float)INT16_MAX;
                float sample2 = (float)tmp_data[read_index2 * num_channels + c] / (float)INT16_MAX;
                float interpolated_sample = (1.0f - interpolation_factor) * sample1 + interpolation_factor * sample2;
                dest_data[c * dest_samples + write_index] = interpolated_sample;
                pos += sample_rate_ratio;
                if (pos >= (double)num_samples) {
                    break;
                }
            }
        }
    } else {
        // no resample required
        dest_buf = am_new_userdata(L, am_buffer, num_samples * am_conf_audio_channels * 4);
        dest_data = (float*)dest_buf->data;
        dest_samples = num_samples;
        for (int c = 0; c < copy_channels; c++) {
            for (int s = 0; s < num_samples; s++) {
                dest_data[c * num_samples + s] = (float)tmp_data[s * num_channels + c] / (float)INT16_MAX;
            }
        }
    }
    free(tmp_data);
    // if less than required channels in decoded stream, then duplicate channels.
    if (copy_channels < am_conf_audio_channels) {
        for (int c = copy_channels; c < am_conf_audio_channels; c++) {
            memcpy(&dest_data[c * dest_samples], &dest_data[0], dest_samples * 4);
        }
    }
    return 1;
}

//-------------------------------------------------------------------------

void am_destroy_audio() {
    audio_context.root = NULL;
    clear_buffer_pool();
}

static void do_post_render(am_audio_context *context, int num_samples, am_audio_node *node) {
    if (node->last_render >= context->render_id) return; // already processed
    node->last_render = context->render_id;
    node->post_render(context, num_samples);
    for (int i = 0; i < node->live_children.size; i++) {
        am_audio_node_child *child = &node->live_children.arr[i];
        if (child->state == AM_AUDIO_NODE_CHILD_STATE_REMOVED) {
            child->state = AM_AUDIO_NODE_CHILD_STATE_DONE;
        } else if (child->state == AM_AUDIO_NODE_CHILD_STATE_NEW) {
            child->state = AM_AUDIO_NODE_CHILD_STATE_OLD;
        }
        do_post_render(context, num_samples, child->child);
    }
}

void am_fill_audio_bus(am_audio_bus *bus) {
    if (audio_context.root == NULL) return;
    audio_context.root->render_audio(&audio_context, bus);
    audio_context.render_id++;
    do_post_render(&audio_context, bus->num_samples, audio_context.root);
}

static void sync_children_list(lua_State *L, am_audio_node *node) {
    int p = 0;
    int l = 0;

    am_lua_array<am_audio_node_child> *parr = &node->pending_children;
    am_lua_array<am_audio_node_child> *larr = &node->live_children;

    // remove live children that need to be removed
    for (l = larr->size-1; l >= 0; l--) {
        if (larr->arr[l].state == AM_AUDIO_NODE_CHILD_STATE_DONE) {
            node->unref(L, larr->arr[l].ref);
            larr->remove(l);
        }
    }

    if (children_dirty(node)) {
        // insert NEW children and mark REMOVED children
        l = 0;
        p = 0;
        while (p < parr->size && l < larr->size) {
            while (p < parr->size && parr->arr[p].child < larr->arr[l].child) {
                parr->arr[p].state = AM_AUDIO_NODE_CHILD_STATE_NEW;
                larr->insert(L, l, parr->arr[p]);
                larr->arr[l].child->push(L);
                larr->arr[l].ref = node->ref(L, -1);
                lua_pop(L, 1);
                p++;
                l++;
            }
            while (p < parr->size && l < larr->size && parr->arr[p].child == larr->arr[l].child) {
                p++;
                l++;
            }
            while (p < parr->size && l < larr->size && parr->arr[p].child > larr->arr[l].child) {
                if (larr->arr[l].state != AM_AUDIO_NODE_CHILD_STATE_DONE) {
                    larr->arr[l].state = AM_AUDIO_NODE_CHILD_STATE_REMOVED;
                }
                l++;
            }
        }
        while (p < parr->size) {
            parr->arr[p].state = AM_AUDIO_NODE_CHILD_STATE_NEW;
            larr->insert(L, l, parr->arr[p]);
            larr->arr[l].child->push(L);
            larr->arr[l].ref = node->ref(L, -1);
            lua_pop(L, 1);
            p++;
            l++;
        }
        while (l < larr->size) {
            if (larr->arr[l].state != AM_AUDIO_NODE_CHILD_STATE_DONE) {
                larr->arr[l].state = AM_AUDIO_NODE_CHILD_STATE_REMOVED;
            }
            l++;
        }

        clear_children_dirty(node);
    }
}

static void sync_audio_graph(lua_State *L, am_audio_context *context, am_audio_node *node) {
    if (node->last_sync >= context->sync_id) return; // already synced
    node->last_sync = context->sync_id;
    node->sync_params();
    sync_children_list(L, node);
    for (int i = 0; i < node->live_children.size; i++) {
        sync_audio_graph(L, context, node->live_children.arr[i].child);
    }
}

void am_sync_audio_graph(lua_State *L) {
    if (audio_context.root == NULL) return;
    audio_context.sync_id++;
    sync_audio_graph(L, &audio_context, audio_context.root);
}

//-------------------------------------------------------------------------
// Backend utility functions

void am_interleave_audio(float* AM_RESTRICT dest, float* AM_RESTRICT src,
    int num_channels, int num_samples, int sample_offset, int count)
{
    int i, j;
    int k = sample_offset + count;
    assert(k <= num_samples);
    for (int c = 0; c < num_channels; c++) {
        i = k - count;
        j = c;
        while (i != k) {
            dest[j] = src[i];
            i++;
            j += num_channels;
        }
        k += num_samples;
    }
}

//-------------------------------------------------------------------------
// Module registration

void am_open_audio_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"audio_node", create_audio_node},
        {"oscillator", create_oscillator_node},
        {"track", create_audio_track_node},
        {"stream", create_audio_stream_node},
        {"decode_ogg", decode_ogg},
        {"root_audio_node", get_root_audio_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_audio_node_mt(L);
    register_gain_node_mt(L);
    register_lowpass_filter_node_mt(L);
    register_highpass_filter_node_mt(L);
    register_audio_track_node_mt(L);
    register_audio_stream_node_mt(L);
    register_oscillator_node_mt(L);

    audio_context.sample_rate = am_conf_audio_sample_rate;
    audio_context.sync_id = 0;
    audio_context.render_id = 0;

    // Create root audio node
    create_audio_node(L);
    audio_context.root = am_get_userdata(L, am_audio_node, -1);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_ROOT_AUDIO_NODE);
}
