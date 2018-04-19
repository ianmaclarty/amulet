#include "amulet.h"

const char *am_conf_app_title = NULL;
const char *am_conf_app_author = NULL;
const char *am_conf_app_id = NULL;
const char *am_conf_app_version = NULL;
const char *am_conf_app_shortname = NULL;
const char *am_conf_app_display_name = NULL;
const char *am_conf_app_dev_region = NULL;
const char *am_conf_app_supported_languages = NULL;
am_display_orientation am_conf_app_display_orientation = AM_DISPLAY_ORIENTATION_ANY;
const char *am_conf_app_icon = NULL;
const char *am_conf_app_launch_image = NULL;
const char *am_conf_luavm = NULL;

int am_conf_default_recursion_limit = 8;
const char *am_conf_default_modelview_matrix_name = "MV";
const char *am_conf_default_projection_matrix_name = "P";

bool am_conf_d3dangle = false;

double am_conf_fixed_delta_time = -1.0; //1.0 / 60.0;
double am_conf_delta_time_step = -1.0; //1.0/240.0;
double am_conf_min_delta_time = 1.0/240.0;
double am_conf_max_delta_time = 1.0/15.0;
double am_conf_warn_delta_time = -1.0; //1.0/30.0;
bool am_conf_vsync = true;

// am_conf_audio_buffer_size is number of samples for each channel
int am_conf_audio_buffer_size = 1024;
int am_conf_audio_channels = 2;
int am_conf_audio_sample_rate = 44100;
int am_conf_audio_interpolate_samples = 128; // must be less than am_conf_audio_buffer_size
bool am_conf_audio_mute = false;

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
int am_conf_log_gl_frames = 1000000;

bool am_conf_allow_restart = false;
bool am_conf_no_close_lua = false;
char *am_conf_test_lang = NULL;

static void read_string_setting(lua_State *L, const char *name, const char **value, const char *default_val) {
    lua_getglobal(L, name);
    const char *lstr = lua_tostring(L, -1);
    if (lstr == NULL) lstr = default_val;
    if (lstr == NULL) {
        *value = NULL;
        return;
    }
    char **val = (char**)value;
    *val = (char*)malloc(strlen(lstr) + 1); // never freed
    memcpy(*val, lstr, strlen(lstr) + 1);
    lua_pop(L, 1);
}

static void read_bool_setting(lua_State *L, const char *name, bool *value) {
    lua_getglobal(L, name);
    if (!lua_isnil(L, -1)) {
        *value = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
}

static void free_if_not_null(void **ptr) {
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

static void free_config() {
    free_if_not_null((void**)&am_conf_app_title);
    free_if_not_null((void**)&am_conf_app_author);
    free_if_not_null((void**)&am_conf_app_id);
    free_if_not_null((void**)&am_conf_app_version);
    free_if_not_null((void**)&am_conf_app_shortname);
    free_if_not_null((void**)&am_conf_app_display_name);
    free_if_not_null((void**)&am_conf_app_dev_region);
    free_if_not_null((void**)&am_conf_app_supported_languages);
    free_if_not_null((void**)&am_conf_app_icon);
    free_if_not_null((void**)&am_conf_app_launch_image);
    free_if_not_null((void**)&am_conf_luavm);
}

bool am_load_config() {
    free_config();
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
        bool res = am_run_script(eng->L, (char*)data, len, "conf.lua");
        free(data);
        if (!res) {
            am_destroy_engine(eng);
            return false;
        }
    }
    read_string_setting(eng->L, "title", &am_conf_app_title, "Untitled");
    read_string_setting(eng->L, "author", &am_conf_app_author, "Unknown");
    read_string_setting(eng->L, "shortname", &am_conf_app_shortname, am_conf_app_title);
    read_string_setting(eng->L, "appid", &am_conf_app_id, "null");
    read_string_setting(eng->L, "version", &am_conf_app_version, "0.0.0");
    read_string_setting(eng->L, "display_name", &am_conf_app_display_name, am_conf_app_title);
    read_string_setting(eng->L, "dev_region", &am_conf_app_dev_region, "en");
    read_string_setting(eng->L, "supported_languages", &am_conf_app_supported_languages, "en");
    const char *orientation_str = NULL;
    read_string_setting(eng->L, "orientation", &orientation_str, "any");
    if (strcmp(orientation_str, "any") == 0) {
        am_conf_app_display_orientation = AM_DISPLAY_ORIENTATION_ANY;
    } else if (strcmp(orientation_str, "portrait") == 0) {
        am_conf_app_display_orientation = AM_DISPLAY_ORIENTATION_PORTRAIT;
    } else if (strcmp(orientation_str, "landscape") == 0) {
        am_conf_app_display_orientation = AM_DISPLAY_ORIENTATION_LANDSCAPE;
    } else if (strcmp(orientation_str, "hybrid") == 0) {
        am_conf_app_display_orientation = AM_DISPLAY_ORIENTATION_HYBRID;
    } else {
        fprintf(stderr, "Invalid orientation in conf.lua: %s\n", orientation_str);
        am_destroy_engine(eng);
        return false;
    }
    read_string_setting(eng->L, "icon", &am_conf_app_icon, NULL);
    read_string_setting(eng->L, "launch_image", &am_conf_app_launch_image, NULL);
    read_string_setting(eng->L, "luavm", &am_conf_luavm, NULL);
    read_bool_setting(eng->L, "d3dangle", &am_conf_d3dangle);
    am_destroy_engine(eng);
    return true;
}
