#include "amulet.h"

static bool is_lang_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
}

static bool lang_is_supported(const char *lang) {
    int n = strlen(lang);
    const char *start = am_conf_app_supported_languages;
    char *ptr = strstr(start, lang);
    while (ptr != NULL) {
        if ((ptr == start || !is_lang_char(*(ptr-1))) && !is_lang_char(*(ptr + n))) {
            return true;
        }
        ptr = strstr(ptr + n, lang);
    }
    return false;
}

static char *get_supported_language(const char *lang) {
    if (am_conf_app_supported_languages == NULL) return am_format("%s", "en");
    if (lang_is_supported(lang)) return am_format("%s", lang);
    if (strchr(lang, '-') != NULL) {
        char *lang2 = am_format("%s", lang);
        char *dash = strchr(lang2, '-');
        *dash = '\0';
        if (lang_is_supported(lang2)) {
            return lang2;
        }
    }
    return am_format("%s", "en");
}

static int language(lua_State *L) {
    static char *lang = NULL;
    if (lang == NULL) {
        if (am_conf_test_lang != NULL) {
            lang = get_supported_language(am_conf_test_lang);
        } else {
            lang = get_supported_language(am_preferred_language());
        }
    }
    lua_pushstring(L, lang);
    return 1;
}

void am_open_i18n_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"language", language},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
}
