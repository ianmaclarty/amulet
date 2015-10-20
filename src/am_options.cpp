#include "amulet.h"

#ifdef AM_OSX
#import <AppKit/AppKit.h>
#include "SDL.h" // for SDL_GetBasePath
#endif

enum e_main_command {
    CMD_NULL,
    CMD_RUN,
    CMD_PAK,
};

static e_main_command main_command = CMD_NULL;
static char *pak_dir;

const char *am_opt_main_module = NULL;
const char *am_opt_data_dir = ".";

static int handle_non_run_options();

static bool pak(int *argc, char ***argv) {
    if (main_command == CMD_NULL) {
        main_command = CMD_PAK;
        if (*argc > 0) {
            pak_dir = (*argv)[0];
            (*argc)--;
            (*argv)++;
            if (!am_file_exists(pak_dir)) {
                am_log0("directory %s does not exist", pak_dir);
                return false;
            } else {
                return true;
            }
        } else {
            am_log0("%s", "missing -pak directory option");
            return false;
        }
    } else {
        am_log0("%s", "multiple conflicting commands given");
        return false;
    }
}

struct option {
    const char *name;
    bool (*func)(int *, char ***);
};

static option options[] = {
    {"-pak", pak},
    {NULL, NULL}
};

bool am_process_args(int *argc, char ***argv, int *exit_status) {

    // skip first arg
    (*argc)--;
    (*argv)++;

#ifdef AM_OSX
    NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
    bool in_osx_bundle = (bundleIdentifier != nil);
    if (in_osx_bundle) {
        // Use Resource dir of OS X bundle
        am_opt_data_dir = SDL_GetBasePath();
        // When invoked from an OSX bundle extra arguments may be set by the OS,
        // so don't try processing them.
        return true;
    }
#endif
    if (am_file_exists("data")) {
        am_opt_data_dir = "./data";
    }

    char *filename = NULL;
    while (*argc > 0) {
        bool foundopt = false;
        char *arg = (*argv)[0];
        option *o = &options[0];
        while (o->name != NULL) {
            if (strcmp(o->name, arg) == 0) {
                (*argc)--;
                (*argv)++;
                if (!o->func(argc, argv)) {
                    *exit_status = EXIT_FAILURE;
                    return false;
                }
                foundopt = true;
                break;
            }
            o++;
        }
        if (!foundopt) {
            if (filename == NULL && (main_command == CMD_NULL || main_command == CMD_RUN)) {
                (*argc)--;
                (*argv)++;
                main_command = CMD_RUN;
                filename = arg;
            }
            break; // pass remaining args to script
        }
    }

    if (main_command == CMD_NULL) {
        main_command = CMD_RUN;
    }

    if (main_command != CMD_RUN) {
        *exit_status = handle_non_run_options();
        return false;
    }

    if (filename != NULL) {
        char *last_slash = am_max(strrchr(filename, AM_PATH_SEP), strrchr(filename, '/')); // accept / on windows
        if (last_slash != NULL) {
            am_opt_data_dir = filename;
            *last_slash = '\0';
            filename = last_slash+1;
        }
        char *lua_ext = strstr(filename, ".lua");
        if (lua_ext != NULL && strlen(lua_ext) == 4) {
            *lua_ext = '\0'; // strip extension
        } 
        am_opt_main_module = filename;
    }
    return true;
}

static int handle_non_run_options() {
    switch (main_command) {
        case CMD_PAK:
            assert(pak_dir != NULL);
            if (am_build_package(pak_dir)) {
                return EXIT_SUCCESS;
            } else {
                return EXIT_FAILURE;
            }
            break;
        case CMD_NULL:
        case CMD_RUN:
            am_log0("%s", "internal command handling error");
            return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}
