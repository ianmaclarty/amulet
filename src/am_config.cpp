#include "amulet.h"

int am_conf_default_recursion_limit = 8;
const char *am_conf_default_modelview_matrix_name = "MV";
const char *am_conf_default_projection_matrix_name = "P";

double am_conf_fixed_delta_time = -1.0;
double am_conf_min_delta_time = 1.0/15.0;
double am_conf_warn_delta_time = -1.0; //1.0/30.0;
int am_conf_vsync = true;

// am_conf_audio_buffer_size is number of samples for each channel
int am_conf_audio_buffer_size = 1024;

int am_conf_audio_channels = 2;
int am_conf_audio_sample_rate = 44100;
int am_conf_audio_interpolate_samples = 128; // must be less than am_conf_audio_buffer_size

lua_CFunction am_conf_main_load_func = &am_require;

// Note: enabling either of the following two options causes substantial
// slowdowns on the html backend in some browsers
#ifdef AM_DEBUG
bool am_conf_validate_shader_programs = true;
bool am_conf_check_gl_errors = true;
#else
bool am_conf_validate_shader_programs = false;
bool am_conf_check_gl_errors = false;
#endif

bool am_conf_dump_translated_shaders = false;
bool am_conf_log_gl_calls = false;

bool am_conf_allow_restart = false;
