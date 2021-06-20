#include "amulet.h"

#ifdef AM_BACKEND_ANDROID

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

static AAssetManager *asset_manager = NULL;
static am_engine *android_eng = NULL;
static bool android_window_created = false;
static bool android_running = false;
static int android_win_width = 480;
static int android_win_height = 705;
static char *android_data_dir = NULL;
static char *android_lang = NULL;
static pthread_mutex_t android_audio_mutex;

static int win_dummy = 0;

static double t0;
static double t_debt;
static double frame_time = 0.0;
static double delta_time;
static double real_delta_time;

static void android_init_audio();
static void android_audio_teardown();
static void android_teardown();

static JNIEnv *jni_env = NULL;

static void android_init_engine() {
    // First call am_destroy_gl in case the gl context was lost and we are re-initializing.
    // This will prevent any glDelete* function finalizers from being called
    // (Android would have already deleted all GL objects).
    am_destroy_gl();
    android_teardown();

    am_debug("%s", "android_init_engine");

    am_opt_main_module = "main";
    if (!am_load_config()) return;
    android_eng = am_init_engine(false, 0, NULL);
    if (android_eng == NULL) return;

    frame_time = am_get_current_time();
    lua_pushcclosure(android_eng->L, am_require, 0);
    lua_pushstring(android_eng->L, am_opt_main_module);
    if (am_call(android_eng->L, 1, 0)) {
        android_running = true;
    }
    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;

    android_init_audio();
}

static void android_teardown() {
    android_running = false;
    if (android_eng != NULL) {
        if (android_eng->L != NULL) {
            am_destroy_engine(android_eng);
        }
        android_eng = NULL;
    }
    am_destroy_gl();
    android_window_created = false;
    android_audio_teardown();
}

static void android_draw() {
    if (android_eng == NULL) return;
    am_update_windows(android_eng->L);
}

static void android_update() {
    if (!android_running) return;

    pthread_mutex_lock(&android_audio_mutex);
    am_sync_audio_graph(android_eng->L);
    pthread_mutex_unlock(&android_audio_mutex);

    frame_time = am_get_current_time();
    
    real_delta_time = frame_time - t0;
    if (am_conf_warn_delta_time > 0.0 && real_delta_time > am_conf_warn_delta_time) {
        am_log0("WARNING: FPS dropped to %0.2f (%fs)", 1.0/real_delta_time, real_delta_time);
    }
    // take min in case app paused, or last frame took very long
    delta_time = am_min(am_conf_max_delta_time, real_delta_time); 
    t_debt += delta_time;

    if (am_conf_fixed_delta_time > 0.0) {
        while (t_debt > 0.0) {
            if (!am_execute_actions(android_eng->L, am_conf_fixed_delta_time)) {
                android_running = false;
                return;
            }
            t_debt -= am_conf_fixed_delta_time;
        }
    } else {
        if (t_debt > am_conf_min_delta_time) {
            if (!am_execute_actions(android_eng->L, t_debt)) {
                android_running = false;
                return;
            }
            t_debt = 0.0;
        }
    }

    t0 = frame_time;
}

extern "C" {
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniResize(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniStep(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniInit(JNIEnv * env, jobject obj, jobject jassman, jstring jdatadir, jstring jlang);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniSurfaceCreated(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTeardown(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniPause(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniResume(JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchDown(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchUp(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchMove(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniIAPProductsRetrieved(JNIEnv * env, jobject obj, jint success, jobjectArray productIds, jobjectArray prices);
    JNIEXPORT jint JNICALL Java_xyz_amulet_AmuletActivity_jniIAPTransactionUpdated(JNIEnv * env, jobject obj, jstring jproductId, jstring jstatus);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniIAPRestoreFinished(JNIEnv * env, jobject obj, jint success);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniSetGameServicesConnected(JNIEnv * env, jobject obj, jint c);
    JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniFillAudioBuffer(JNIEnv * env, jobject obj, jfloatArray jbuffer, jint size);
};

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniResize(JNIEnv * env, jobject obj,  jint width, jint height)
{
    android_win_width = width;
    android_win_height = height;

}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniStep(JNIEnv * env, jobject obj)
{
    jni_env = env;
    android_draw();
    android_update();
    jni_env = NULL;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniInit(JNIEnv * env, jobject obj, jobject jassman, jstring jdatadir, jstring jlang)
{
    jni_env = env;
    pthread_mutex_init(&android_audio_mutex, NULL);

    asset_manager = AAssetManager_fromJava(env, jassman);

    const char* datadir_tmp = env->GetStringUTFChars(jdatadir , NULL ) ;
    android_data_dir = am_format("%s", datadir_tmp);
    env->ReleaseStringUTFChars(jdatadir, datadir_tmp);

    const char* lang_tmp = env->GetStringUTFChars(jlang , NULL ) ;
    android_lang = am_format("%s", lang_tmp);
    env->ReleaseStringUTFChars(jlang, lang_tmp);
    jni_env = NULL;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniSurfaceCreated(JNIEnv * env, jobject obj)
{
    jni_env = env;
    android_init_engine();
    jni_env = NULL;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTeardown(JNIEnv * env, jobject obj) {
    android_teardown();
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchDown(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y) {
    jni_env = env;
    if (android_eng == NULL || android_eng->L == NULL) return;
    am_window *win = am_find_window((am_native_window*)&win_dummy);
    if (win == NULL) return;
    win->touch_begin(android_eng->L, (void*)((uintptr_t)id), x, y, 1.0);
    jni_env = NULL;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchUp(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y) {
    jni_env = env;
    if (android_eng == NULL || android_eng->L == NULL) return;
    am_window *win = am_find_window((am_native_window*)&win_dummy);
    if (win == NULL) return;
    win->touch_end(android_eng->L, (void*)((uintptr_t)id), x, y, 1.0);
    jni_env = NULL;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchMove(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y) {
    jni_env = env;
    if (android_eng == NULL || android_eng->L == NULL) return;
    am_window *win = am_find_window((am_native_window*)&win_dummy);
    if (win == NULL) return;
    win->touch_move(android_eng->L, (void*)((uintptr_t)id), x, y, 1.0);
    jni_env = NULL;
}

struct am_iap_product : am_nonatomic_userdata {
    char *product_id;
    char *price;
};

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniIAPProductsRetrieved(JNIEnv * env, jobject obj, jint success, jobjectArray productIds, jobjectArray prices) {
    jni_env = env;
    if (android_eng != NULL && android_eng->L != NULL) {
        lua_State *L = android_eng->L;
        if (success) {
            lua_newtable(L);
            int n = env->GetArrayLength(productIds);
            for (int i = 0; i < n; i++) {
                jstring jpid = (jstring) (env->GetObjectArrayElement(productIds, i));
                jstring jprice = (jstring) (env->GetObjectArrayElement(prices, i));
                const char* pid = env->GetStringUTFChars(jpid , NULL ) ;
                const char* price = env->GetStringUTFChars(jprice , NULL ) ;
                lua_pushstring(L, pid);
                am_iap_product *product = am_new_userdata(L, am_iap_product);
                product->product_id = am_format("%s", pid);
                product->price = am_format("%s", price);
                env->ReleaseStringUTFChars(jpid, pid);
                env->ReleaseStringUTFChars(jprice, price);
                env->DeleteLocalRef(jpid);
                env->DeleteLocalRef(jprice);
                lua_settable(L, -3);
            } 
        } else {
            lua_pushnil(L);
        }
        am_call_amulet(L, "_iap_retrieve_products_finished", 1, 0);
    }
    jni_env = NULL;
}

JNIEXPORT jint JNICALL Java_xyz_amulet_AmuletActivity_jniIAPTransactionUpdated(JNIEnv * env, jobject obj, jstring jpid, jstring jstatus) {
    jni_env = env;
    jint res = 0;
    if (android_eng != NULL && android_eng->L != NULL) {
        lua_State *L = android_eng->L;
        const char* pid = env->GetStringUTFChars(jpid , NULL ) ;
        const char* status = env->GetStringUTFChars(jstatus , NULL ) ;
        lua_pushstring(L, pid);
        lua_pushstring(L, status);
        env->ReleaseStringUTFChars(jpid, pid);
        env->ReleaseStringUTFChars(jstatus, status);
        if (am_call_amulet(L, "_iap_transaction_updated", 2, 1)) {
            res = (jint)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    jni_env = NULL;
    return res;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniIAPRestoreFinished(JNIEnv * env, jobject obj, jint success) {
    jni_env = env;
    if (android_eng != NULL && android_eng->L != NULL) {
        lua_State *L = android_eng->L;
        lua_pushboolean(L, success);
        am_call_amulet(L, "_iap_restore_finished", 1, 0);
    }
    jni_env = NULL;
}

//------------------------------

static float *android_audio_buffer = NULL;
static int android_audio_buffer_size = 0;
static bool audio_initialized = false;
static bool audio_is_paused = false;

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniFillAudioBuffer(JNIEnv * env, jobject obj, jfloatArray jbuffer, jint size) {
    if (!audio_initialized) return;
    if (android_audio_buffer_size != size) {
        if (android_audio_buffer != NULL) {
            free(android_audio_buffer);
        }
        android_audio_buffer = (float*)malloc(size * sizeof(float));
        android_audio_buffer_size = size;
    }
    jfloat *buffer = env->GetFloatArrayElements(jbuffer, 0);
    memset(android_audio_buffer, 0, size * sizeof(float));
    if (!audio_is_paused) {
        am_audio_bus bus(2, size / 2, android_audio_buffer);
        pthread_mutex_lock(&android_audio_mutex);
        am_fill_audio_bus(&bus);
        pthread_mutex_unlock(&android_audio_mutex);
    }
    am_interleave_audio(buffer, android_audio_buffer, 2, size / 2, 0, size / 2);
    env->ReleaseFloatArrayElements(jbuffer, buffer, 0);
}

static void android_init_audio() {
    audio_initialized = true;
}

static void android_audio_teardown() {
    audio_initialized = false;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniPause(JNIEnv * env, jobject obj)
{
    audio_is_paused = true;
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniResume(JNIEnv * env, jobject obj)
{
    audio_is_paused = false;
}

//-----------------------------------------------------------------

am_native_window *am_create_native_window(
    am_window_mode mode,
    am_display_orientation orientation,
    int top, int left,
    int width, int height,
    const char *title,
    bool highdpi,
    bool resizable,
    bool borderless,
    bool depth_buffer,
    bool stencil_buffer,
    int msaa_samples)
{
    if (android_window_created) {
        am_log0("%s", "attempt to create two Android windows");
        return NULL;
    }
    android_window_created = true;
    if (!am_gl_is_initialized()) {
        am_init_gl();
    }
    return (am_native_window*)&win_dummy;
}

void am_get_native_window_size(am_native_window *window, int *pw, int *ph, int *sw, int *sh) {
    *pw = android_win_width;
    *ph = android_win_height;
    *sw = android_win_width;
    *sh = android_win_height;
}

void am_get_native_window_safe_area_margin(am_native_window *window, 
    int *left, int *right, int *bottom, int *top)
{
    *left = 0;
    *right = 0;
    *bottom = 0;
    *top = 0;
}

bool am_set_native_window_size_and_mode(am_native_window *window, int w, int h, am_window_mode mode) {
    return false;
}

bool am_get_native_window_lock_pointer(am_native_window *window) {
    return false;
}

void am_set_native_window_lock_pointer(am_native_window *window, bool lock) {
}

bool am_get_native_window_show_cursor(am_native_window *window) {
    return false;
}

void am_set_native_window_show_cursor(am_native_window *window, bool show) {
}

void am_destroy_native_window(am_native_window *window) {
}

void am_native_window_bind_framebuffer(am_native_window *win) {
    am_bind_framebuffer(0); // XXX Not sure if default framebuffer is guaranteed to be zero.
}

void am_native_window_swap_buffers(am_native_window *window) {
    // handled by Android, I think
}

double am_get_current_time() {
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    double t = 0.000000001 * (double)tm.tv_nsec + (double)tm.tv_sec;
    return t;
}

void *am_read_resource(const char *filename, int *len, char** errmsg) {
    *errmsg = NULL;
    if (asset_manager == NULL) {
        *errmsg = am_format("%s", "asset manager is null");
        return NULL;
    }
    AAsset* asset = AAssetManager_open(asset_manager, filename, AASSET_MODE_STREAMING);
    if (asset == NULL) {
        *errmsg = am_format("unable to open resource %s", filename);
        return NULL;
    }
    *len = (int)AAsset_getLength(asset);
    void* buf = malloc(*len);
    if (buf == NULL) {
        *errmsg = am_format("insufficient memory to read resource %s", filename);
        AAsset_close(asset);
        return NULL;
    }
    int res = AAsset_read(asset, buf, *len);
    if (res < 0) {
        *errmsg = am_format("error reading resource %s", filename);
        AAsset_close(asset);
        return NULL;
    }
    if (res < *len) {
        *errmsg = am_format("unable to read all of %s in one go", filename);
        AAsset_close(asset);
        return NULL;
    }
    AAsset_close(asset);
    return buf;
}

int am_next_video_capture_frame() {
    return 0;
}

void am_copy_video_frame_to_texture() {
}

void am_capture_audio(am_audio_bus *bus) {
}

char *am_get_base_path() {
    return am_format("%s", ".");
}

char *am_get_data_path() {
    return am_format("%s", android_data_dir);
}

const char *am_preferred_language() {
    return android_lang;
}

char *am_open_file_dialog() {
    return NULL;
}

static int retrieve_iap_products(lua_State *L) {
    am_check_nargs(L, 1);
    if (!lua_istable(L, 1)) return luaL_error(L, "expecting a table in position 1");
    int n = lua_objlen(L, 1);
    jclass string_cls = jni_env->FindClass("java/lang/String");
    jobjectArray productIds = jni_env->NewObjectArray(n, string_cls, 0);
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, 1, i);
        const char *pid = lua_tostring(L, -1);
        if (pid == NULL) return luaL_error(L, "all product ids must be strings");
        jstring jpid = jni_env->NewStringUTF(pid);
        jni_env->SetObjectArrayElement(productIds, i-1, jpid);
        jni_env->DeleteLocalRef(jpid);
        lua_pop(L, 1);
    }

    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppGetProducts", "([Ljava/lang/String;)V");
    jni_env->CallStaticVoidMethod(cls, mid, productIds);
    jni_env->DeleteLocalRef(productIds);
    return 0;
}

static int purchase_iap_product(lua_State *L) {
    am_check_nargs(L, 1);
    am_iap_product *product = am_get_userdata(L, am_iap_product, 1);
    jstring jpid = jni_env->NewStringUTF(product->product_id);
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppPurchaseProduct", "(Ljava/lang/String;)V");
    jni_env->CallStaticVoidMethod(cls, mid, jpid);
    jni_env->DeleteLocalRef(jpid);
    return 0;
}

static int restore_iap_purchases(lua_State *L) {
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppRestorePurchases", "()V");
    jni_env->CallStaticVoidMethod(cls, mid);
    return 0;
}

static int can_make_iap_payments(lua_State *L) {
    lua_pushboolean(L, 1);
    return 1;
}

static int iap_product_local_price(lua_State *L) {
    am_check_nargs(L, 1);
    am_iap_product *product = am_get_userdata(L, am_iap_product, 1);
    lua_pushstring(L, product->price);
    return 1;
}

static int iap_product_gc(lua_State *L) {
    am_iap_product *product = am_get_userdata(L, am_iap_product, 1);
    free(product->product_id);
    free(product->price);
    return 0;
}

static int init_google_banner_ad(lua_State *L) {
    am_check_nargs(L, 1);
    const char *unitid = lua_tostring(L, 1);
    if (unitid == NULL) return luaL_error(L, "expecting a string unitid argument");
    jstring junitid = jni_env->NewStringUTF(unitid);
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppInitAds", "(Ljava/lang/String;)V");
    jni_env->CallStaticVoidMethod(cls, mid, junitid);
    jni_env->DeleteLocalRef(junitid);
    return 0;
}

static int set_google_banner_ad_visible(lua_State *L) {
    am_check_nargs(L, 1);
    int vis = lua_toboolean(L, 1);
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppSetAdVisible", "(I)V");
    jni_env->CallStaticVoidMethod(cls, mid, vis);
    return 0;
}

static int is_google_banner_ad_visible(lua_State *L) {
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppIsAdVisible", "()I");
    int vis = jni_env->CallStaticIntMethod(cls, mid);
    lua_pushboolean(L, vis);
    return 1;
}

static int request_google_banner_ad(lua_State *L) {
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppRefreshAd", "()V");
    jni_env->CallStaticVoidMethod(cls, mid);
    return 0;
}

static int get_banner_ad_height(lua_State *L) {
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppGetAdHeight", "()I");
    int h = jni_env->CallStaticIntMethod(cls, mid);
    lua_pushinteger(L, h);
    return 1;
}

static int google_play_connected = 0;

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniSetGameServicesConnected(JNIEnv * env, jobject obj, jint c) {
    google_play_connected = c;
}

static int google_play_is_available(lua_State *L) {
    lua_pushboolean(L, google_play_connected);
    return 1;
}

static int init_google_play_services(lua_State *L) {
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppInitGoogleApiClient", "()V");
    jni_env->CallStaticVoidMethod(cls, mid);
    return 0;
}

static int show_google_play_leaderboard(lua_State *L) {
    am_check_nargs(L, 1);
    const char *leaderboard = lua_tostring(L, 1);
    if (leaderboard == NULL) return luaL_error(L, "expecting a string in position 1");
    jstring jleaderboard = jni_env->NewStringUTF(leaderboard);
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppShowLeaderboard", "(Ljava/lang/String;)V");
    jni_env->CallStaticVoidMethod(cls, mid, jleaderboard);
    jni_env->DeleteLocalRef(jleaderboard);
    return 0;
}

static int submit_google_play_score(lua_State *L) {
    am_check_nargs(L, 2);
    const char *leaderboard = lua_tostring(L, 1);
    if (leaderboard == NULL) return luaL_error(L, "expecting a string in position 1");
    jlong jscore = (jlong)lua_tonumber(L, 2);
    jstring jleaderboard = jni_env->NewStringUTF(leaderboard);
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppSubmitLeaderboardScore", "(Ljava/lang/String;J)V");
    jni_env->CallStaticVoidMethod(cls, mid, jleaderboard, jscore);
    jni_env->DeleteLocalRef(jleaderboard);
    return 0;
}

static int submit_google_play_achievement(lua_State *L) {
    am_check_nargs(L, 1);
    const char *id = lua_tostring(L, 1);
    if (id == NULL) return luaL_error(L, "expecting a string in position 1");
    jstring jid = jni_env->NewStringUTF(id);
    jclass cls = jni_env->FindClass("xyz/amulet/AmuletActivity");
    jmethodID mid = jni_env->GetStaticMethodID(cls, "cppSubmitAchievement", "(Ljava/lang/String;)V");
    jni_env->CallStaticVoidMethod(cls, mid, jid);
    jni_env->DeleteLocalRef(jid);
    return 0;
}

static void register_iap_product_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);

    lua_pushcclosure(L, iap_product_gc, 0);
    lua_setfield(L, -2, "__gc");

    am_register_metatable(L, "iap_product", MT_am_iap_product, 0);
}

lua_State *am_get_global_lua_state() {
    if (android_eng != NULL) {
        return android_eng->L;
    }
    return NULL;
}

void am_open_android_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"init_google_play_services", init_google_play_services},
        {"google_play_available", google_play_is_available},
        {"show_google_play_leaderboard", show_google_play_leaderboard},
        {"submit_google_play_score", submit_google_play_score},
        {"submit_google_play_achievement", submit_google_play_achievement},

        {"init_google_banner_ad", init_google_banner_ad},
        {"set_google_banner_ad_visible", set_google_banner_ad_visible},
        {"is_google_banner_ad_visible", is_google_banner_ad_visible},
        {"request_google_banner_ad", request_google_banner_ad},
        {"get_banner_ad_height", get_banner_ad_height},

        {"retrieve_iap_products", retrieve_iap_products},
        {"purchase_iap_product", purchase_iap_product},
        {"restore_iap_purchases", restore_iap_purchases},
        {"can_make_iap_payments", can_make_iap_payments},
        {"iap_product_local_price", iap_product_local_price},

        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_iap_product_mt(L);
}

#endif // AM_BACKEND_ANDROID
