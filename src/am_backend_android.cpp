#include "amulet.h"

#ifdef AM_BACKEND_ANDROID

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

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
static void android_teardown();

static void android_init_engine() {
    // First call am_destroy_gl in case the gl context was lost and we are re-initializing.
    // This will prevent any glDelete* function finalizers from being called
    // (Android would have already deleted all GL objects).
    am_destroy_gl();
    android_teardown();

    am_debug("%s", "android_init_engine");
    android_running = false;

    am_opt_main_module = "main";
    am_debug("%s", "load config");
    if (!am_load_config()) return;
    am_debug("%s", "init engine");
    android_eng = am_init_engine(false, 0, NULL);
    if (android_eng == NULL) return;

    am_debug("%s", "call main");
    frame_time = am_get_current_time();
    lua_pushcclosure(android_eng->L, am_require, 0);
    lua_pushstring(android_eng->L, am_opt_main_module);
    if (am_call(android_eng->L, 1, 0)) {
        android_running = true;
    }
    am_debug("%s", "call main");
    t0 = am_get_current_time();
    frame_time = t0;
    t_debt = 0.0;

    static bool audio_initialized = false;
    if (!audio_initialized) {
        am_debug("%s", "init audio");
        // only initialize audio once (android_init_engine maybe be called again if surface recreated)
        android_init_audio();
        audio_initialized = true;
    }
    am_debug("%s", "done");
}

static void android_teardown() {
    android_running = false;
    if (android_eng != NULL) {
        if (android_eng->L != NULL) {
            am_destroy_engine(android_eng);
        }
        android_eng = NULL;
    }
    if (am_gl_is_initialized()) {
        am_destroy_gl();
    }
    android_window_created = false;
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

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    am_debug("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        am_debug("after %s() glError (0x%x)\n", op, error);
    }
}

static const char* gVertexShader =
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";

static const char* gFragmentShader =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    am_debug("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    am_debug("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram = 0;
GLuint gvPositionHandle;

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    am_debug("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        am_debug("%s", "Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    am_debug("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

void renderFrame() {
    static float grey;
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");
}

typedef void (*AndroidAudioCallback)(short *buffer, int num_samples);

static void audio_cb(short *buffer, int num_samples);
static bool OpenSLWrap_Init(AndroidAudioCallback cb);

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
};

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniResize(JNIEnv * env, jobject obj,  jint width, jint height)
{
    android_win_width = width;
    android_win_height = height;
    //am_debug("resize %d %d", width, height);
    //setupGraphics(width, height);

}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniStep(JNIEnv * env, jobject obj)
{
    android_draw();
    android_update();
    //renderFrame();
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniInit(JNIEnv * env, jobject obj, jobject jassman, jstring jdatadir, jstring jlang)
{
    asset_manager = AAssetManager_fromJava(env, jassman);

    const char* datadir_tmp = env->GetStringUTFChars(jdatadir , NULL ) ;
    android_data_dir = am_format("%s", datadir_tmp);
    env->ReleaseStringUTFChars(jdatadir, datadir_tmp);

    const char* lang_tmp = env->GetStringUTFChars(jlang , NULL ) ;
    android_lang = am_format("%s", lang_tmp);
    env->ReleaseStringUTFChars(jlang, lang_tmp);

    //setupGraphics(android_win_width, android_win_height);
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniSurfaceCreated(JNIEnv * env, jobject obj)
{
    android_init_engine();
}

static void OpenSLWrap_Shutdown();

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTeardown(JNIEnv * env, jobject obj) {
    android_teardown();
    OpenSLWrap_Shutdown();
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchDown(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y) {
    if (android_eng == NULL || android_eng->L == NULL) return;
    am_window *win = am_find_window((am_native_window*)&win_dummy);
    if (win == NULL) return;
    win->touch_begin(android_eng->L, (void*)id, x, y, 1.0);
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchUp(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y) {
    if (android_eng == NULL || android_eng->L == NULL) return;
    am_window *win = am_find_window((am_native_window*)&win_dummy);
    if (win == NULL) return;
    win->touch_end(android_eng->L, (void*)id, x, y, 1.0);
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniTouchMove(JNIEnv * env, jobject obj, jint id, jfloat x, jfloat y) {
    if (android_eng == NULL || android_eng->L == NULL) return;
    am_window *win = am_find_window((am_native_window*)&win_dummy);
    if (win == NULL) return;
    win->touch_move(android_eng->L, (void*)id, x, y, 1.0);
}

//------------------------------

static float *android_audio_buffer = NULL;

static void android_init_audio() {
    android_audio_buffer = (float*)malloc(am_conf_audio_channels * am_conf_audio_buffer_size * sizeof(float));
    OpenSLWrap_Init(audio_cb);
}

void android_audio_teardown() {
}

#define BUFFER_SIZE 2048
#define BUFFER_SIZE_IN_SAMPLES (BUFFER_SIZE / 2)
#define FREQ 44100

static void audio_cb(short *buffer, int n_samples) {
    /*
    static double t = 0.0;
    //am_debug("audio_cb %d", num_samples);
    for (int i = 0; i < num_samples; i+=1) {
        buffer[i*2] = (short)(sin(2.0 * 3.14 * t * 440.0) * 30000.0);
        //buffer[i * BUFFER_SIZE] = 0;
        t += 1.0 / (double)FREQ;
    }
    */

    int num_channels = am_conf_audio_channels;
    int num_samples = am_conf_audio_buffer_size;
    memset(android_audio_buffer, 0, num_channels * num_samples * sizeof(float));
    am_audio_bus bus(num_channels, num_samples, android_audio_buffer);
    pthread_mutex_lock(&android_audio_mutex);
    am_fill_audio_bus(&bus);
    pthread_mutex_unlock(&android_audio_mutex);
    am_interleave_audio16(buffer, android_audio_buffer, num_channels, num_samples, 0, num_samples);
    double mx = 0.0;
    for (int i = 0; i < 2048; i++) {
        if (android_audio_buffer[i] > mx) mx = android_audio_buffer[i];
    }
    //am_debug("audio_callback %g", mx);
}

// engine interfaces
static SLObjectItf engineObject;
static SLEngineItf engineEngine;
static SLObjectItf outputMixObject;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;

static short buffer[BUFFER_SIZE];

static AndroidAudioCallback audioCallback;

// This callback handler is called every time a buffer finishes playing.
// The documentation available is very unclear about how to best manage buffers.
// I've chosen to this approach: Instantly enqueue a buffer that was rendered to the last time,
// and then render the next. Hopefully it's okay to spend time in this callback after having enqueued. 
static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
  assert(bq == bqPlayerBufferQueue);
  assert(NULL == context);

  int nextSize = sizeof(buffer);

  audioCallback(buffer, BUFFER_SIZE_IN_SAMPLES);
  SLresult result;
  result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, nextSize);

  // Comment from sample code:
  // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
  // which for this code example would indicate a programming error
  assert(SL_RESULT_SUCCESS == result);
}

// create the engine and output mix objects
//extern "C"
static bool OpenSLWrap_Init(AndroidAudioCallback cb) {
  audioCallback = cb;

  SLresult result;
  // create engine
  result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
  assert(SL_RESULT_SUCCESS == result);
  result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
  assert(SL_RESULT_SUCCESS == result);
  result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
  assert(SL_RESULT_SUCCESS == result);
  result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);

  SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
  SLDataFormat_PCM format_pcm = {
    SL_DATAFORMAT_PCM,
    2,
    SL_SAMPLINGRATE_44_1,
    SL_PCMSAMPLEFORMAT_FIXED_16,
    SL_PCMSAMPLEFORMAT_FIXED_16,
    SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
    SL_BYTEORDER_LITTLEENDIAN
  };

  SLDataSource audioSrc = {&loc_bufq, &format_pcm};

  // configure audio sink
  SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
  SLDataSink audioSnk = {&loc_outmix, NULL};

  // create audio player
  const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
  const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
  result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 2, ids, req);
  assert(SL_RESULT_SUCCESS == result);

  result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
    &bqPlayerBufferQueue);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
  assert(SL_RESULT_SUCCESS == result);
  result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
  assert(SL_RESULT_SUCCESS == result);

  // Render and enqueue a first buffer. (or should we just play the buffer empty?)
  audioCallback(buffer, BUFFER_SIZE_IN_SAMPLES);

  result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, sizeof(buffer));
  if (SL_RESULT_SUCCESS != result) {
    return false;
  }
  return true;
}

// shut down the native audio system
//extern "C"
static void OpenSLWrap_Shutdown() {
  if (bqPlayerObject != NULL) {
    (*bqPlayerObject)->Destroy(bqPlayerObject);
    bqPlayerObject = NULL;
    bqPlayerPlay = NULL;
    bqPlayerBufferQueue = NULL;
    bqPlayerMuteSolo = NULL;
    bqPlayerVolume = NULL;
  }
  if (outputMixObject != NULL) {
    (*outputMixObject)->Destroy(outputMixObject);
    outputMixObject = NULL;
  }
  if (engineObject != NULL) {
    (*engineObject)->Destroy(engineObject);
    engineObject = NULL;
    engineEngine = NULL;
  }
}


JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniPause(JNIEnv * env, jobject obj)
{
    if (bqPlayerObject != NULL) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    }
}

JNIEXPORT void JNICALL Java_xyz_amulet_AmuletActivity_jniResume(JNIEnv * env, jobject obj)
{
    if (bqPlayerObject != NULL) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    }
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
        am_log0("%s", "attempt to create two iOS windows");
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

#endif // AM_BACKEND_ANDROID
