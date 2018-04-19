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

static bool help_export() {
    printf(
       /*-------------------------------------------------------------------------------*/
        "Usage: amulet export [-r] [-windows] [-mac] [-ios] [-iossim] [-linux] [-html] [ <dir> ]\n"
        "\n"
        "  Generates distribution packages for the project in <dir>,\n"
        "  or the current directory if <dir> is omitted.\n"
        "  <dir> should contain main.lua.\n"
        "\n"
        "  If none of the -windows, -mac, -ios, -iossim -linux or -html options are given\n"
        "  then packages for all supported platforms will be generated,\n"
        "  otherwise only packages for the specified platforms will be generated.\n"
        "\n"
        "  All files with the following extensions will be included:\n"
        "  .lua .png .jpg .ogg .obj\n"
        "  All .txt files will also be copied to the generated zip and\n"
        "  be visible to the user when they open it.\n"
        "\n"
        "  If the -r option is given then all subdirectories of <dir> are included\n"
        "  recursively, otherwise only the files in <dir> are included.\n"
        "\n"
        "  If you create a conf.lua file in the same\n"
        "  directory as main.lua containing the following:\n"
        "\n"
        "    title = \"My Game Title\"\n"
        "    shortname = \"mygame\"\n"
        "    author = \"Your Name\"\n"
        "    appid = \"com.some-unique-id.123\"\n"
        "    version = \"1.0.0\"\n"
        "\n"
        "    display_name = \"My Game\"\n"
        "    dev_region = \"en\"\n"
        "    supported_languages = \"en,fr,nl,de,ja,zh-TW,zh-CN,ko\"\n"
        "    icon = \"icon.png\"\n"
        "    launch_image = \"launch.png\"\n"
        "    orientation = \"any\"\n"
        "\n"
        "  then this will be used for various bits of meta-data in the\n"
        "  generated packages.\n"
        "\n"
        "  IMPORTANT: avoid unzipping and re-zipping the generated packages\n"
        "  as you may inadvertently strip the executable bit from\n"
        "  some files, which will cause them not to work.\n"
        "\n"
       /*-------------------------------------------------------------------------------*/
    );
    return true;
}

static bool help_pack() {
    printf(
       /*-------------------------------------------------------------------------------*/
        "Usage: amulet pack -png <filename.png> -lua <filename.lua> \n"
        "                   [-mono] [-minfilter <filter>] [-magfilter <filter>]\n"
        "                   [-no-premult] [-keep-padding] <files> ...\n"
        "\n"
        "  Packs images and/or fonts into a sprite sheet and generates a Lua\n"
        "  module for accessing the sprites therein.\n"
        "\n"
        "Options:\n"
        "  -png <filename.png>      The name of the png file to generate.\n"
        "  -lua <filename.lua>      The name of the Lua module to generate.\n"
        "  -mono                    Do not anti-alias fonts.\n"
        "  -minfilter               nearest or linear (default is linear).\n"
        "  -magfilter               nearest or linear (default is linear).\n"
        "  -no-premult              Do not pre-multiply RGB channels by alpha.\n"
        "  -keep-padding            Do not strip transparent pixels around images.\n"
        "\n"
        "  <files> is a list of image files (.png or .jpg) and/or font files (.ttf).\n"
        "  Each font file must additionally have a suffix of the form @size which\n"
        "  specifies the size of the font in pixels.\n"
        "  Optionally each font item may also be followed by a colon and a comma\n"
        "  separated list of character ranges to include in the sprite sheet.\n"
        "  The characters in the character range can be written directly if they\n"
        "  are ASCII, or given as hexidecimal unicode codepoints.\n"
        "  Here are some examples of valid font items:\n"
        "    times.ttf@64\n"
        "    VeraMono.ttf@32:A-Z,0-9\n"
        "    ComicSans.ttf@122:0x20-0xFF\n"
        "    gbsn00lp.ttf@42:a-z,A-Z,0-9,0x4E00-0x9FFF\n"
        "  If the character range list is omitted, it defaults to 0x20-0x7E\n"
        "  i.e: !\"#$%%&'()*+,-./:;<=>?@[\\]^_`{|}~\n"
        "  0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.\n"
        "\n"
        "Example:\n"
        "\n"
        "  amulet pack -png sprites.png -lua sprites.lua images/*.png fonts/myfont.ttf@16\n"
        "\n"
        "This will produce sprites.lua and sprites.png.\n"
        "Use them in your Lua code like so:\n"
        "\n"
        "  local sprites = require \"sprites\"\n"
        "  local sprite_node = am.sprite(sprites.img1)\n"
        "  local text_node = am.text(sprites.myfont16, \"BARF!\")\n"
        "\n"
       /*-------------------------------------------------------------------------------*/
    );
    return true;
}

static bool help_cmd(int *argc, char ***argv) {
    if (*argc > 0 && strcmp((*argv)[0], "export") == 0) {
        return help_export();
    } else if (*argc > 0 && strcmp((*argv)[0], "pack") == 0) {
        return help_pack();
    }
    printf(
       /*-------------------------------------------------------------------------------*/
        "Usage: amulet [ <options> ] [ <file> ] ...\n"
        "   or: amulet <command> ...\n"
        "\n"
        "If no command is supplied, amulet runs the lua script <file>.\n"
        "If no file is supplied, it tries to run main.lua in the current directory.\n"
      //"Failing that it looks for data/main.lua. Finally it\n"
      //"tries to run data.pak in the current directory, where data.pak is\n"
      //"a zipped archive containing main.lua.\n"
        "Any extra arguments after <file> are passed on to the script\n"
        "where they can be accessed via the arg global.\n"
        "\n"
        "Options:\n"
        "  -mute              Mute audio\n"
        "  -lang <lang>       Pretend <lang> is the current system language\n"
        "\n"
        "Commands:\n"
        "  help [ <command> ] Show help\n"
        "  version            Show version\n"
#ifdef AM_EXPORT
        "  export [ <dir> ]   Generate distribution packages\n"
#endif
#ifdef AM_SPRITEPACK
        "  pack ...           Generate sprite sheet from images and/or fonts\n"
#endif
        "\n"
       /*-------------------------------------------------------------------------------*/
    );
    return true;
}

static bool version_cmd(int *argc, char ***argv) {
    printf("Amulet %s\n", am_version);
    return true;
}

static bool export_cmd(int *argc, char ***argv) {
#ifdef AM_EXPORT
    uint32_t flags = 0;
    int n = *argc;
    int i;
    for (i = 0; i < n; i++) {
        char *arg = (*argv)[i];
        if (strcmp(arg, "-windows") == 0) {
            flags |= AM_EXPORT_FLAG_WINDOWS;
        } else if (strcmp(arg, "-mac") == 0) {
            flags |= AM_EXPORT_FLAG_OSX;
        } else if (strcmp(arg, "-linux") == 0) {
            flags |= AM_EXPORT_FLAG_LINUX;
        } else if (strcmp(arg, "-ios") == 0) {
            flags |= AM_EXPORT_FLAG_IOS;
        } else if (strcmp(arg, "-iossim") == 0) {
            flags |= AM_EXPORT_FLAG_IOSSIM;
        } else if (strcmp(arg, "-html") == 0) {
            flags |= AM_EXPORT_FLAG_HTML;
        } else if (strcmp(arg, "-r") == 0) {
            flags |= AM_EXPORT_FLAG_RECURSE;
        } else {
            break;
        }
    }
    *argc -= i;
    *argv += i;
    if (flags == 0 || flags == AM_EXPORT_FLAG_RECURSE) flags |= AM_EXPORT_FLAG_WINDOWS | AM_EXPORT_FLAG_OSX | AM_EXPORT_FLAG_LINUX | AM_EXPORT_FLAG_IOS | AM_EXPORT_FLAG_HTML;
    char *dir = (char*)".";
    if (*argc > 0) {
        dir = (*argv)[0];
    }
    char last = dir[strlen(dir)-1];
    if (last == '/' || last == '\\') dir[strlen(dir)-1] = 0;
    am_opt_data_dir = dir;
    return am_build_exports(flags);
#else
    fprintf(stderr, "Sorry, the export command is not supported on this platform.\n");
    return false;
#endif
}

static bool pack_cmd(int *argc, char ***argv) {
#ifdef AM_SPRITEPACK
    return am_pack_sprites(*argc, *argv);
#else
    fprintf(stderr, "Sorry, the pack command is not supported on this platform.\n");
    return false;
#endif
}

static bool mute_opt(int *argc, char ***argv) {
    am_conf_audio_mute = true;
    return true;
}

static bool lang_opt(int *argc, char ***argv) {
    if (*argc > 0) {
        am_conf_test_lang = am_format("%s", (*argv)[0]);
        (*argc)--;
        (*argv)++;
        return true;
    } else {
        fprintf(stderr, "Missing -lang argument value.\n");
        return false;
    }
}

static bool gllog_opt(int *argc, char ***argv) {
    am_conf_log_gl_calls = true;
    return true;
}

static bool nocloselua_opt(int *argc, char ***argv) {
    am_conf_no_close_lua = true;
    return true;
}

#ifdef AM_WINDOWS
static bool d3dangle_opt(int *argc, char ***argv) {
    am_conf_d3dangle = true;
    return true;
}
#endif

static option options[] = {
    {"help",        help_cmd, true},
    {"-help",       help_cmd, true},
    {"--help",      help_cmd, true},
    {"version",     version_cmd, true},
    {"-version",    version_cmd, true},
    {"--version",   version_cmd, true},
    {"export",      export_cmd, true},
    {"pack",        pack_cmd, true},
    {"-mute",       mute_opt, false},
    {"-lang",       lang_opt, false},
    {"-gllog",      gllog_opt, false},
    {"-nocloselua", nocloselua_opt, false},
#ifdef AM_WINDOWS
    {"-d3dangle",   d3dangle_opt, false},
#endif

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
            if (*argc > 0) {
                fprintf(stderr, 
                    "'%s' is not a lua file and no main.lua found in the current directory.\n"
                    "Type 'amulet help' for usage information.\n", (*argv)[0]);
            } else {
                fprintf(stderr, 
                    "No main.lua found in the current directory.\n"
                    "Type 'amulet help' for usage information.\n");
            }
            *exit_status = EXIT_FAILURE;
            return false;
        }
    } else {
        if (!am_file_exists(filename)) {
            fprintf(stderr, "File '%s' not found.\nType 'amulet help' for usage information.\n", filename);
            *exit_status = EXIT_FAILURE;
            return false;
        }
        char *lua_ext = strstr(filename, ".lua");
        if (lua_ext == NULL) {
            fprintf(stderr, "File must end with .lua.\nType 'amulet help' for usage information.\n");
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
