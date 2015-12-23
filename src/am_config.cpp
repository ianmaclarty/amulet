#include "amulet.h"

const char *am_conf_app_title = NULL;
const char *am_conf_app_id = NULL;
const char *am_conf_app_version = NULL;

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

static void read_string_setting(lua_State *L, const char *name, const char **value, const char *default_val) {
    lua_getglobal(L, name);
    const char *lstr = lua_tostring(L, -1);
    if (lstr == NULL) lstr = default_val;
    char **val = (char**)value;
    *val = (char*)malloc(strlen(lstr) + 1); // never freed
    memcpy(*val, lstr, strlen(lstr) + 1);
    lua_pop(L, 1);
}

bool am_load_config() {
    am_engine *eng = am_init_engine(true, 0, NULL);
    if (eng == NULL) return false;
    // remove globals metatable, so we can set conf options as globals
    lua_getglobal(eng->L, "_G");
    lua_pushnil(eng->L);
    lua_setmetatable(eng->L, -2);
    lua_pop(eng->L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource("conf.lua", &len, &errmsg);
    if (data == NULL) {
        // assume conf.lua doesn't exist
        free(errmsg);
    } else {
        if (!am_run_script(eng->L, (char*)data, len, "conf.lua")) {
            return false;
        }
        read_string_setting(eng->L, "title", &am_conf_app_title, "Untitled");
        read_string_setting(eng->L, "id", &am_conf_app_id, "null");
        read_string_setting(eng->L, "version", &am_conf_app_version, "0.0.0");
    }
    am_destroy_engine(eng);
    return true;
}
