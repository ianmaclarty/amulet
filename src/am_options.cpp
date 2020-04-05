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
        "Usage: amulet export [-windows] [-windows64] [-mac] [-linux] [-html] \n"
        "                     [-ios-xcode-proj] [-android-studio-proj] [-datapak]\n"
        "                     [-a] [-r] [-d <out-dir>] [-o <out-path>] [-nozipdir] [ <dir> ]\n"
        "\n"
        "  Exports distribution packages for the project in <dir>,\n"
        "  or the current directory if <dir> is omitted.\n"
        "  <dir> should contain main.lua and conf.lua.\n"
        "\n"
        "  If no export platform is specified, then packages for windows,\n"
        "  mac and linux will be generated.\n"
        "\n"
        "  Unless the -a options is given, only files with the following\n"
        "  extensions will be included: .lua .png .jpg .ogg .obj .json .frag .vert.\n"
        "  All .txt files in <dir> will also be copied to the generated zip and\n"
        "  be visible to the user when they extract it (this is meant for REAMDEs).\n"
        "\n"
        "  If the -r option is given then all subdirectories of <dir> are included\n"
        "  recursively, otherwise only the files in <dir> are included.\n"
        "\n"
        "  The -d option can be used to specify the directory to export the packages to.\n"
        "  By default packages are exported to the current directory.\n"
        "\n"
        "  The -o option allows you to specify the complete path (dir + filename) of\n"
        "  the generated package. In this case the -d option is ignored. The -o option\n"
        "  doesn't work if you're exporting multiple platforms at once.\n"
        "\n"
        "  The -datapak option causes the platform agnostic data.pak file to be generated.\n"
        "  This contains all your project's Lua source, images and other assets.\n"
        "  You can use this to create your own custom distribution or update a previously\n"
        "  generated distribution manually.\n"
        "\n"
        "  As a courtesy to the user, the generate zip packages will contain the game\n"
        "  files in a sub-folder. If you instead want the game files to appear in the\n"
        "  root of the zip, use -nozipdir. You might want this if the game will\n"
        "  run from a launcher, such as Steam.\n"
        "\n"
        "  A minimal conf.lua might look something like this:\n"
        "\n"
        "    title = \"My Game Title\"\n"
        "    shortname = \"mygame\"\n"
        "    author = \"Your Name\"\n"
        "    version = \"1.0.0\"\n"
        "\n"
        "  See the online docs for the full list of conf options.\n"
        "\n"
        "  IMPORTANT: avoid unzipping and re-zipping the generated packages manually\n"
        "  as you may inadvertently strip the executable bit from some files,\n"
        "  causing them not to work on certain platforms.\n"
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
        "                   [-no-premult] [-keep-padding] [-no-border] <files> ...\n"
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
        "  -no-border               Do not add a transparent border around images.\n"
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
        "  export ...         Generate distribution packages\n"
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
    am_export_flags flags;
    int n = *argc;
    int i;
    for (i = 0; i < n; i++) {
        char *arg = (*argv)[i];
        if (strcmp(arg, "-windows") == 0) {
            flags.export_windows = true;
        } else if (strcmp(arg, "-windows64") == 0) {
            flags.export_windows64 = true;
        } else if (strcmp(arg, "-mac") == 0) {
            flags.export_mac = true;
        } else if (strcmp(arg, "-mac-app-store") == 0) {
            flags.export_mac_app_store = true;
        } else if (strcmp(arg, "-linux") == 0) {
            flags.export_linux = true;
        } else if (strcmp(arg, "-ios-xcode-proj") == 0) {
            flags.export_ios_xcode_proj = true;
        } else if (strcmp(arg, "-android-studio-proj") == 0) {
            flags.export_android_studio_proj = true;
        } else if (strcmp(arg, "-html") == 0) {
            flags.export_html = true;
        } else if (strcmp(arg, "-datapak") == 0) {
            flags.export_data_pak = true;
        } else if (strcmp(arg, "-r") == 0) {
            flags.recurse = true;
        } else if (strcmp(arg, "-a") == 0) {
            flags.allfiles = true;
        } else if (strcmp(arg, "-nozipdir") == 0) {
            flags.zipdir = false;
        } else if (strcmp(arg, "-d") == 0) {
            if (i >= n - 1) {
                fprintf(stderr, "Missing -d argument value.\n");
                return false;
            }
            i++;
            arg = (*argv)[i];
            flags.outdir = arg;
        } else if (strcmp(arg, "-o") == 0) {
            if (i >= n - 1) {
                fprintf(stderr, "Missing -o argument value.\n");
                return false;
            }
            i++;
            arg = (*argv)[i];
            flags.outpath = arg;
        } else if (strcmp(arg, "-debug") == 0) {
            flags.debug = true;
        } else {
            break;
        }
    }
    *argc -= i;
    *argv += i;
    if (flags.num_platforms() == 0) {
        flags.export_windows = true;
        flags.export_mac = true;
        flags.export_linux = true;
    }
    if (flags.outpath != NULL && flags.num_platforms() != 1) {
        fprintf(stderr, "If the -o option is given, then exactly one export platform must be specified.\n");
        return false;
    }
    char *dir = (char*)".";
    if (*argc > 0) {
        dir = (*argv)[0];
    }
    char last = dir[strlen(dir)-1];
    if (last == '/' || last == '\\') dir[strlen(dir)-1] = 0;
    am_opt_data_dir = dir;
    return am_build_exports(&flags);
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

static bool d3dangle_opt(int *argc, char ***argv) {
    am_conf_d3dangle = true;
    #if !defined(AM_WINDOWS)
        am_conf_d3dangle = false;
    #endif
    return true;
}

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
    {"-d3dangle",   d3dangle_opt, false},

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
