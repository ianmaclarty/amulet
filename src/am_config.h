// app settings
extern const char *am_conf_app_title;
extern const char *am_conf_app_author;
extern const char *am_conf_app_id;
extern const char *am_conf_app_id_ios;
extern const char *am_conf_app_id_mac;
extern const char *am_conf_app_id_android;
extern const char *am_conf_app_version;
extern const char *am_conf_app_shortname;
extern const char *am_conf_app_display_name;
extern const char *am_conf_app_dev_region;
extern const char *am_conf_app_supported_languages;
extern am_display_orientation am_conf_app_display_orientation;
extern const char *am_conf_app_icon;
extern const char *am_conf_app_icon_mac;
extern const char *am_conf_app_icon_ios;
extern const char *am_conf_app_icon_android;
extern const char *am_conf_app_launch_image;
extern const char *am_conf_luavm;
extern const char *am_conf_support_email;
extern const char *am_conf_copyright_message;

extern const char *am_conf_mac_category;
extern const char *am_conf_mac_application_cert_identity;
extern const char *am_conf_mac_installer_cert_identity;

extern const char *am_conf_ios_cert_identity;
extern const char *am_conf_ios_dev_cert_identity;
extern const char *am_conf_ios_appstore_cert_identity;
extern const char *am_conf_ios_code_sign_identity;
extern const char *am_conf_ios_dev_prov_profile_name;
extern const char *am_conf_ios_dist_prov_profile_name;

extern bool am_conf_game_center_enabled;
extern bool am_conf_icloud_enabled;

extern const char *am_conf_google_play_services_id;
extern const char *am_conf_android_app_version_code;
extern const char *am_conf_android_target_sdk_version;
extern const char *am_conf_android_adaptive_icon_fg;
extern const char *am_conf_android_adaptive_icon_bg;
extern bool am_conf_android_needs_internet_permission;
extern bool am_conf_android_needs_billing_permission;

// graphic driver options
extern bool am_conf_d3dangle;

// scene options
extern int am_conf_default_recursion_limit;
extern const char *am_conf_default_modelview_matrix_name;
extern const char *am_conf_default_projection_matrix_name;

// game loop options
extern double am_conf_fixed_delta_time;
extern double am_conf_delta_time_step;
extern double am_conf_min_delta_time;
extern double am_conf_max_delta_time;
extern double am_conf_warn_delta_time;
extern bool am_conf_vsync;

// audio options
extern int am_conf_audio_buffer_size;
extern int am_conf_audio_channels;
extern int am_conf_audio_sample_rate;
extern int am_conf_audio_interpolate_samples;
extern bool am_conf_audio_mute;

// memory options
extern int am_conf_buffer_malloc_threshold;

// dev options
extern bool am_conf_validate_shader_programs;
extern bool am_conf_check_gl_errors;
extern bool am_conf_dump_translated_shaders;
extern bool am_conf_log_gl_calls;
extern int am_conf_log_gl_frames;
extern bool am_conf_no_close_lua;
extern char *am_conf_test_lang;

bool am_load_config();
