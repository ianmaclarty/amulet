#ifdef AM_STEAMWORKS
#include "amulet.h"
#include "steam/steam_api.h"

#include <list>

bool am_steam_overlay_enabled = false;

enum AMSteamLeaderboardQueryType {
    AM_STEAM_GLOBAL,
    AM_STEAM_GLOBAL_NEIGHBOURHOOD,
    AM_STEAM_FRIENDS,
};

static lua_State *g_L = NULL;

static void amSteamInit(lua_State *L);
static void amSteamSubmitScore(lua_State *L, int func_ref, int score, const char *leaderboard);
static void amSteamQueryLeaderboard(lua_State *L, AMSteamLeaderboardQueryType t, int func_ref, const char *leaderboard, int before, int after);
static void amSteamSubmitAchievement(const char *achievement);
struct score_submission;

std::list<score_submission*> submissions;

struct score_submission {
    lua_State *L;
    int func_ref;
    char *leaderboard_name;
    int score;

    CCallResult<score_submission, LeaderboardFindResult_t> find_leaderboard_callback;
    CCallResult<score_submission, LeaderboardScoreUploaded_t> upload_score_callback;

    score_submission(lua_State *L, int func_ref, const char *name, int score) {
        //am_debug("%s", "new score submission");
        leaderboard_name = (char*)malloc(strlen(name) + 1);
        strcpy(leaderboard_name, name);
        score_submission::score = score;
        score_submission::L = L;
        score_submission::func_ref = func_ref;

        submissions.push_back(this);
        
        SteamAPICall_t call = SteamUserStats()->FindLeaderboard(name);
        find_leaderboard_callback.Set(call, this, 
          &score_submission::on_find_leaderboard);
    }

    virtual ~score_submission() {
        submissions.remove(this);
        free(leaderboard_name);
        if (L != NULL) {
            luaL_unref(L, LUA_REGISTRYINDEX, func_ref);
        }
        //am_debug("%s", "delete score submission");
    }

    void call_func(bool success) {
        //am_debug("%s", "score submission call_func");
        if (L == NULL || func_ref == LUA_NOREF) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
        lua_pushboolean(L, success);
        if (lua_pcall(L, 1, 0, 0)) {
            const char *err = lua_tostring(L, -1);
            if (err == NULL) err = "<unknown>";
            am_log1("error running score submission callback: %s", err);
            lua_pop(L, 1);
        }
    }

    void on_find_leaderboard(LeaderboardFindResult_t *res, bool io_failure) {
        //am_debug("%s", "score submission on_find_leaderboard");
        if (io_failure) {
            am_log1("network error while trying to retrieve leaderboard '%s'", leaderboard_name);
            call_func(false);
            delete this;
            return;
        }
        if (!res->m_bLeaderboardFound) {
            am_log1("leaderboard '%s' not found", leaderboard_name);
            call_func(false);
            delete this;
            return;
        }
        //am_debug("%s", "score submission on_find_leaderboard succeeded");

        SteamLeaderboard_t handle = res->m_hSteamLeaderboard;
        SteamAPICall_t call = 
            SteamUserStats()->UploadLeaderboardScore(handle, k_ELeaderboardUploadScoreMethodKeepBest, score, NULL, 0);
        upload_score_callback.Set(call, this, &score_submission::on_upload_score);
    }

    void on_upload_score(LeaderboardScoreUploaded_t *res, bool io_failure) {
        //am_debug("%s", "score submission on_upload_score");
        if (io_failure || !res->m_bSuccess) {
            am_log1("error while trying to upload score to leaderboard '%s'", leaderboard_name);
            call_func(false);
        } else {
            //am_log1("successfully submitted score %d to leaderboard '%s'", score, leaderboard_name);
            call_func(true);
        }
        delete this;
    }
};

struct leaderboard_neighbourhood_request;

std::list<leaderboard_neighbourhood_request*> neightbourhood_requests;

struct leaderboard_neighbourhood_request {
    char *leaderboard_name;
    lua_State *L;
    int func_ref;
    int before;
    int after;
    ELeaderboardDataRequest reqtype;

    CCallResult<leaderboard_neighbourhood_request, LeaderboardFindResult_t> find_leaderboard_callback;
    CCallResult<leaderboard_neighbourhood_request, LeaderboardScoresDownloaded_t> download_scores_callback;

    leaderboard_neighbourhood_request(lua_State *L, AMSteamLeaderboardQueryType t, int func_ref, const char *leaderboard, int before, int after) {
        leaderboard_name = (char*)malloc(strlen(leaderboard) + 1);
        strcpy(leaderboard_name, leaderboard);
        leaderboard_neighbourhood_request::L = L;
        leaderboard_neighbourhood_request::func_ref = func_ref;
        leaderboard_neighbourhood_request::before = before;
        leaderboard_neighbourhood_request::after = after;
        switch (t) {
            case AM_STEAM_GLOBAL: reqtype = k_ELeaderboardDataRequestGlobal; break;
            case AM_STEAM_GLOBAL_NEIGHBOURHOOD: reqtype = k_ELeaderboardDataRequestGlobalAroundUser; break;
            case AM_STEAM_FRIENDS: reqtype = k_ELeaderboardDataRequestFriends; break;
            default:
                reqtype = k_ELeaderboardDataRequestFriends;
        }

        neightbourhood_requests.push_back(this);
        
        SteamAPICall_t call = SteamUserStats()->FindLeaderboard(leaderboard_name);
        find_leaderboard_callback.Set(call, this, 
          &leaderboard_neighbourhood_request::on_find_leaderboard);
    }

    virtual ~leaderboard_neighbourhood_request() {
        neightbourhood_requests.remove(this);
        free(leaderboard_name);
        if (L != NULL) {
            luaL_unref(L, LUA_REGISTRYINDEX, func_ref);
        }
    }

    void call_func() {
        if (lua_pcall(L, 1, 0, 0)) {
            const char *err = lua_tostring(L, -1);
            if (err == NULL) err = "<unknown>";
            am_log1("error running leaderboard callback: %s", err);
            lua_pop(L, 1);
        }
    }

    void on_find_leaderboard(LeaderboardFindResult_t *res, bool io_failure) {
        if (L == NULL) {
            delete this;
            return;
        }
        if (io_failure) {
            am_log1("network error while trying to retrieve leaderboard '%s'", leaderboard_name);
            lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
            lua_pushnil(L);
            call_func();
            delete this;
            return;
        }
        if (!res->m_bLeaderboardFound) {
            am_log1("leaderboard '%s' not found", leaderboard_name);
            lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
            lua_pushnil(L);
            call_func();
            delete this;
            return;
        }

        SteamLeaderboard_t handle = res->m_hSteamLeaderboard;
        SteamAPICall_t call = 
            SteamUserStats()->DownloadLeaderboardEntries(handle, reqtype, before, after);
        download_scores_callback.Set(call, this, &leaderboard_neighbourhood_request::on_download_scores);
    }

    void on_download_scores(LeaderboardScoresDownloaded_t *res, bool io_failure) {
        if (L == NULL) {
            delete this;
            return;
        }
        if (io_failure) {
            am_log1("error while trying to download scores from leaderboard '%s'", leaderboard_name);
            lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
            lua_pushnil(L);
            call_func();
            delete this;
            return;
        }
        LeaderboardEntry_t entry;
        lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
        lua_newtable(L);
        for (int i = 0; i < res->m_cEntryCount; i++) {
            SteamUserStats()->GetDownloadedLeaderboardEntry(res->m_hSteamLeaderboardEntries, i, &entry, NULL, 0);
            const char *name = SteamFriends()->GetFriendPersonaName(entry.m_steamIDUser);
            int rank = entry.m_nGlobalRank;
            int score = entry.m_nScore;
            bool is_user = (entry.m_steamIDUser == SteamUser()->GetSteamID());
            lua_newtable(L);
            lua_pushstring(L, name);
            lua_setfield(L, -2, "name");
            lua_pushinteger(L, rank);
            lua_setfield(L, -2, "rank");
            lua_pushinteger(L, score);
            lua_setfield(L, -2, "score");
            lua_pushboolean(L, is_user);
            lua_setfield(L, -2, "is_user");
            lua_rawseti(L, -2, i + 1);
        }
        call_func();
        delete this;
    }
};

struct overlay_listener_t { 
    STEAM_CALLBACK( overlay_listener_t, OnGameOverlayActivated, GameOverlayActivated_t ); 
}; 

void overlay_listener_t::OnGameOverlayActivated( GameOverlayActivated_t* pCallback ) { 
    if ( pCallback->m_bActive ) {
        am_steam_overlay_enabled = true;
    } else {
        am_steam_overlay_enabled = false;
    }
}

static overlay_listener_t *overlay_listener = NULL;

static bool steam_initialized = false;

static void amSteamSubmitScore(lua_State *L, int func_ref, int score, const char *leaderboard) {
    if (steam_initialized) {
        new score_submission(L, func_ref, leaderboard, score);
    }
}

static void amSteamSubmitAchievement(const char *achievement) {
    if (steam_initialized) {
        SteamUserStats()->SetAchievement(achievement);
        SteamUserStats()->StoreStats();
    }
}

static void amSteamQueryLeaderboard(lua_State *L, AMSteamLeaderboardQueryType t, int func, const char *leaderboard, int before, int after) {
    if (steam_initialized) {
        new leaderboard_neighbourhood_request(L, t, func, leaderboard, before, after);
    }
}

static void amSteamInit(lua_State *L) {
    g_L = NULL;
    if (!steam_initialized) {
        steam_initialized = SteamAPI_Init();
        if (!steam_initialized) {
            am_log1("%s", "Unable to initialize steam");
            return;
        }
        if (!SteamUserStats()->RequestCurrentStats()) {
            am_log1("%s", "Unable to get steam user stats");
            SteamAPI_Shutdown();
            steam_initialized = false;
            return;
        }
        overlay_listener = new overlay_listener_t();
        g_L = L;
    }
}

void am_steam_teardown() {
    if (steam_initialized) {
        if (overlay_listener != NULL) {
            delete overlay_listener;
            overlay_listener = NULL;
        }
        for (std::list<leaderboard_neighbourhood_request*>::iterator it = neightbourhood_requests.begin(); it != neightbourhood_requests.end(); ++it) {
            (*it)->L = NULL;
        }
        neightbourhood_requests.clear();
        for (std::list<score_submission*>::iterator it = submissions.begin(); it != submissions.end(); ++it) {
            (*it)->L = NULL;
        }
        submissions.clear();
        SteamAPI_Shutdown();
        steam_initialized = false;
        g_L = NULL;
    }
}

void am_steam_step() {
    if (steam_initialized) {
        SteamAPI_RunCallbacks();
    }
}


static int submit_score(lua_State *L) {
    if (g_L == NULL) return 0;
    int nargs = am_check_nargs(L, 2);
    const char *leaderboard = lua_tostring(L, 1);
    if (leaderboard != NULL) {
        int score = lua_tointeger(L, 2);
        int ref = LUA_NOREF;
        if (nargs > 2) {
            if (!lua_isfunction(L, 3)) {
                return luaL_error(L, "expecting arg 3 to be a callback");
            }
            lua_pushvalue(L, 3);
            ref = luaL_ref(L, LUA_REGISTRYINDEX);
        }
        amSteamSubmitScore(g_L, ref, score, leaderboard);
    }
    return 0;
}

static int submit_achievement(lua_State *L) {
    if (g_L == NULL) return 0;
    am_check_nargs(L, 1);
    const char *achievement = lua_tostring(L, 1);
    if (achievement != NULL) {
        amSteamSubmitAchievement(achievement);
    }
    return 0;
}

static int query_leaderboard(lua_State *L) {
    if (g_L == NULL) return 0;
    am_check_nargs(L, 5);
    const char *name = lua_tostring(L, 1);
    if (name == NULL) {
        return luaL_error(L, "expecting leaderboard name in position 1");
    }
    const char *type = lua_tostring(L, 2);
    if (type == NULL) {
        return luaL_error(L, "expecting a query type in position 2");
    }
    AMSteamLeaderboardQueryType t;
    if (strcmp(type, "global") == 0) {
        t = AM_STEAM_GLOBAL;
    } else if (strcmp(type, "global_neighbourhood") == 0) {
        t = AM_STEAM_GLOBAL_NEIGHBOURHOOD;
    } else if (strcmp(type, "friends") == 0) {
        t = AM_STEAM_FRIENDS;
    } else {
        return luaL_error(L, "invalid leaderboard query type: %s", type);
    }
    int before = lua_tointeger(L, 3);
    int after = lua_tointeger(L, 4);
    if (!lua_isfunction(L, 5)) {
        return luaL_error(L, "expecting callback function in position 4");
    }
    lua_pushvalue(L, 5);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    amSteamQueryLeaderboard(g_L, t, ref, name, before, after);
    return 0;
}

static int steamworks_available(lua_State *L) {
    lua_pushboolean(L, steam_initialized ? 1 : 0);
    return 1;
}

void am_open_steamworks_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"submit_steamworks_score", submit_score},
        {"submit_steamworks_achievement", submit_achievement},
        {"query_steamworks_leaderboard", query_leaderboard},
        {"steamworks_available", steamworks_available},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    amSteamInit(L);
}

const char* am_get_steam_lang() {
    ISteamApps *apps = SteamApps();
    const char *lang = "";
    if (apps != NULL) {
        lang = apps->GetCurrentGameLanguage();
    }
    if (lang == NULL || strcmp(lang, "") == 0) {
        ISteamUtils *utils = SteamUtils();
        if (utils != NULL) {
            lang = utils->GetSteamUILanguage();
        }
    }
    if (lang == NULL) return "en";
    if (strcmp(lang, "english") == 0) return "en";
    if (strcmp(lang, "arabic") == 0) return "ar";
    if (strcmp(lang, "bulgarian") == 0) return "br";
    if (strcmp(lang, "schinese") == 0) return "zh-Hans";
    if (strcmp(lang, "tchinese") == 0) return "zh-Hant";
    if (strcmp(lang, "czech") == 0) return "cs";
    if (strcmp(lang, "danish") == 0) return "da";
    if (strcmp(lang, "dutch") == 0) return "nl";
    if (strcmp(lang, "finnish") == 0) return "fi";
    if (strcmp(lang, "french") == 0) return "fr";
    if (strcmp(lang, "german") == 0) return "de";
    if (strcmp(lang, "greek") == 0) return "el";
    if (strcmp(lang, "hungarian") == 0) return "hu";
    if (strcmp(lang, "italian") == 0) return "it";
    if (strcmp(lang, "japanese") == 0) return "ja";
    if (strcmp(lang, "koreana") == 0) return "ko";
    if (strcmp(lang, "norwegian") == 0) return "no";
    if (strcmp(lang, "polish") == 0) return "pl";
    if (strcmp(lang, "portuguese") == 0) return "pt-PT";
    if (strcmp(lang, "brazilian") == 0) return "pt-BR";
    if (strcmp(lang, "romanian") == 0) return "ro";
    if (strcmp(lang, "russian") == 0) return "ru";
    if (strcmp(lang, "spanish") == 0) return "es";
    if (strcmp(lang, "swedish") == 0) return "sv";
    if (strcmp(lang, "thai") == 0) return "th";
    if (strcmp(lang, "turkish") == 0) return "tr";
    if (strcmp(lang, "ukrainian") == 0) return "uk";
    return "en";
}

#endif
