#include "amulet.h"

#ifdef AM_OSX
#import <AppKit/AppKit.h>
#include "SDL.h" // for SDL_GetBasePath
#endif

const char *am_opt_main_module = NULL;
const char *am_opt_data_dir = ".";

struct option {
    const char *name;
    bool (*handle_opt)(int *, char ***);
    bool stop;
};

static bool help(int *argc, char ***argv) {
    printf(
       /*-------------------------------------------------------------------------------*/
        "Usage: amulet [ <command> ] [ <file> ... ]\n"
        "\n"
        "  If no command is supplied, amulet runs the lua script <file>.\n"
      //"  If <file> is a directory, it runs main.lua in that directory.\n"
        "  If no file is supplied, it runs main.lua in the current directory.\n"
      //"  Failing that it looks for data/main.lua. Finally it\n"
      //"  tries to run data.pak in the current directory, where data.pak is\n"
      //"  a zipped archive containing main.lua.\n"
        "  Any extra arguments after <file> are passed on to the script\n"
        "  where they can be accessed via the arg global.\n"
        "\n"
        "Commands:\n"
        "  help            Show help message.\n"
        "  version         Show version.\n"
      //"  dist [<dir>]    Generate distributions for the project in <dir>.\n"
      //"                  <dir> should contain main.lua.\n"
      //"                  If <dir> is omitted, it is assumed to be the\n"
      //"                  current directory.\n"
       /*-------------------------------------------------------------------------------*/
    );
    return true;
}

static bool version(int *argc, char ***argv) {
    printf("Amulet %s\n", am_version);
    return true;
}

static bool dist(int *argc, char ***argv) {
    fprintf(stderr, "NYI\n");
    return false;
}

static option options[] = {
    {"help", help, true},
    {"version", version, true},
    {"dist", dist, true},

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
    char *filename = NULL;
    while (*argc > 0) {
        bool foundopt = false;
        char *arg = (*argv)[0];
        option *o = &options[0];
        while (o->name != NULL) {
            if (strcmp(o->name, arg) == 0) {
                (*argc)--;
                (*argv)++;
                if (!o->handle_opt(argc, argv)) {
                    *exit_status = EXIT_FAILURE;
                    return false;
                }
                if (o->stop) return false;
                foundopt = true;
                break;
            }
            o++;
        }
        if (!foundopt) {
            if (strstr(arg, ".lua")) {
                (*argc)--;
                (*argv)++;
                filename = arg;
            }
            break; // pass remaining args to script
        }
    }

    if (filename == NULL) {
        if (am_file_exists("main.lua")) {
            am_opt_main_module = "main";
        } else if (am_file_exists("./data/main.lua")) {
            am_opt_data_dir = "./data";
            am_opt_main_module = "main";
        } else if (!am_file_exists("data.pak")) {
            fprintf(stderr, 
                "No main.lua, data/main.lua or data.pak file found.\n"
                "Type \"amulet help\" for usage information.\n");
            *exit_status = EXIT_FAILURE;
            return false;
        }
    } else {
        if (!am_file_exists(filename)) {
            fprintf(stderr, "File '%s' not found.\nType \"amulet help\" for usage information.\n", filename);
            *exit_status = EXIT_FAILURE;
            return false;
        }
        char *lua_ext = strstr(filename, ".lua");
        if (lua_ext == NULL) {
            fprintf(stderr, "File must end with .lua.\nType \"amulet help\" for usage information.\n");
            *exit_status = EXIT_FAILURE;
            return false;
        }
        *lua_ext = '\0'; // strip extension
        char *last_slash = am_max(strrchr(filename, AM_PATH_SEP), strrchr(filename, '/')); // accept / on windows
        if (last_slash != NULL) {
            am_opt_data_dir = filename;
            *last_slash = '\0';
            filename = last_slash+1;
        }
        am_opt_main_module = filename;
    }

    return true;
}
