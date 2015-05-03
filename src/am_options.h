extern const char *am_opt_main_module;
extern const char *am_opt_data_dir;

bool am_process_args(int *argc, char ***argv, int *exit_status);
bool am_handle_options(int *exit_status);
