int am_conf_default_recursion_limit = 8;
int am_conf_default_action_priority = 100;

int am_conf_fixed_update_time = -1.0;
int am_conf_vsync = true;

// am_conf_audio_buffer_size is number of samples for each channel
#if defined(AM_BACKEND_EMSCRIPTEN)
// More likely to get drop outs in browser, so use larger buffer
int am_conf_audio_buffer_size = 2048;
#else
int am_conf_audio_buffer_size = 1024;
#endif

int am_conf_audio_channels = 2;
int am_conf_audio_sample_rate = 44100;

// Note: enabling either of the following two options causes substantial
// slowdowns on the html backend in some browsers
bool am_conf_validate_shader_programs = false;
bool am_conf_check_gl_errors = false;

bool am_conf_dump_translated_shaders = false;
