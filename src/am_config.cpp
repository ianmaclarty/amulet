#include "amulet.h"

const char *am_conf_app_title = NULL;
const char *am_conf_app_author = NULL;
const char *am_conf_app_id = NULL;
const char *am_conf_app_id_ios = NULL;
const char *am_conf_app_id_mac = NULL;
const char *am_conf_app_id_android = NULL;
const char *am_conf_app_version = NULL;
const char *am_conf_app_shortname = NULL;
const char *am_conf_app_display_name = NULL;
const char *am_conf_app_dev_region = NULL;
const char *am_conf_app_supported_languages = NULL;
am_display_orientation am_conf_app_display_orientation = AM_DISPLAY_ORIENTATION_ANY;
const char *am_conf_app_icon = NULL;
const char *am_conf_app_icon_mac = NULL;
const char *am_conf_app_icon_ios = NULL;
const char *am_conf_app_icon_android = NULL;
const char *am_conf_app_launch_image = NULL;
const char *am_conf_luavm = NULL;
const char *am_conf_support_email = NULL;
const char *am_conf_copyright_message = NULL;

const char *am_conf_mac_category = NULL;
const char *am_conf_mac_application_cert_identity = NULL;
const char *am_conf_mac_installer_cert_identity = NULL;

const char *am_conf_ios_cert_identity = NULL;
const char *am_conf_ios_dev_cert_identity = NULL;
const char *am_conf_ios_appstore_cert_identity = NULL;
const char *am_conf_ios_code_sign_identity = NULL;
const char *am_conf_ios_dev_prov_profile_name = NULL;
const char *am_conf_ios_dist_prov_profile_name = NULL;

bool am_conf_game_center_enabled = true;
bool am_conf_icloud_enabled = true;

const char *am_conf_google_play_services_id = NULL;
const char *am_conf_android_app_version_code = NULL;
const char *am_conf_android_target_sdk_version = NULL;
const char *am_conf_android_adaptive_icon_fg = NULL;
const char *am_conf_android_adaptive_icon_bg = NULL;
bool am_conf_android_needs_internet_permission = false;
bool am_conf_android_needs_billing_permission = false;

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

// This determines whether non-pooled buffers are allocated
// using lua's allocator or malloc. If a buffer's data area is smaller
// than or equal to this threshold then the buffer's header and data area
// will be allocated as one contiguous Lua userdata block and won't require a
// __gc metamethod. We avoid allocating all buffers this way, because
// that seems to lead to larger memory consumption. My guess is that's because
// it raises the Lua GC's high water mark too much.
int am_conf_buffer_malloc_threshold = 512;

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
    *val = (char*)malloc(strlen(lstr) + 1);
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
    free_if_not_null((void**)&am_conf_app_id_ios);
    free_if_not_null((void**)&am_conf_app_id_mac);
    free_if_not_null((void**)&am_conf_app_id_android);
    free_if_not_null((void**)&am_conf_app_version);
    free_if_not_null((void**)&am_conf_app_shortname);
    free_if_not_null((void**)&am_conf_app_display_name);
    free_if_not_null((void**)&am_conf_app_dev_region);
    free_if_not_null((void**)&am_conf_app_supported_languages);
    free_if_not_null((void**)&am_conf_app_icon);
    free_if_not_null((void**)&am_conf_app_icon_mac);
    free_if_not_null((void**)&am_conf_app_icon_ios);
    free_if_not_null((void**)&am_conf_app_icon_android);
    free_if_not_null((void**)&am_conf_app_launch_image);
    free_if_not_null((void**)&am_conf_luavm);
    free_if_not_null((void**)&am_conf_support_email);
    free_if_not_null((void**)&am_conf_copyright_message);
    free_if_not_null((void**)&am_conf_mac_category);
    free_if_not_null((void**)&am_conf_mac_application_cert_identity);
    free_if_not_null((void**)&am_conf_mac_installer_cert_identity);
    free_if_not_null((void**)&am_conf_ios_cert_identity);
    free_if_not_null((void**)&am_conf_ios_dev_cert_identity);
    free_if_not_null((void**)&am_conf_ios_appstore_cert_identity);
    free_if_not_null((void**)&am_conf_ios_code_sign_identity);
    free_if_not_null((void**)&am_conf_ios_dev_prov_profile_name);
    free_if_not_null((void**)&am_conf_ios_dist_prov_profile_name);
    free_if_not_null((void**)&am_conf_google_play_services_id);
    free_if_not_null((void**)&am_conf_android_app_version_code);
    free_if_not_null((void**)&am_conf_android_target_sdk_version);
    free_if_not_null((void**)&am_conf_android_adaptive_icon_fg);
    free_if_not_null((void**)&am_conf_android_adaptive_icon_bg);
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
    read_string_setting(eng->L, "appid", &am_conf_app_id, "unknown.app.id");
    read_string_setting(eng->L, "appid_ios", &am_conf_app_id_ios, am_conf_app_id);
    read_string_setting(eng->L, "appid_mac", &am_conf_app_id_mac, am_conf_app_id);
    read_string_setting(eng->L, "appid_android", &am_conf_app_id_android, am_conf_app_id);
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
    read_string_setting(eng->L, "icon_mac", &am_conf_app_icon_mac, am_conf_app_icon);
    read_string_setting(eng->L, "icon_ios", &am_conf_app_icon_ios, am_conf_app_icon);
    read_string_setting(eng->L, "icon_android", &am_conf_app_icon_android, am_conf_app_icon);
    read_string_setting(eng->L, "launch_image", &am_conf_app_launch_image, NULL);
    read_string_setting(eng->L, "luavm", &am_conf_luavm, NULL);
    read_string_setting(eng->L, "support_email", &am_conf_support_email, NULL);
    read_string_setting(eng->L, "copyright_message", &am_conf_copyright_message, "");

    read_string_setting(eng->L, "mac_category", &am_conf_mac_category, "public.app-category.games");
    read_string_setting(eng->L, "mac_application_cert_identity", &am_conf_mac_application_cert_identity, NULL);
    read_string_setting(eng->L, "mac_installer_cert_identity", &am_conf_mac_installer_cert_identity, NULL);

    read_string_setting(eng->L, "ios_cert_identity", &am_conf_ios_cert_identity, NULL);
    read_string_setting(eng->L, "ios_dev_cert_identity", &am_conf_ios_dev_cert_identity, am_conf_ios_cert_identity);
    read_string_setting(eng->L, "ios_appstore_cert_identity", &am_conf_ios_appstore_cert_identity, am_conf_ios_cert_identity);
    read_string_setting(eng->L, "ios_code_sign_identity", &am_conf_ios_code_sign_identity, "iPhone Distribution");
    read_string_setting(eng->L, "ios_dev_prov_profile_name", &am_conf_ios_dev_prov_profile_name, NULL);
    read_string_setting(eng->L, "ios_dist_prov_profile_name", &am_conf_ios_dist_prov_profile_name, NULL);

    read_bool_setting(eng->L, "game_center_enabled", &am_conf_game_center_enabled);
    read_bool_setting(eng->L, "icloud_enabled", &am_conf_icloud_enabled);

    read_string_setting(eng->L, "google_play_services_id", &am_conf_google_play_services_id, "0");
    read_string_setting(eng->L, "android_app_version_code", &am_conf_android_app_version_code, "1");
    read_string_setting(eng->L, "android_target_sdk_version", &am_conf_android_target_sdk_version, "34");
    read_string_setting(eng->L, "android_adaptive_icon_fg", &am_conf_android_adaptive_icon_fg, NULL);
    read_string_setting(eng->L, "android_adaptive_icon_bg", &am_conf_android_adaptive_icon_bg, NULL);
    read_bool_setting(eng->L, "android_needs_internet_permission", &am_conf_android_needs_internet_permission);
    read_bool_setting(eng->L, "android_needs_billing_permission", &am_conf_android_needs_billing_permission);
    // XXX not sure this is required?
    //if (am_conf_android_needs_billing_permission) {
    //    am_conf_android_needs_internet_permission = true;
    //}

    read_bool_setting(eng->L, "d3dangle", &am_conf_d3dangle);
    #if !defined(AM_WINDOWS)
        am_conf_d3dangle = false;
    #endif
    am_destroy_engine(eng);
    return true;
}
