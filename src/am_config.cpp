int am_conf_default_recursion_limit = 8;
int am_conf_default_action_priority = 100;

int am_conf_fixed_update_time = -1.0;
int am_conf_vsync = true;

int am_conf_audio_buffer_size = 512; // samples per channel
int am_conf_audio_channels = 2;
int am_conf_audio_sample_rate = 48000;

// Note: enabling these causes substantial slowdowns on html
bool am_conf_validate_shader_programs = false;
bool am_conf_check_gl_errors = false;
