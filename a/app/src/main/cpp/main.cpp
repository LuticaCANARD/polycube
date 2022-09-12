/*
* Copyright (C) 2010 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

inline int main2();
//BEGIN_INCLUDE(all)
struct touchdata {
    int flag;//0:disable 1:down 2:move 3:up
    float x;
    float y;
    float oldx;
    float oldy;
    float setx;
    float sety;
};
touchdata touchs[40];
int touchcount;
int activitysizex, activitysizey;
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <errno.h>
#include <cassert>
#include <time.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include "img.h"
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <stdio.h>
#include <math.h>
#define ilog(x) (9-(int)log10((double)(x>0?x:1)))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
void LoadGLTextures();
inline int GetTickCount2(void) {
    timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000 * res.tv_sec + (res.tv_nsec / 1000000);

}
inline int getdatep(){
    tm *tm;
    timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    tm=localtime(&res.tv_sec);
    long long temp=(tm->tm_mday) + (tm->tm_mon)*32+(tm->tm_year)*366;
    temp=temp*(temp%5)-temp%10;
    temp = (temp*temp*125124524213+231)/5+(temp%20);
    return temp%21;
}
inline int gethour(){

    tm *tm;
    timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    tm=localtime(&res.tv_sec);

    return (tm->tm_hour)%12;
}
inline int getminute(){
    tm *tm;
    timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    tm=localtime(&res.tv_sec);

    return tm->tm_min;
}
/**
* Our saved state data.
*/
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
* Shared state for our app.
*/
long long oscore = 0;
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

/**
* Initialize an EGL context for the current display.
*/

static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL

    /*
	* Here specify the attributes of the desired configuration.
	* Below, we select an EGLConfig with at least 8 bits per color
	* component compatible with on-screen windows
	*/
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE,24,
            EGL_NONE
    };
    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    activitysizey = h;
    activitysizex = w;
    // Check openGL on the system
    auto opengl_info = { GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS };
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }
    // Initialize GL state.
    // glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glClearDepthf(1.0f);
    //glEnable(GL_CULL_FACE);glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    //  glCullFace(GL_FRONT);

    //  glShadeModel(GL_SMOOTH);
    //   glViewport(0,0,activitysizex,activitysizey);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-1.5, 1.5, -1.5*h / w, 1.5*h / w, 0.96, 20.);




    // glOrthof(-1.5,1.5,-1.5*activitysizey/activitysizex,1.5*activitysizey/activitysizex,-45.,45.);

    glMatrixMode(GL_MODELVIEW);
    //   glLoadIdentity();

    // / glDisable(GL_DEPTH_TEST);
    LoadGLTextures();
    return 0;
}

/**
* Just the current frame in the display.
*/
engine *enginetmp;
void eglSwapBuffers() {
    eglSwapBuffers(enginetmp->display, enginetmp->surface);
}
static void engine_draw_frame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return;
    }
    enginetmp = engine;

    main2();
}

/**
* Tear down the EGL context currently associated with the display.
*/
static void engine_term_display(struct engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
* Process the next input event.
*/
int asc;
int ptridt;
int ptrcount;
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine *engine = (struct engine *) app->userData;
    activitysizex = engine->width;
    activitysizey = engine->height;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = (int)AMotionEvent_getX(event, 0);
        engine->state.y = (int)AMotionEvent_getY(event, 0);

        int tempv;

        int ac = AMotionEvent_getAction(event);
        ptridt = ptrcount;
        ptrcount = (int)AMotionEvent_getPointerCount(event);
        switch (ac & 0x00FF) {
            case AMOTION_EVENT_ACTION_DOWN:
                tempv = AMotionEvent_getPointerId(event, 0);
                touchs[tempv].flag = 1;
                touchs[tempv].x = (AMotionEvent_getX(event, 0) / activitysizex*3. - 1.5) / 2.095;
                touchs[tempv].y = -((AMotionEvent_getY(event, 0) + activitysizey / 40) / activitysizey*3. - 1.5)*activitysizey / activitysizex / 2.095;
                touchs[tempv].oldx = touchs[tempv].x;
                touchs[tempv].oldy = touchs[tempv].y;
                touchs[tempv].setx = touchs[tempv].x;
                touchs[tempv].sety = touchs[tempv].y;
                break;
            case AMOTION_EVENT_ACTION_UP:
                tempv = AMotionEvent_getPointerId(event, 0);
                if (touchs[tempv].flag == 2)touchs[tempv].flag = 3;
                else if (touchs[tempv].flag == 1) touchs[tempv].flag = 0;
                touchs[tempv].oldx = touchs[tempv].x;
                touchs[tempv].oldy = touchs[tempv].y;
                touchs[tempv].x = (AMotionEvent_getX(event, 0) / activitysizex*3. - 1.5) / 2.095;
                touchs[tempv].y = -((AMotionEvent_getY(event, 0) + activitysizey / 40) / activitysizey*3. - 1.5)*activitysizey / activitysizex / 2.095;

                break;
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
            {

                for (size_t i = 0; i < ptrcount; i++) {
                    tempv = AMotionEvent_getPointerId(event, i);
                    touchs[tempv].oldx = touchs[tempv].x;
                    touchs[tempv].oldy = touchs[tempv].y;
                    touchs[tempv].x = (AMotionEvent_getX(event, i) / activitysizex*3. - 1.5) / 2.095;
                    touchs[tempv].y = -((AMotionEvent_getY(event, i) + activitysizey / 40) / activitysizey*3. - 1.5)*activitysizey / activitysizex / 2.095;

                }
                // tempv = AMotionEvent_getPointerId(event, ptrcount - 1);
                touchs[tempv].oldx = touchs[tempv].x;
                touchs[tempv].oldy = touchs[tempv].y;
                touchs[tempv].setx = touchs[tempv].x;
                touchs[tempv].sety = touchs[tempv].y;
                touchs[tempv].flag = 1;
                // LOGI("%d ",tempv);
            }
                break;
            case AMOTION_EVENT_ACTION_POINTER_UP:


                break;
            case AMOTION_EVENT_ACTION_MOVE: {

                for (size_t i = 0; i < ptrcount; i++) {

                    tempv = AMotionEvent_getPointerId(event, i);
                    touchs[tempv].oldx = touchs[tempv].x;
                    touchs[tempv].oldy = touchs[tempv].y;
                    touchs[tempv].x = (AMotionEvent_getX(event, i) / activitysizex*3. - 1.5) / 2.095;
                    touchs[tempv].y = -((AMotionEvent_getY(event, i) + activitysizey / 40) / activitysizey*3. - 1.5)*activitysizey / activitysizex / 2.095;

                }

            }
                break;



        }
        if (ptridt>ptrcount) {
            for (size_t i = 0; i < ptrcount; i++) {

                tempv = AMotionEvent_getPointerId(event, i);
                touchs[tempv].oldx = touchs[tempv].x;
                touchs[tempv].oldy = touchs[tempv].y;
                touchs[tempv].x = (AMotionEvent_getX(event, i) / activitysizex*3. - 1.5) / 2.095;
                touchs[tempv].y = -((AMotionEvent_getY(event, i) + activitysizey / 40) / activitysizey*3. - 1.5)*activitysizey / activitysizex / 2.095;
                touchs[tempv].flag += 20;
            }
            for (size_t i = 0; i < 40; i++) {
                if (touchs[i].flag != 0 && touchs[i].flag < 20) {
                    if (touchs[i].flag == 1) touchs[i].flag = 0;
                    else if (touchs[i].flag == 2) touchs[i].flag = 3;
                }
                else {
                    if (touchs[i].flag > 19) touchs[i].flag -= 20;
                }
            }
        }
    }

    return 0;
}

/**
* Process the next main2 command.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L / 60) * 1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}

/*
* AcquireASensorManagerInstance(void)
*    Workaround ASensorManager_getInstance() deprecation false alarm
*    for Android-N and before, when compiling with NDK-r15
*/
#include <dlfcn.h>
ASensorManager* AcquireASensorManagerInstance(android_app* app) {

    if (!app)
        return nullptr;

    typedef ASensorManager *(*PF_GETINSTANCEFORPACKAGE)(const char *name);
    void* androidHandle = dlopen("libandroid.so", RTLD_NOW);
    PF_GETINSTANCEFORPACKAGE getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE)
            dlsym(androidHandle, "ASensorManager_getInstanceForPackage");
    if (getInstanceForPackageFunc) {
        JNIEnv* env = nullptr;
        app->activity->vm->AttachCurrentThread(&env, NULL);

        jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
        jmethodID midGetPackageName = env->GetMethodID(android_content_Context,
                                                       "getPackageName",
                                                       "()Ljava/lang/String;");
        jstring packageName = (jstring)env->CallObjectMethod(app->activity->clazz,
                                                             midGetPackageName);

        const char *nativePackageName = env->GetStringUTFChars(packageName, 0);
        ASensorManager* mgr = getInstanceForPackageFunc(nativePackageName);
        env->ReleaseStringUTFChars(packageName, nativePackageName);
        app->activity->vm->DetachCurrentThread();
        if (mgr) {
            dlclose(androidHandle);
            return mgr;
        }
    }

    typedef ASensorManager *(*PF_GETINSTANCE)();
    PF_GETINSTANCE getInstanceFunc = (PF_GETINSTANCE)
            dlsym(androidHandle, "ASensorManager_getInstance");
    // by all means at this point, ASensorManager_getInstance should be available
    assert(getInstanceFunc);
    dlclose(androidHandle);

    return getInstanceFunc();
}


/**
* This is the main2 entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
int setup();
char dir[100];
void android_main(struct android_app* state) {
    for (int i = 0;i<40;i++) {
        touchs[i].flag = 0;
    }


    struct engine engine;

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = AcquireASensorManagerInstance(state);
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(
            engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(
            engine.sensorManager,
            state->looper, LOOPER_ID_USER,
            NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }
    sprintf(dir, "%s/c.dat", state->activity->internalDataPath);
    LOGI("%s", dir);

    setup();
    // loop waiting for stuff to do.
    {
        struct engine *engine = (struct engine *) state->userData;
        activitysizex = engine->width;
        activitysizey = engine->height;

    }
    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                                        (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {
                        //LOGI("accelerometer: x=%f y=%f z=%f",
                        //   event.acceleration.x, event.acceleration.y,
                        // event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine.state.angle += .01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}
//END_INCLUDE(all)
#define BOOL int
#define TRUE 1
#define FALSE 0
int gltemp;
int glcounter;
float glbuffer[1200];
#define GL_POLYGON GL_TRIANGLE_FAN
#define GL_QUADS GL_TRIANGLE_FAN
#define GL_QUAD_STRIP GL_TRIANGLE_STRIP
#define  glColor3f(r,g,b) glColor4f(r,g,b,1.f)
#define glBegin(i) (glcounter=0,gltemp=i,glEnableClientState(GL_VERTEX_ARRAY))
#define glVertex3f(x,y,z) (glbuffer[glcounter*3]=x,glbuffer[glcounter*3+1]=y,glbuffer[glcounter*3+2]=z,glcounter++)
#define glEnd()  glVertexPointer(3, GL_FLOAT, 0, glbuffer),glDrawArrays(gltemp,0,glcounter),glDisableClientState(GL_VERTEX_ARRAY)
void DrawScene();
void initblock();
inline int main2() {
    DrawScene();
    return 0;

}
inline int setup() {

    initblock();
    return 0;
}
float centerx = 0, centery = 0;
bool pause = false;


long long oh;
bool upd = false;
GLfloat h = 1.0f;
BOOL     bBusy = FALSE;
GLfloat  wAngleY = 6.0f;
GLfloat  wAngleX = 6.0f;
GLfloat  wAngleZ = 0.0f;
float t2 = 0.085;
int bi, ci;
int about = 0;
FILE *f;
int startscreen = true;
int vkspace = false;
int vkspace2 = false;
unsigned char rawblock[57][7][7][7];
unsigned char b[4][7][7][7];
unsigned char(*nowblock)[7][7], (*nextblock)[7][7];
unsigned char (*holdblock)[7][7];
unsigned char blk[7][7][26];
unsigned char rbt[3][2][7][7][7];
int floorz=0;
int blockpos[3];
int blockpostmp[3];
int level;
int monoonly, spinlock,hideblock,hidenext,score2x;
long long score;
int u;
int goverflg = 0;
long long lines;
unsigned int timestamp;
int nowhb,nowib,holdhb,holdib,nexthb,nextib;

//65-97 102 104-105 116-127
//+:103 done
//rev:106 done
//ac 107-115 done
//bmb:98 done
//monofromitem:99-101 done
int move(int pos, int deg);
void createnewblock() {

    for (int x = 0; x < 7; x++)
        for (int y = 0; y < 7; y++)
            for (int z = 0; z < 7; z++)
                b[2][x][y][z] = 0;
    int blkcnt = 0;
    int x = 2, y = 2, z = 2;
    int blockcnt = rand()%32768;
    if (blockcnt > 8192)
        blockcnt = 6;
    else if (blockcnt > 2048)
        blockcnt = 7;
    else if (blockcnt > 512)
        blockcnt = 8;
    else if (blockcnt > 128)
        blockcnt = 9;
    else if (blockcnt > 32)
        blockcnt = 10;
    else if (blockcnt > 8)
        blockcnt = 11;
    else if (blockcnt > 4)
        blockcnt = 12;
    else if (blockcnt > 2)
        blockcnt = 13;
    else blockcnt = 14;
    int xh=0,xl=6,yh=0,yl=6,zh=0,zl=6;
    while (blkcnt<blockcnt) {
        if (b[2][x][y][z] == 0) {
            //("^");
            blkcnt++;
            b[2][x][y][z] = 101 + blockcnt;
            if(x>=xh)xh=x;
            if(x<=xl)xl=x;
            if(y>=yh)yh=y;
            if(y<=yl)yl=y;
            if(z>=zh)zh=z;
            if(z<=zl)zl=z;
        }
        int t = rand() % 6;
        switch (t) {
            case 0:
                x++;
                if (x > 6) x = 6;
                break;
            case 1:
                x--;
                if (x < 0) x = 0;
                break;
            case 2:
                y++;
                if (y > 6) y = 6;
                break;
            case 3:
                y--;
                if (y < 0) y = 0;
                break;
            case 4:
                z++;
                if (z > 6) z = 6;
                break;
            case 5:
                z--;
                if (z < 0) z = 0;
                break;
        }
    }
    xl=(xl+xh)/2-3;
    yl=(yl+yh)/2-3;
    zl=(zl+zh)/2-3;
    for(int x=0;x<7;x++)
        for(int y=0;y<7;y++)
            for(int z=0;z<7;z++)
                if(0<=x+xl&&x+xl<=6&& 0<=y+yl&&y+yl<=6&& 0<=z+zl&&z+zl<=6)rawblock[56][x][y][z]=b[2][x+xl][y+yl][z+zl];
                else rawblock[56][x][y][z]=0;


}

int setnextblock();
void initblock() {
    floorz=0;
nowhb=0,nowib=0,holdhb=0,holdib=0,nexthb=0,nextib=0;
    bi = 0;ci = 0;FILE *f = fopen(dir, "rb");

    long long t[2], v;
    if (f != NULL) {

        fread(t, 8, 2, f);
        v = t[0] * 51231 % 134 + t[0] * 12241 % 142 + t[0] * 1411 % 131 + t[0] * 215 % 13 + t[0] * 2;
        if (v == t[1]) {
            oh = t[0];

        }
        fclose(f);

    }

    //("%d\n", oh);
    timestamp = 0;
    vkspace2 = false;
    vkspace = false;
    lines = 0;
    score = 0;
    level = 1;
    srand(time(0));
    for (int x = 0;x < 7;x++)
        for (int y = 0;y < 7;y++)
            for (int z = 0;z < 26;z++)
                blk[x][y][z] = 0;
    asc=0;
    monoonly = 0;
    spinlock = 0;
    hideblock=0;
    hidenext=0;
    score2x=0;
    nowblock = b[0];
    nextblock = b[1];
    holdblock=b[3];
    for(int x=0;x<7;x++)
        for(int y=0;y<7;y++)
            for(int z=0;z<7;z++)
                holdblock[x][y][z]=0;
    holdblock[3][3][3]=4;


//116 2- 0.25% 82
//117 2+ 0.25% 164
//118 vc 1.25% 574 (4*4)
//119 ac 0.05% 590
//104 mono15 0.20% 657
//120~123 bomb 0.6% 854
//124 3- 0.20% 901
//125 + 0.8% 1163
//91 spinlock 0.2% 1228
//102 up del 0.05% 1244
//126 xz del 0.6% 1441 (3)
//105 yz del 0.6% 1638 (3)
//127 bombcretor 0.02% 1644
//65 +2
//66 hide faling block
//69 erase item
//68 score x2
//70 hide next

    for (int p = 0; p < 56; p++) {
        for (int x = 0; x < 7; x++)
            for (int y = 0; y < 7; y++)
                for (int z = 0; z < 7; z++)
                    rawblock[p][x][y][z] = 0;
        switch (p) {
            //105
            case 0:
                rawblock[p][3][3][3] |= 65;
                break;
            case 1:
                rawblock[p][3][3][4] |= 66;
                rawblock[p][3][3][3] |= 66;
                break;
            case 2:
                rawblock[p][3][3][4] |= 67;
                rawblock[p][3][3][3] |= 67;
                rawblock[p][3][3][2] |= 67;
                break;
            case 3:
                rawblock[p][3][3][4] |= 68;
                rawblock[p][3][3][3] |= 68;
                rawblock[p][4][3][3] |= 68;
                break;
            case 4:
                rawblock[p][3][3][4] |= 69;//I
                rawblock[p][3][3][3] |= 69;
                rawblock[p][3][3][2] |= 69;
                rawblock[p][3][3][1] |= 69;
                break;
            case 5:
                rawblock[p][3][3][4] |= 70;//L/J
                rawblock[p][3][3][3] |= 70;
                rawblock[p][3][3][2] |= 70;
                rawblock[p][4][3][2] |= 70;
                break;
            case 6:
                rawblock[p][3][3][4] |= 71;//T
                rawblock[p][2][3][3] |= 71;
                rawblock[p][3][3][3] |= 71;
                rawblock[p][4][3][3] |= 71;
                break;
            case 7:
                rawblock[p][3][3][4] |= 72;//O
                rawblock[p][3][3][3] |= 72;
                rawblock[p][4][3][3] |= 72;
                rawblock[p][4][3][4] |= 72;
                break;
            case 8:
                rawblock[p][3][3][4] |= 73;//s/z
                rawblock[p][3][3][3] |= 73;
                rawblock[p][4][3][3] |= 73;
                rawblock[p][4][3][2] |= 73;
                break;
            case 9:
                rawblock[p][3][3][3] |= 74;//br
                rawblock[p][4][3][3] |= 74;
                rawblock[p][3][4][3] |= 74;
                rawblock[p][3][3][4] |= 74;
                break;
            case 10:
                rawblock[p][3][3][3] |= 75;//rs
                rawblock[p][4][3][3] |= 75;
                rawblock[p][3][4][3] |= 75;
                rawblock[p][3][4][4] |= 75;
                break;
            case 11:
                rawblock[p][3][3][3] |= 76;//ls
                rawblock[p][2][3][3] |= 76;
                rawblock[p][3][4][3] |= 76;
                rawblock[p][3][4][4] |= 76;
                break;
                //t 29
            case 12:
                rawblock[p][3][3][3] |= 77;//A
                rawblock[p][4][3][3] |= 77;
                rawblock[p][3][3][4] |= 77;
                rawblock[p][4][2][3] |= 77;
                rawblock[p][3][2][4] |= 77;
                break;
            case 13:
                rawblock[p][3][3][4] |= 78;//B
                rawblock[p][3][3][3] |= 78;
                rawblock[p][3][3][2] |= 78;
                rawblock[p][4][3][3] |= 78;
                rawblock[p][4][4][3] |= 78;
                break;
            case 14:
                rawblock[p][2][3][4] |= 79;//C
                rawblock[p][2][3][3] |= 79;
                rawblock[p][2][4][3] |= 79;
                rawblock[p][3][4][3] |= 79;
                rawblock[p][3][4][2] |= 79;
                break;
            case 15:
                rawblock[p][2][3][2] |= 80;//C'
                rawblock[p][2][3][3] |= 80;
                rawblock[p][2][4][3] |= 80;
                rawblock[p][3][4][3] |= 80;
                rawblock[p][3][4][4] |= 80;
                break;
            case 16:
                rawblock[p][3][3][4] |= 81;//E
                rawblock[p][3][3][3] |= 81;
                rawblock[p][2][3][3] |= 81;
                rawblock[p][4][3][3] |= 81;
                rawblock[p][4][2][3] |= 81;
                break;
            case 17:
                rawblock[p][3][3][4] |= 82;//E'
                rawblock[p][3][3][3] |= 82;
                rawblock[p][2][3][3] |= 82;
                rawblock[p][4][3][3] |= 82;
                rawblock[p][4][4][3] |= 82;
                break;
            case 18:
                rawblock[p][3][3][4] |= 83;//F
                rawblock[p][2][3][3] |= 83;
                rawblock[p][3][3][3] |= 83;
                rawblock[p][4][3][3] |= 83;
                rawblock[p][2][3][2] |= 83;
                break;
            case 19:
                rawblock[p][3][3][4] |= 84;//H
                rawblock[p][3][3][3] |= 84;
                rawblock[p][4][3][3] |= 84;
                rawblock[p][4][3][2] |= 84;
                rawblock[p][4][4][2] |= 84;
                break;
            case 20:
                rawblock[p][3][3][4] |= 85;//H'
                rawblock[p][3][3][3] |= 85;
                rawblock[p][4][3][3] |= 85;
                rawblock[p][4][3][2] |= 85;
                rawblock[p][4][2][2] |= 85;
                break;
            case 21:
                rawblock[p][3][3][5] |= 86;//I
                rawblock[p][3][3][4] |= 86;
                rawblock[p][3][3][3] |= 86;
                rawblock[p][3][3][2] |= 86;
                rawblock[p][3][3][1] |= 86;
                break;
            case 22:
                rawblock[p][4][3][4] |= 87;//J
                rawblock[p][3][3][4] |= 87;
                rawblock[p][3][3][3] |= 87;
                rawblock[p][3][3][2] |= 87;
                rawblock[p][4][2][4] |= 87;
                break;
            case 23:
                rawblock[p][4][3][4] |= 88;//J'
                rawblock[p][3][3][4] |= 88;
                rawblock[p][3][3][3] |= 88;
                rawblock[p][3][3][2] |= 88;
                rawblock[p][4][4][4] |= 88;
                break;
            case 24:
                rawblock[p][3][3][3] |= 89;//K
                rawblock[p][3][3][4] |= 89;
                rawblock[p][3][3][5] |= 89;
                rawblock[p][3][4][3] |= 89;
                rawblock[p][4][3][3] |= 89;
                break;
            case 25:
                rawblock[p][3][3][5] |= 90;//L
                rawblock[p][3][3][4] |= 90;
                rawblock[p][3][3][3] |= 90;
                rawblock[p][3][3][2] |= 90;
                rawblock[p][4][3][2] |= 90;
                break;
            case 26:
                rawblock[p][3][3][4] |= 97;//M
                rawblock[p][3][3][3] |= 97;
                rawblock[p][3][3][2] |= 97;
                rawblock[p][4][3][3] |= 97;
                rawblock[p][3][4][3] |= 97;
                break;
            case 27:
                rawblock[p][4][3][4] |= 92;//N
                rawblock[p][4][3][3] |= 92;
                rawblock[p][3][3][3] |= 92;
                rawblock[p][3][3][2] |= 92;
                rawblock[p][3][3][1] |= 92;
                break;
            case 28:
                rawblock[p][3][3][3] |= 93;//P
                rawblock[p][4][3][3] |= 93;
                rawblock[p][3][3][4] |= 93;
                rawblock[p][4][3][4] |= 93;
                rawblock[p][3][3][2] |= 93;
                break;
            case 29:
                rawblock[p][3][3][3] |= 94;//Q
                rawblock[p][4][3][3] |= 94;
                rawblock[p][3][3][4] |= 94;
                rawblock[p][4][3][4] |= 94;
                rawblock[p][3][4][3] |= 94;
                break;
            case 30:
                rawblock[p][3][3][4] |= 95;//R
                rawblock[p][3][3][3] |= 95;
                rawblock[p][4][3][3] |= 95;
                rawblock[p][4][3][2] |= 95;
                rawblock[p][3][4][3] |= 95;
                break;
            case 31:
                rawblock[p][3][3][4] |= 96;//R'
                rawblock[p][3][3][3] |= 96;
                rawblock[p][4][3][3] |= 96;
                rawblock[p][4][3][2] |= 96;
                rawblock[p][3][2][3] |= 96;
                break;
            case 32:
                rawblock[p][4][3][4] |= 99;//S
                rawblock[p][4][3][3] |= 99;
                rawblock[p][3][3][3] |= 99;
                rawblock[p][2][3][3] |= 99;
                rawblock[p][2][4][3] |= 99;
                break;
            case 33:
                rawblock[p][4][3][4] |= 100;//S'
                rawblock[p][4][3][3] |= 100;
                rawblock[p][3][3][3] |= 100;
                rawblock[p][2][3][3] |= 100;
                rawblock[p][2][2][3] |= 100;
                break;
            case 34:
                rawblock[p][3][3][3] |= 101;//T
                rawblock[p][3][3][4] |= 101;
                rawblock[p][3][3][5] |= 101;
                rawblock[p][4][3][3] |= 101;
                rawblock[p][2][3][3] |= 101;
                break;
            case 35:
                rawblock[p][3][3][3] |= 3;//U
                rawblock[p][4][3][3] |= 3;
                rawblock[p][2][3][3] |= 3;
                rawblock[p][4][3][2] |= 3;
                rawblock[p][2][3][2] |= 3;
                break;
            case 36:
                rawblock[p][3][3][3] |= 8;//V
                rawblock[p][4][3][3] |= 8;
                rawblock[p][5][3][3] |= 8;
                rawblock[p][3][3][4] |= 8;
                rawblock[p][3][3][5] |= 8;
                break;
            case 37:
                rawblock[p][4][3][4] |= 7;//W
                rawblock[p][4][3][3] |= 7;
                rawblock[p][5][3][3] |= 7;
                rawblock[p][3][3][4] |= 7;
                rawblock[p][3][3][5] |= 7;
                break;
            case 38:
                rawblock[p][3][3][4] |= 9;//X
                rawblock[p][3][3][2] |= 9;
                rawblock[p][3][3][3] |= 9;
                rawblock[p][4][3][3] |= 9;
                rawblock[p][2][3][3] |= 9;
                break;
            case 39:
                rawblock[p][3][3][5] |= 11;//Y
                rawblock[p][3][3][4] |= 11;
                rawblock[p][3][3][3] |= 11;
                rawblock[p][3][3][2] |= 11;
                rawblock[p][4][3][3] |= 11;
                break;
            case 40:
                rawblock[p][2][3][2] |= 10;//Z
                rawblock[p][2][3][3] |= 10;
                rawblock[p][3][3][3] |= 10;
                rawblock[p][4][3][3] |= 10;
                rawblock[p][4][3][4] |= 10;
                break;
            case 41:
                rawblock[p][2][3][2] |= 64;
                rawblock[p][2][3][3] |= 64;
                rawblock[p][2][3][4] |= 64;
                rawblock[p][3][3][2] |= 64;
                rawblock[p][3][3][3] |= 64;
                rawblock[p][3][3][4] |= 64;
                rawblock[p][4][3][2] |= 64;
                rawblock[p][4][3][3] |= 64;
                rawblock[p][4][3][4] |= 64;
                break;
            case 42:
                rawblock[p][2][3][2] |= 16;
                rawblock[p][2][3][3] |= 16;
                rawblock[p][2][3][4] |= 16;
                rawblock[p][3][3][2] |= 16;
                rawblock[p][3][3][3] |= 16;
                rawblock[p][3][3][4] |= 16;
                break;
            case 43:
                rawblock[p][2][2][2] |= 17;
                rawblock[p][2][2][3] |= 17;
                rawblock[p][2][3][2] |= 17;
                rawblock[p][2][3][3] |= 17;
                rawblock[p][3][2][2] |= 17;
                rawblock[p][3][2][3] |= 17;
                rawblock[p][3][3][2] |= 17;
                rawblock[p][3][3][3] |= 17;
                break;
            case 44:
                rawblock[p][3][0][3] |= 18;
                rawblock[p][3][1][3] |= 18;
                rawblock[p][3][2][3] |= 18;
                rawblock[p][3][3][3] |= 18;
                rawblock[p][3][4][3] |= 18;
                rawblock[p][3][5][3] |= 18;
                break;
            case 45:
                rawblock[p][3][0][3] |= 19;
                rawblock[p][3][1][3] |= 19;
                rawblock[p][3][2][3] |= 19;
                rawblock[p][3][3][3] |= 19;
                rawblock[p][3][4][3] |= 19;
                rawblock[p][3][5][3] |= 19;
                rawblock[p][3][6][3] |= 19;
                break;
            case 46:
                rawblock[p][3][3][3] |= 20;
                rawblock[p][3][2][3] |= 20;
                rawblock[p][3][4][3] |= 20;
                rawblock[p][2][3][3] |= 20;
                rawblock[p][4][3][3] |= 20;
                rawblock[p][3][3][2] |= 20;
                rawblock[p][3][3][4] |= 20;
                break;
            case 47:
                rawblock[p][2][3][2] |= 21;
                rawblock[p][2][3][3] |= 21;
                rawblock[p][2][3][4] |= 21;
                rawblock[p][3][3][2] |= 21;
                rawblock[p][3][3][4] |= 21;
                rawblock[p][4][3][2] |= 21;
                rawblock[p][4][3][3] |= 21;
                rawblock[p][4][3][4] |= 21;
                break;
            case 48:
                rawblock[p][2][3][3] |= 22;
                rawblock[p][3][3][3] |= 22;
                rawblock[p][4][3][3] |= 22;
                rawblock[p][5][3][3] |= 22;
                rawblock[p][2][4][3] |= 22;
                rawblock[p][3][4][3] |= 22;
                rawblock[p][4][4][3] |= 22;
                rawblock[p][5][4][3] |= 22;
                break;
            case 49:
                rawblock[p][2][2][3] |= 23;
                rawblock[p][3][2][3] |= 23;
                rawblock[p][4][2][3] |= 23;
                rawblock[p][2][3][3] |= 23;
                rawblock[p][3][3][3] |= 23;
                rawblock[p][2][4][3] |= 23;
                break;
            case 50:
                rawblock[p][3][3][3] |= 24;
                rawblock[p][3][2][3] |= 24;
                rawblock[p][3][4][3] |= 24;
                rawblock[p][2][3][3] |= 24;
                rawblock[p][4][3][3] |= 24;
                rawblock[p][3][3][4] |= 24;
                break;
            case 51:
                rawblock[p][3][3][3] |= 25;
                rawblock[p][3][2][3] |= 25;
                rawblock[p][3][4][3] |= 25;
                rawblock[p][2][3][3] |= 25;
                rawblock[p][4][3][3] |= 25;
                rawblock[p][3][3][4] |= 25;
                rawblock[p][3][3][5] |= 25;
                break;
            case 52:
                rawblock[p][3][3][3] |= 26;
                rawblock[p][3][2][3] |= 26;
                rawblock[p][3][1][3] |= 26;
                rawblock[p][2][3][3] |= 26;
                rawblock[p][1][3][3] |= 26;
                rawblock[p][3][3][4] |= 26;
                rawblock[p][3][3][5] |= 26;
                break;
            case 53:
                rawblock[p][2][3][2] |= 27;
                rawblock[p][2][3][3] |= 27;
                rawblock[p][2][3][4] |= 27;
                rawblock[p][3][3][2] |= 27;
                rawblock[p][3][3][3] |= 27;
                rawblock[p][3][3][4] |= 27;
                rawblock[p][4][3][2] |= 27;
                rawblock[p][4][3][3] |= 27;
                rawblock[p][4][3][4] |= 27;
                rawblock[p][3][4][3] |= 27;
                rawblock[p][3][2][3] |= 27;
                break;
            case 54:
                rawblock[p][2][3][2] |= 28;
                rawblock[p][2][3][3] |= 28;
                rawblock[p][2][3][4] |= 28;
                rawblock[p][3][3][2] |= 28;
                rawblock[p][3][3][3] |= 28;
                rawblock[p][3][3][4] |= 28;
                rawblock[p][4][3][2] |= 28;
                rawblock[p][4][3][3] |= 28;
                rawblock[p][4][3][4] |= 28;

                rawblock[p][2][4][2] |= 28;
                rawblock[p][2][4][3] |= 28;
                rawblock[p][2][4][4] |= 28;
                rawblock[p][3][4][2] |= 28;
                rawblock[p][3][4][3] |= 28;
                rawblock[p][3][4][4] |= 28;
                rawblock[p][4][4][2] |= 28;
                rawblock[p][4][4][3] |= 28;
                rawblock[p][4][4][4] |= 28;

                rawblock[p][2][2][2] |= 28;
                rawblock[p][2][2][3] |= 28;
                rawblock[p][2][2][4] |= 28;
                rawblock[p][3][2][2] |= 28;
                rawblock[p][3][2][3] |= 28;
                rawblock[p][3][2][4] |= 28;
                rawblock[p][4][2][2] |= 28;
                rawblock[p][4][2][3] |= 28;
                rawblock[p][4][2][4] |= 28;
                break;
            case 55:
                rawblock[p][2][3][2] |= 29;
                rawblock[p][2][3][4] |= 29;
                rawblock[p][4][3][2] |= 29;
                rawblock[p][4][3][4] |= 29;

                rawblock[p][2][4][2] |= 29;
                rawblock[p][2][4][3] |= 29;
                rawblock[p][2][4][4] |= 29;
                rawblock[p][3][4][2] |= 29;
                rawblock[p][3][4][4] |= 29;
                rawblock[p][4][4][2] |= 29;
                rawblock[p][4][4][3] |= 29;
                rawblock[p][4][4][4] |= 29;

                rawblock[p][2][2][2] |= 29;
                rawblock[p][2][2][3] |= 29;
                rawblock[p][2][2][4] |= 29;
                rawblock[p][3][2][2] |= 29;
                rawblock[p][3][2][4] |= 29;
                rawblock[p][4][2][2] |= 29;
                rawblock[p][4][2][3] |= 29;
                rawblock[p][4][2][4] |= 29;
                break;

            default:
                break;
        }
    }
    setnextblock();
    setnextblock();
}
void rotate2();
int rotate(int pos, int deg);
int setnextblock() {


    asc=0;
    {
        unsigned char(*tmp)[7][7];
        tmp = nextblock;
        nextblock = nowblock;
        nowblock = tmp;
        int intt;
        intt=nexthb;
        nexthb=nowhb;
        nowhb=intt;
        intt=nextib;
        nextib=nowib;
        nowib=intt;
    }
    nexthb=0,nextib=0;
    int b1, b2, b3, b4, b5;
  //  level=0;
    switch (level) {
        case 0:
            b1 = 50;b2 = 100;b3 = 100;b4 = 100;b5 = 100;
            break;
        case 1:
            //
            b1 = 10; b2 = 30; b3 = 60; b4 = 100;b5 = 100;
            break;
        case 2:
            b1 = 6; b2 = 20; b3 = 40; b4 = 98;b5 = 100;
            break;
        case 3:
            b1 = 4; b2 = 10; b3 = 30; b4 = 95;b5 = 100;
            break;
        case 4:
            b1 = 3; b2 = 8; b3 = 34; b4 = 93;b5 = 99;
            break;
        case 5:
            b1 = 3; b2 = 7; b3 = 32; b4 = 90;b5 = 99;
            break;
        case 6:
            b1 = 2; b2 = 6; b3 = 31; b4 = 88;b5 = 99;
            break;
        case 7:
            b1 = 2; b2 = 5; b3 = 30; b4 = 86;b5 = 98;
            break;
        case 8:
            b1 = 2; b2 = 5; b3 = 29; b4 = 84;b5 = 98;
            break;
        case 9:
            b1 = 2; b2 = 5; b3 = 29; b4 = 82;b5 = 98;
            break;
        case 10:
            b1 = 2; b2 = 5; b3 = 28; b4 = 81;b5 = 97;
            break;
        case 11:
            b1 = 2; b2 = 5; b3 = 28; b4 = 80;b5 = 97;
            break;
        case 12:
            b1 = 2; b2 = 5; b3 = 28; b4 = 79;b5 = 97;
            break;
        case 13:
            b1 = 2; b2 = 5; b3 = 28; b4 = 78;b5 = 96;
            break;
        case 14:
            b1 = 2; b2 = 5; b3 = 27; b4 = 78;b5 = 96;
            break;
        case 15:
            b1 = 2; b2 = 5; b3 = 27; b4 = 77;b5 = 96;
            break;
        case 16:
            b1 = 2; b2 = 5; b3 = 27; b4 = 76;b5 = 96;
            break;
        default:
            b1 = 2; b2 = 5; b3 = 27; b4 = 75;b5 = 95;
            break;
    }
    if (monoonly) { b1 = 100; b2 = 100; b3 = 100; b4 = 100;monoonly--; }
    int t = rand() % 100;
    if (t < b1) t = 0;
    else if (t < b2) t = 1;
    else if (t < b3) t = 2 + rand() % 2;
    else if (t < b4) t = 4 + rand() % 8;
    else if (t < b5)t = 12 + rand() % 29;
    else {
        if(rand()%2){createnewblock();t = 56;}
        else t=41+rand()%15;
    }
//116 2-         0.0100  0100
//117 2+         0.0300  0400
//118 vc         0.0300  0700
//119 ac         0.0001  0701
//104 mono15     0.0300  1001
//120 bomb       0.3500  1501
//121 bomb       0.1000  2501
//122 bomb       0.0700  3201
//123 bomb       0.0300  3501
//124 3-         0.0003  3504
//125 +          0.0800  4304
//91 spinlock    0.0250  4554
//102 up del     0.0100  4654
//126 xz del     0.0200  4854
//105 yz del     0.0200  5054
//127 bombcretor 0.0100  5154
//1 +2           0.0200  5354
//2 hidefaling   0.0250  5554
//5 eraseitem    0.1000  6554
//6 hidenext     0.0250  6804
//bt                     9804
// 4 scorex4      1.0000 19804
    for (int x = 0; x < 7; x++)
        for (int y = 0; y < 7; y++)
            for (int z = 0; z < 26; z++) {
                if (120 <= blk[x][y][z] && blk[x][y][z] < 123) blk[x][y][z]++;
                else if (blk[x][y][z] == 123) { //97
                    for (int x2 = x - 1; x2 <= x + 1; x2++)
                        for (int y2 = y - 1; y2 <= y + 1; y2++)
                            for (int z2 = z - 1; z2 <= z + 1; z2++)
                                if (x2 >= 0 && x2 < 7 && y2 >= 0 && y2 < 7 && z2 >= 0 && z2 < 26)
                                    blk[x2][y2][z2] = (rand() % 4 != 0) * 98;
                }
                else if (blk[x][y][z] == 32) { //97
                    for (int x2 = x - 1; x2 <= x + 1; x2++)
                        for (int y2 = y - 1; y2 <= y + 1; y2++)
                            for (int z2 = z - 1; z2 <= z + 1; z2++)
                                if (x2 >= 0 && x2 < 7 && y2 >= 0 && y2 < 7 && z2 >= 0 && z2 < 26)
                                    blk[x2][y2][z2] = 0;
                }
            }
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++) {
                if (rawblock[t][x][y][z] != 0) {
                    u = (rand() % 16384 + (rand()%16384) * 16384)%1000000;

                    if (u < 100) nextblock[x][y][z] = 116;
                    else if (u < 400) nextblock[x][y][z] = 117;
                    else if (u < 700) nextblock[x][y][z] = 118;
                    else if (u < 701) nextblock[x][y][z] = 119;
                    else if (u < 1001) nextblock[x][y][z] = 104;
                    else if (u < 1501) nextblock[x][y][z] = 120;
                    else if (u < 2501) nextblock[x][y][z] = 121;
                    else if (u < 3201) nextblock[x][y][z] = 122;
                    else if (u < 3501) nextblock[x][y][z] = 123;
                    else if (u < 3504) nextblock[x][y][z] = 124;
                    else if (u < 4304) nextblock[x][y][z] = 125;
                    else if (u < 4554) nextblock[x][y][z] = 91;
                    else if (u < 4654) nextblock[x][y][z] = 102;
                    else if (u < 4854) nextblock[x][y][z] = 126;
                    else if (u < 5054) nextblock[x][y][z] = 105;
                    else if (u < 5154) nextblock[x][y][z] = 127;
                    else if (u < 5354) nextblock[x][y][z] = 1;
                    else if (u < 5554) nextblock[x][y][z] = 2;
                    else if (u < 6554) nextblock[x][y][z] = 5;
                    else if (u < 6804) nextblock[x][y][z] = 6;
                    else if (u < 9804) nextblock[x][y][z] = 120;
                    else if (u <19804) nextblock[x][y][z] = 4;
                    else if(t==0 && rand()%20==0) {nexthb=1;nextblock[x][y][z]=30;}
                    else if(t==0 && rand()%10==1) {nextblock[x][y][z]=32;}
                    else if(t==0 && rand()%5<2) {nextib=1;nextblock[x][y][z]=31;}

                    else if (monoonly) nextblock[x][y][z] = 12 + rand() % 4;
                    else if( 20000<u && u<60000&&gethour()==0 && getminute()==0){
                        u-=20000;
                        switch(getdatep()){

                            case 0:
                                if(u<4900) nextblock[x][y][z] = 116;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 1:
                                if(u<14700) nextblock[x][y][z] = 117;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 2:
                                if(u<14700) nextblock[x][y][z] = 118;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 3:
                                if(u<499) nextblock[x][y][z] = 119;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 4:
                                if(u<14700) nextblock[x][y][z] = 104;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 5:
                                if(u<171500) nextblock[x][y][z] = 120;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 6:
                                if(u<49000) nextblock[x][y][z] = 121;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 7:
                                if(u<34300) nextblock[x][y][z] = 122;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 8:
                                if(u<14700) nextblock[x][y][z] = 123;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 9:
                                if(u<1497) nextblock[x][y][z] = 124;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 10:
                                if(u<39200) nextblock[x][y][z] = 125;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 11:
                                if(u<12250) nextblock[x][y][z] = 91;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 12:
                                if(u<4900) nextblock[x][y][z] = 102;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 13:
                                if(u<9800) nextblock[x][y][z] = 126;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 14:
                                if(u<9800) nextblock[x][y][z] = 105;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 15:
                                if(u<4900) nextblock[x][y][z] = 127;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 16:
                                if(u<9800) nextblock[x][y][z] = 1;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 17:
                                if(u<12250) nextblock[x][y][z] = 2;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 18:
                                if(u<49000) nextblock[x][y][z] = 5;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            case 19:
                                if(u<12250) nextblock[x][y][z] = 6;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                            default:
                                if(u<40000) nextblock[x][y][z] =4 ;
                                else nextblock[x][y][z] = rawblock[t][x][y][z];
                                break;
                        }
                    }
                    else nextblock[x][y][z] = rawblock[t][x][y][z];

                }
                else nextblock[x][y][z] = 0;
            }
    blockpos[0] = 0;
    blockpos[1] = 0;
    blockpos[2] = 14;
    {
        {
            unsigned char(*tmp)[7][7];
            tmp = nextblock;
            nextblock = nowblock;
            nowblock = tmp;
        }
        int i = rand() % 4, j = rand() % 4, k = rand() % 4;
        for (int x = 0; x < i; x++)  rotate(0, 1);
        for (int x = 0; x < j; x++) rotate(1, 1);
        for (int x = 0; x < k; x++) rotate(2, 1);
        {
            unsigned char(*tmp)[7][7];
            tmp = nextblock;
            nextblock = nowblock;
            nowblock = tmp;
        }
    }
    rotate2();

    return move(0, 0);
}
void rotate2() {
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                b[2][y][6 - x][z] = nowblock[x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                rbt[0][0][x][y][z] = b[2][x][y][z];

    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                b[2][6 - y][x][z] = nowblock[x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                rbt[0][1][x][y][z] = b[2][x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                b[2][x][z][6 - y] = nowblock[x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                rbt[1][0][x][y][z] = b[2][x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                b[2][x][6 - z][y] = nowblock[x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                rbt[1][1][x][y][z] = b[2][x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                b[2][z][y][6 - x] = nowblock[x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                rbt[2][0][x][y][z] = b[2][x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                b[2][6 - z][y][x] = nowblock[x][y][z];
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++)
                rbt[2][1][x][y][z] = b[2][x][y][z];
    return;
}
int rotate(int pos, int deg) {//pos 0:xy pos 1:yz pos 2:xz
    //success 0 fail 1

    if (spinlock != 0) return 0;
    if ((deg & 3) == 0) return 0;
    switch (pos) {
        case 0:
            if ((deg & 3) == 1)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][6 - y][x][z] = nowblock[x][y][z];
            else if ((deg & 3) == 2)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][6 - y][6 - x][z] = nowblock[x][y][z];
            else//(deg==-1)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][y][6 - x][z] = nowblock[x][y][z];
            break;
        case 1:
            if ((deg & 3) == 1)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][x][6 - z][y] = nowblock[x][y][z];
            else if ((deg & 3) == 2)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][x][6 - z][6 - y] = nowblock[x][y][z];
            else//(deg==-1)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][x][z][6 - y] = nowblock[x][y][z];
            break;
        default:
            if ((deg & 3) == 1)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][6 - z][y][x] = nowblock[x][y][z];
            else if ((deg & 3) == 2)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][6 - z][y][6 - x] = nowblock[x][y][z];
            else//(deg==-1)
                for (int x = 0; x<7; x++)
                    for (int y = 0; y<7; y++)
                        for (int z = 0; z < 7; z++)
                            b[2][z][y][6 - x] = nowblock[x][y][z];
            break;
    }
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++) {
                if (b[2][x][y][z] != 0) {
                    //b[x+blockpos[0]]
                    if (x + blockpos[0] < 0) {

                        return 1;
                    }
                    else if (x + blockpos[0] > 6) return 1;
                    else if (y + blockpos[1] < 0) return 1;
                    else if (y + blockpos[1] > 6) return 1;
                    else if (z + blockpos[2] < 0) return 1;
                    else if (z + blockpos[2] > 25) return 1;
                    if (blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2]]!=0 && blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2]]!=31 ) return 1;
                }
            }
    for (int x = 0; x<7; x++)
        for (int y = 0; y<7; y++)
            for (int z = 0; z < 7; z++){
                nowblock[x][y][z] = b[2][x][y][z];
                if(nowblock[x][y][z]!=0&&blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2]]==31) {nowblock[x][y][z]=0;blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2]]=0;}
            }

    rotate2();
    return 0;
}
void gover();
int removeline();
int move(int pos, int deg) {//pos 0:x pos 1:y pos 2:z
    Q:
    blockpostmp[0] = blockpos[0];
    blockpostmp[1] = blockpos[1];
    blockpostmp[2] = blockpos[2];
    //success 0 fail 1
    switch (pos) {
        case 0:
            blockpostmp[0] += deg;
            break;
        case 1:
            blockpostmp[1] += deg;
            break;
        default:
            blockpostmp[2] += deg;
            break;
    }
    for (int x = 0;x<7;x++)
        for (int y = 0;y<7;y++)
            for (int z = 0;z < 7;z++) {
                if (nowblock[x][y][z] != 0) {
                    //b[x+blockpostmp[0]]
                    if (x + blockpostmp[0] < 0) return 1;
                    else if (x + blockpostmp[0] > 6) return 1;
                    else if (y + blockpostmp[1] < 0) return 1;
                    else if (y + blockpostmp[1] > 6) return 1;
                    else if (z + blockpostmp[2] < 0) {
                        if(nowhb){
                            for (int x = 0;x<7;x++)
                                for (int y = 0;y<7;y++){
                                    int z=0,t=0;
                                    while(z<25){


                                        if(blk[x][y][z]!=0 ){

                                            blk[x][y][t]=blk[x][y][z];
                                            if(t!=0 &&((blk[x][y][t]==31 && blk[x][y][t-1]!=0&&blk[x][y][t-1]!=31)||(blk[x][y][t-1]==31 && blk[x][y][t]!=0&&blk[x][y][t]!=31))){
                                                blk[x][y][t]=0;
                                                blk[x][y][t-1]=0;
                                                t-=2;
                                            }
                                            t++;

                                        }
                                        z++;
                                    }
                                    while(t<25){
                                        blk[x][y][t]=0;
                                        t++;
                                    }

                                }
                            removeline();
                        }
                        return 1;
                    }
                    else if (z + blockpostmp[2] > 25) return 1;
                    if(nowhb==0&&0==nowib) {
                        if(blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]==31){
                            blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]=0;
                            nowblock[x][y][z]=0;
                            for (int x = 0;x<7;x++)
                                for (int y = 0;y<7;y++)
                                    for (int z = 0;z < 7;z++)
                                        if(nowblock[x][y][z]!=0) goto P;
                            setnextblock();

                            score+=40;
                            goto Q;
                            P:;

                        }
                        else if (blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]) return 1;
                    }
                    else if(nowhb==1) {
                        if(blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]==31){
                            blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]=0;
                            nowblock[x][y][z]=0;

                            setnextblock();

                            score+=40;
                            goto Q;


                        }
                        blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]=0;

                    }
                    else if(nowib==1&& (blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]!=31&&blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]!=0)) {
                        blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]=0;
                        setnextblock();
                        score+=40;
                        goto Q;
                    }
                    else if(nowib==1 &&blk[x + blockpostmp[0]][y + blockpostmp[1]][z + blockpostmp[2]]==31 )return 1;
                }
            }
    blockpos[0] = blockpostmp[0];
    blockpos[1] = blockpostmp[1];
    blockpos[2] = blockpostmp[2];
    return 0;
}
int removeline() {
//116 2- 0.25% 82
//117 2+ 0.25% 164
//118 vc 1.25% 574 (4*4)
//119 ac 0.05% 590
//104 mono15 0.20% 657
//120~123 bomb 0.6% 854
//124 3- 0.20% 901
//125 + 0.8% 1163
//91 spinlock 0.2% 1228
//102 up del 0.05% 1244
//126 xz del 0.6% 1441 (3)
//105 yz del 0.6% 1638 (3)
//127 bombcretor 0.02% 1644
//65 +2
//66 hide faling block
//69 erase item
//68 score x2
//70 hide next

    if (spinlock > 0) spinlock--;
    if(hideblock>0) hideblock--;
    if(hidenext>0) hidenext--;

    int filledline = 0;
    int tline = 0;
    int tline2;
    bool tmp;
    for (int x = 0; x < 7; x++) {
        for (int z = 0; z < 26; z++) {
            tline2 = 0;
            tmp = 0;
            for (int y = 0; y < 7; y++) {
                if (blk[x][y][z] == 0) { tmp = 1; break; }
                else if (blk[x][y][z] <128) tline2++;
            }
            if (tmp == 0) {
                if (tline2)filledline++;
                for (int y = 0; y < 7; y++) {
                    if ((blk[x][y][z] & 127) == 116) { tline -= 2; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 117) { tline += 2; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 118) {
                        blk[x][y][z] = 128;
                        for (int x2 = x - 1; x2 <= x + 1; x2++) {
                            for (int y2 = y - 1; y2 <= y + 1; y2++) {
                                if (x2 >= 0 && x2 < 7 && y2 >= 0 && y2 < 7) {
                                    for (int z2 = 0; z2 < 26; z2++) {
                                        blk[x2][y2][z2] |= 128;
                                    }
                                }
                            }
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 119) {
                        for (int x2 = 0; x2<7; x2++)
                            for (int y2 = 0; y2<7; y2++)
                                for (int z2 = 0; z2 < 26; z2++)
                                    blk[x2][y2][z2] = 0;
                        return 0;
                    }
                    else if ((blk[x][y][z] & 127) == 104) { monoonly += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 124) { tline -= 3; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 125) { tline++; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 91) { spinlock += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 2) { hideblock += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 6) { hidenext += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 5) {
                        for (int x2 = 0; x2<7; x2++)
                            for (int y2 = 0; y2<7; y2++)
                                for (int z2 = 0; z2<26; z2++)
                                    if(blk[x2][y2][z2]!=0&&blk[x2][y2][z2]!=128 ) blk[x2][y2][z2]=
                                                                                          33+ blk[x2][y2][z2] % 31;
                        blk[x][y][z] = 128;

                    }

                    else if ((blk[x][y][z] & 127) == 4) { score2x += 1; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 1) {
                        blk[x][y][z] = 128;
                        int t=0;
                        for(int i=0;i<50;i++){
                            int x=rand()%6;
                            int y=rand()%6;
                            int z=rand()%6;
                            if(blk[x][y][z]==0 && blk[x+1][y][z]==0 && blk[x][y+1][z]==0 && blk[x][y][z+1]==0){
                                t++;
                                blk[x][y][z]=103;
                            }
                            if(t==3) break;
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 102) {
                        for (int x2 = 0; x2<7; x2++)
                            for (int y2 = 0; y2<7; y2++)
                                for (int z2 = z; z2<26; z2++)
                                    blk[x2][y2][z2] = 128;
                    }
                    else if ((blk[x][y][z] & 127) == 105) {
                        blk[x][y][z] = 128;
                        for (int x2 = x - 1; x2 <= x + 1; x2++) {
                            for (int y2 = 0; y2 <7; y2++) {
                                if (x2 >= 0 && x2 < 7) {
                                    for (int z2 = 0; z2 < 26; z2++) {
                                        blk[x2][y2][z2] |= 128;
                                    }
                                }
                            }
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 126) {
                        blk[x][y][z] = 128;
                        for (int x2 = 0; x2 <7; x2++) {
                            for (int y2 = y - 1; y2 <= y + 1; y2++) {
                                if (y2 >= 0 && y2 < 7) {
                                    for (int z2 = 0; z2 < 26; z2++) {
                                        blk[x2][y2][z2] |= 128;
                                    }
                                }
                            }
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 127) {
                        blk[x][y][z] |= 128;
                        for (int x2 = 0; x2 < 7; x2++)
                            for (int y2 = 0; y2 < 7; y2++)
                                for (int z2 = 0; z2 < 26; z2++)
                                    if ((blk[x2][y2][z2] & 127) != 0 && rand() % 10 == 0)
                                        blk[x2][y2][z2] = (blk[x2][y2][z2] & 128) + 120;
                    }
                    else blk[x][y][z] |= 128;
                }
            }
        }
    }
    for (int y = 0; y < 7; y++) {
        for (int z = 0; z < 26; z++) {
            tline2 = 0;
            tmp = 0;
            for (int x = 0; x < 7; x++) {
                if (blk[x][y][z] == 0) { tmp = 1; break; }
                else if (blk[x][y][z] <128) tline2++;
            }
            if (tmp == 0) {
                if (tline2)filledline++;
                for (int x = 0; x < 7; x++) {
                    if ((blk[x][y][z] & 127) == 116) { tline -= 2; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 117) { tline += 2; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 118) {
                        blk[x][y][z] = 128;
                        for (int x2 = x - 1; x2 <= x + 1; x2++) {
                            for (int y2 = y - 1; y2 <= y + 1; y2++) {
                                if (x2 >= 0 && x2 < 7 && y2 >= 0 && y2 < 7) {
                                    for (int z2 = 0; z2 < 26; z2++) {
                                        blk[x2][y2][z2] |= 128;
                                    }
                                }
                            }
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 119) {
                        for (int x2 = 0; x2<7; x2++)
                            for (int y2 = 0; y2<7; y2++)
                                for (int z2 = 0; z2 < 26; z2++)
                                    blk[x2][y2][z2] = 0;
                        return 0;
                    }

                    else if ((blk[x][y][z] & 127) == 104) { monoonly += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 124) { tline -= 3; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 125) { tline++; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 91) { spinlock += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 2) { hideblock += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 6) { hidenext += 10; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 5) {
                        for (int x2 = 0; x2<7; x2++)
                            for (int y2 = 0; y2<7; y2++)
                                for (int z2 = 0; z2<26; z2++)
                                    if(blk[x2][y2][z2]!=0&&blk[x2][y2][z2]!=128 ) blk[x2][y2][z2]=33 + blk[x2][y2][z2] % 31;
                        blk[x][y][z] = 128;

                    }

                    else if ((blk[x][y][z] & 127) == 4) { score2x += 1; blk[x][y][z] = 128; }
                    else if ((blk[x][y][z] & 127) == 1) {
                        blk[x][y][z] = 128;
                        int t=0;
                        for(int i=0;i<50;i++){
                            int x=rand()%6;
                            int y=rand()%6;
                            int z=rand()%6;
                            if(blk[x][y][z]==0 && blk[x+1][y][z]==0 && blk[x][y+1][z]==0 && blk[x][y][z+1]==0){
                                t++;
                                blk[x][y][z]=103;
                            }
                            if(t==3) break;
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 102) {
                        for (int x2 = 0; x2<7; x2++)
                            for (int y2 = 0; y2<7; y2++)
                                for (int z2 = z; z2<26; z2++)
                                    blk[x2][y2][z2] = 128;
                    }
                    else if ((blk[x][y][z] & 127) == 105) {
                        blk[x][y][z] = 128;
                        for (int x2 = x - 1; x2 <= x + 1; x2++) {
                            for (int y2 = 0; y2 <7; y2++) {
                                if (x2 >= 0 && x2 < 7) {
                                    for (int z2 = 0; z2 < 26; z2++) {
                                        blk[x2][y2][z2] |= 128;
                                    }
                                }
                            }
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 126) {
                        blk[x][y][z] = 128;
                        for (int x2 = 0; x2 <7; x2++) {
                            for (int y2 = y - 1; y2 <= y + 1; y2++) {
                                if (y2 >= 0 && y2 < 7) {
                                    for (int z2 = 0; z2 < 26; z2++) {
                                        blk[x2][y2][z2] |= 128;
                                    }
                                }
                            }
                        }
                    }
                    else if ((blk[x][y][z] & 127) == 127) {
                        blk[x][y][z] |= 128;
                        for (int x2 = 0; x2 < 7; x2++)
                            for (int y2 = 0; y2 < 7; y2++)
                                for (int z2 = 0; z2 < 26; z2++)
                                    if ((blk[x2][y2][z2] & 127) != 0 && rand() % 10 == 0)
                                        blk[x2][y2][z2] = (blk[x2][y2][z2] & 128) + 120;
                    }

                    else blk[x][y][z] |= 128;
                }
            }
        }
    }
    if (tline < 0) {
        for (int z = 0; z < -tline; z++)
            for (int x = 0; x < 7; x++)
                for (int y = 0; y < 7; y++)
                    blk[x][y][z] = 128;
        tline = 0;
    }

    int t;
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            t = 0;
            for (int z = 0; z < 26; z++) {
                if (blk[x][y][z] < 128) {
                    blk[x][y][t] = blk[x][y][z];
                    t++;
                }
            }
            for (; t < 26; t++) {
                blk[x][y][t] = 0;
            }
        }
    }
    if (tline > 0) {

        for (int x = 0; x < 7; x++) {
            for (int y = 0; y < 7; y++) {
                for (int z = 25 - tline; z >-1; z--) {
                    blk[x][y][z + tline] = blk[x][y][z];
                }
                for (int t = 0; t < tline; t++) {

                    blk[x][y][t] = (rand() % 2 != 0) * 103;
                    if (x % 7 == (y + t) % 7) blk[x][y][t] = 0;
                }
            }
        }

    }
    if (filledline != 0) filledline += removeline();



    return filledline;
}
long long gt,ht=0;
int stickblock() {//1:dead 0:non-dead
    asc=0;
    for (int x = 0;x<7;x++)
        for (int y = 0;y<7;y++)
            for (int z = 0;z < 7;z++) {
                if(nowblock[x][y][z]!=0){

                    if(blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2]+1]!=0 ) {
                        int it=0,jt=0;
                        if (z+blockpos[2]==0 || blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2] - 1] != 0)
                            it++;
                        if (y+blockpos[1]==6 || blk[x + blockpos[0]][y + blockpos[1] + 1][z + blockpos[2]] != 0)
                            it++;
                        if (y!=6&&nowblock[x][y + 1][z] != 0) jt++;
                        if (y+blockpos[1]==0 || blk[x + blockpos[0]][y + blockpos[1] - 1][z + blockpos[2]] != 0)
                            it++;
                        if (y!=0&&nowblock[x][y - 1][z] != 0) jt++;
                        if (x+blockpos[0]==6 || blk[x + blockpos[0] + 1][y + blockpos[1]][z + blockpos[2]] != 0)
                            it++;
                        if (x!=6&&nowblock[x + 1][y][z] != 0) jt++;
                        if (x+blockpos[0]==0 || blk[x + blockpos[0] - 1][y + blockpos[1]][z + blockpos[2]] != 0)
                            it++;
                        if (x!=0&&nowblock[x - 1][y][z] != 0) jt++;
                       // LOGI("!!!!!!!!!!!!!%d %d",it,jt);
                        //4 1
                        if (jt<3 && it+jt>3)
                            asc++;


                    }
                }

            }
    if(asc!=0) asc--;
    if(asc!=0){

        gt=50*(long long)pow(4.,asc);
        score += gt;
        ht=GetTickCount2();
    }
    for (int x = 0;x<7;x++)
        for (int y = 0;y<7;y++)
            for (int z = 0;z < 7;z++) {
                if (nowblock[x][y][z] != 0) {
                    if (z + blockpos[2] > 8) {

                        return 1;
                    }
                    blk[x + blockpos[0]][y + blockpos[1]][z + blockpos[2]] = nowblock[x][y][z];

                }
            }
    return setnextblock();

}
void calculatescore(int line) {
    lines += line;
if(score2x>2) score2x=2;
    score += (int)(20.*((double)line)*sqrt((double)line)*pow(4.,(double)score2x))*pow(4,(double)asc);
    if(score>999999999) score=999999999;
    score2x=0;
    level = ((score+600)/800 ) + 1;
    if (level > 16)level = 16;
}

#define PI 3.1415926535898
double r1o, r2o;
//
int clickbutton(GLfloat x, GLfloat y) {
    x /= 0.96;y /= 0.96;

    if (goverflg == 1 && -0.25 < x && x < 0.25 && -0.31 > y && y > -0.46) {
        goverflg = 0;
        return 0;
    }
    else if (goverflg == 1 && -0.25 < x && x < 0.25 && -0.51 > y && y > -0.66) {
        goverflg = 0;startscreen = 1;about = 0;
        return 0;
    }
    else if (startscreen == 1 && about == 0 && -0.25 < x && x < 0.25 && -0.31 > y && y > -0.46) {
        startscreen = 0;
        return 0;
    }
    else if (startscreen == 1 && -0.25 < x && x < 0.25 && -0.51 > y && y > -0.66) {
        about++;
        if (about == 5) about = 0;
        return 0;
    }
    else if (startscreen == 1 && about != 0) {
        about++;
        if (about == 5) about = 0;
        return 0;
    }
    else if((x - 0.5)*(x - 0.5) + (y + 0.325)*(y + 0.475) < 0.03){
        floorz++;
        if(floorz>=7)floorz=0;
        return 0;
    }
    else if ((x - 0.5)*(x - 0.5) + (y + 0.925)*(y + 0.925) < 0.09) {

        double deg = atan2(y + 0.925, x - 0.5) - r1o;
        printf("%lf\n", deg);
        while (deg < -PI) deg += 2 * PI;
        while (deg > PI) deg -= 2 * PI;
        if (1) {

            if (-3 * PI / 4 < deg && deg <= -PI / 4) {
                printf("2");
                if (!pause)move(1, 1);
                return 0;
            }
            else if (-PI / 4 < deg && deg <= PI / 4) {
                printf("3");
                if (!pause)move(0, 1);
                return 0;
            }
            else if (PI / 4 < deg && deg <= 3 * PI / 4) {
                printf("4");
                if (!pause)move(1, -1);
                return 0;
            }
            else {
                printf("1");
                if (!pause)move(0, -1);
                return 0;
            }
        }
        else {
            if (-3 * PI / 4 < deg && deg <= -PI / 4) {
                printf("2");
                if (!pause)move(1, -1);
                return 0;
            }
            else if (-PI / 4 < deg && deg <= PI / 4) {
                printf("1");
                if (!pause)move(0, 1);
                return 0;
            }
            else if (PI / 4 < deg && deg <= 3 * PI / 4) {
                printf("4");
                if (!pause)move(1, 1);
                return 0;
            }
            else {
                printf("3");
                if (!pause)move(0, -1);
                return 0;
            }
        }
    }
    else if (-0.4 + 0.115 < x && x < -0.4 + 0.305 && -1.0 - 0.095 < y && y < -1.0 + 0.095) {
        rotate(0, -1);
        return 0;
    }
    else if (-0.4 + 0.115 < x && x < -0.4 + 0.305 && -1.0 + 0.115 < y && y < -1.0 + 0.305) {
        rotate(0, 1);
        return 0;
    }
    else if (-0.4 - 0.095 < x && x < -0.4 + 0.095 && -1.0 - 0.095 < y && y < -1.0 + 0.095) {
        rotate(1, -1);
        return 0;
    }
    else if (-0.4 - 0.095 < x && x < -0.4 + 0.095 && -1.0 + 0.115 < y && y < -1.0 + 0.305) {
        rotate(1, 1);
        return 0;
    }
    else if (-0.4 - 0.115 > x && x > -0.4 - 0.305 && -1.0 - 0.095 < y && y < -1.0 + 0.095) {
        rotate(2, -1);
        return 0;
    }
    else if (-0.4 - 0.115 > x && x > -0.4 - 0.305 && -1.0 + 0.115 < y && y < -1.0 + 0.305) {
        rotate(2, 1);
        return 0;
    }
    else if (0.13 > x && x > -0.07 && -1.10 < y && y < -0.90) {
        vkspace2 = 1;
        return 0;
    }
    else if (0.13 > x && x > -0.07 && -0.80 < y && y < -0.60) {
        if(hidenext!=0) goto E;
for(int x=0;x<7;x++){
    for(int y=0;y<7;y++){
        for(int z=0;z<7;z++){
        if(holdblock[x][y][z]!=0){
            if(x+blockpos[0]<0) goto E;
            if(x+blockpos[0]>6) goto E;
            if(y+blockpos[1]<0) goto E;
            if(y+blockpos[1]>6) goto E;
            if(z+blockpos[2]<0) goto E;
            if(blk[x+blockpos[0]][y+blockpos[1]][z+blockpos[2]]!=0) goto E;

        }
        }
    }
}
        unsigned char (*blocktmp)[7][7];
        blocktmp=holdblock;
        holdblock=nowblock;
        nowblock=blocktmp;
        int intt;
        intt=holdhb;
        holdhb=nowhb;
        nowhb=intt;
        intt=holdib;
        holdib=nowib;
        nowib=intt;
        rotate2();
        E:

        return 0;
    }
    else if (0.6 > x && x > 0.4 && 1.0 > y && y > 0.75) {
        pause = !pause;

        return 0;
    }
    else if (-0.60>y) {

        return 0;
    }
    else return 1;
}

float scrollstack[300];
int scrollstackptr = 0;
long long zDeltasum = 0;
void zoom(short zDelta, float y) {
    zDeltasum += zDelta;
    t2 = 0.05 + (float)zDeltasum*0.00005;
    if (t2 <= 0) t2 = 0.005;
    if (zDelta > 0) {
        if (t2 > 0.05) {
            centery -= y*t2;
            scrollstack[scrollstackptr] = centery;
            if (scrollstackptr<300) scrollstackptr++;
        }
        else {
            centery = 0;
        }
    }
    else {
        if (scrollstackptr != 0) {
            scrollstackptr--;
            centery = scrollstack[scrollstackptr];
        }
        else {
            centery = 0;
        }

    }

}
GLuint texture[6];
struct Image {
    unsigned long sizeX;
    unsigned long sizeY;
    char *data;
};

void LoadGLTextures() {
    int i;
    LOGI("1");
    Image *image1[6];
    for (int t = 0;t<6;t++) {
        image1[t] = (Image*)malloc(sizeof(Image));
        if (image1[t] == NULL) exit(1);
        image1[t]->sizeX = 512;
        image1[t]->sizeY = 1024;
        image1[t]->data = (char*)malloc(1572864);
    }
    for (i = 0;i<1572864;i += 3) { /* reverse all of the colors. (bgr -> rgb)*/
        image1[0]->data[i] = stra[i + 2+54];
        image1[0]->data[i + 1] = stra[i + 1+54];
        image1[0]->data[i + 2] = stra[i+54];
    }
    for (i = 0;i<1572864;i += 3) { /* reverse all of the colors. (bgr -> rgb)*/
        image1[1]->data[i] = strb[i + 2+54];
        image1[1]->data[i + 1] = strb[i + 1+54];
        image1[1]->data[i + 2] = strb[i+54];
    }
    for (i = 0;i<1572864;i += 3) { /* reverse all of the colors. (bgr -> rgb)*/
        image1[2]->data[i] = strc[i + 2+54];
        image1[2]->data[i + 1] = strc[i + 1+54];
        image1[2]->data[i + 2] = strc[i+54];
    }
    for (i = 0;i<1572864;i += 3) { /* reverse all of the colors. (bgr -> rgb)*/
        image1[3]->data[i] = strd[i + 2+54];
        image1[3]->data[i + 1] = strd[i + 1+54];
        image1[3]->data[i + 2] = strd[i+54];
    }
    for (i = 0;i<1572864;i += 3) { /* reverse all of the colors. (bgr -> rgb)*/
        image1[4]->data[i] = stre[i + 2+54];
        image1[4]->data[i + 1] = stre[i + 1+54];
        image1[4]->data[i + 2] = stre[i+54];
    }
    for (i = 0;i<1572864;i += 3) { /* reverse all of the colors. (bgr -> rgb)*/
        image1[5]->data[i] = strf[i + 2+54];
        image1[5]->data[i + 1] = strf[i + 1+54];
        image1[5]->data[i + 2] = strf[i+54];
    }
    LOGI("3");
    glGenTextures(6, &texture[0]);
    for (int t = 0;t<6;t++) {


        glBindTexture(GL_TEXTURE_2D, texture[t]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, image1[t]->data);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    //glEnable(GL_TEXTURE_2D);
}
const double tr = 57.295779513082320876798154814105;
bool tts = false;
int ul;
#define sa(r,g,b) ((r<<4)+(g<<2)+b)
void draw();
void graph();
void dc(float X, float Y, float Z, unsigned char R, unsigned char G, unsigned char B, unsigned char pic, float t);
void drawcircle(float x, float y, float rad);
int ft;
void draw2();
void draw3();
void draw4();
unsigned long long otp = 0;
void DrawScene(void)
{

    if (touchs[0].flag != 0 && touchs[1].flag != 0) {
        if (touchs[0].y>-0.6&&touchs[1].y>-0.6) {
            if (touchs[0].flag == 2 && touchs[1].flag == 2) {
                double dist = sqrt((touchs[0].x - touchs[1].x) * (touchs[0].x - touchs[1].x) +
                                   (touchs[0].y - touchs[1].y) * (touchs[0].y - touchs[1].y));
                double odist = sqrt((touchs[0].oldx - touchs[1].oldx) * (touchs[0].oldx - touchs[1].oldx) +
                                    (touchs[0].oldy - touchs[1].oldy) * (touchs[0].oldy - touchs[1].oldy));

                zoom(1000. * (dist - odist), (touchs[0].y + touchs[1].y) / 2);
            }
        }

        if (touchs[0].flag == 1) touchs[0].flag = 2;
        else if (touchs[0].flag == 3)touchs[0].flag = 0;
        if (touchs[1].flag == 1) touchs[1].flag = 2;
        else if (touchs[1].flag == 3)touchs[1].flag = 0;
    }
    else if (touchs[0].flag == 1) {
        otp = GetTickCount2();
        ft = 1;

        if (clickbutton(touchs[0].x, touchs[0].y)) {
            if (touchs[0].y>-0.65) {
                ci = 0;
                tts = true;

                bi = GetTickCount2();
            }
        }
        else tts = false;
        touchs[0].flag = 2;
        ul = 3;
    }
    else if (touchs[0].flag == 3) {
        if (tts) {
            tts = false;
            ci = GetTickCount2() - bi;
        }
        else {
            vkspace2 = 0;
            ci = 0;
        }
        touchs[0].flag = 0;
    }
    else if (touchs[0].flag == 2) {
        if (otp + 80<GetTickCount2()) {
            otp = GetTickCount2();
            if (ul == 0) clickbutton(touchs[0].x, touchs[0].y);
            else ul--;
        }


        if (ft>0) ft++;
        if (ft == 6) ft = 0;
        if (tts == true) {
            upd = true;
        }
    }
    if (bBusy)
        return;
    bBusy = TRUE;
    if (startscreen == 1) draw4();
    else if (goverflg == 0) {
        if (!pause)draw();
        else draw2();
    }
    else draw3();
    bBusy = FALSE;

}
const GLshort texcoord[] = {
        0,1,0,0,1,0,1,1
};
void drawcircle(float x, float y, float rad, float z) {
    glBegin(GL_LINE_STRIP);
    for (float i = 0;i < 12.61;i += 0.5) {
        glVertex3f(x + sin(i)*rad, y + cos(i)*rad, z);
    }
    glEnd();
}
void draw2() {
    glLineWidth(4.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glViewport(0, activitysizey / 40, activitysizex, activitysizey * 41 / 40);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -4);

    glRotatef(0, 0.0f, 0.0f, 0.0f);
    glColor3f(0.5, 0.5, 0.5);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

    glVertex3f(-1., 2., -1.);
    glVertex3f(-1., -2., -1.);
    glVertex3f(1., -2., -1.);
    glVertex3f(1., 2., -1.);
    glVertexPointer(3, GL_FLOAT, 0, glbuffer);
    glTexCoordPointer(2, GL_SHORT, 0, texcoord);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(gltemp, 0, glcounter);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    float xt = -0.5, yt = 0.;

    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glVertex3f(xt, yt, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.05, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt + 0.025, yt, 0);
    glVertex3f(xt + 0.075, yt, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glEnd();

    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glEnd();
    glColor3f(0, 1, 1);
    glLineWidth(2.0);
    glPushMatrix();
    glTranslatef(0,-0.2,0);
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.40*2.1, 0.9*2.1, 0);
    glVertex3f(0.46*2.1, 0.935*2.1, 0);
    glVertex3f(0.40*2.1, 0.97*2.1, 0);
    glVertex3f(0.40*2.1, 0.9*2.1, 0);
    glEnd();

    glBegin(GL_LINE_STRIP);

    glVertex3f(0.3475*2.1, 0.86*2.1, 0);
    glVertex3f(0.4975*2.1, 0.86*2.1, 0);
    glVertex3f(0.4975*2.1, 1.01*2.1, 0);
    glVertex3f(0.3475*2.1, 1.01*2.1, 0);
    glVertex3f(0.3475*2.1, 0.86*2.1, 0);
    glEnd();
    glPopMatrix();
    glPopMatrix();
    eglSwapBuffers();
}

void draw() {

    glLineWidth(2.0);

    {

        if (timestamp + 6000 / (level/3 + 5) < GetTickCount2() || vkspace2 || vkspace) {

            if (move(2, -1)) {
                if (stickblock()) {

                    gover();
                    initblock();
                    goto END;

                }

                calculatescore(removeline());

            }
            timestamp = GetTickCount2();
        }
        if(GetTickCount2()-ht>500) gt=0;
#define abs2(x) ((x>0)?x:(-x))
        if (upd == true) {
            if (!ft) {
                if (abs2((touchs[0].x - touchs[0].oldx))<0.2)
                    wAngleY += (touchs[0].x - touchs[0].oldx) * 100.;
                if (abs2((touchs[0].y - touchs[0].oldy))<0.2)
                    wAngleX -= (touchs[0].y - touchs[0].oldy) * 100.;
            }
            upd = false;

        }



        if (ci > 300) {
            if (!ft) {
                if (abs2((touchs[0].x - touchs[0].oldx))<0.2)
                    wAngleY += (touchs[0].x - touchs[0].oldx) * 100.;
                if (abs2((touchs[0].y - touchs[0].oldy))<0.2)
                    wAngleX -= (touchs[0].y - touchs[0].oldy) * 100.;
            }
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    glViewport(0, activitysizey / 40, activitysizex, activitysizey * 41 / 40);
    glLoadIdentity();
    glTranslatef(centerx - 0.6, centery + 0.2, -4);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    glColor4f(0.7, 0.7, 0.7, 1);
    glBegin(GL_QUADS);

    glVertex3f(-7 * t2, -9.975 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -9.975* t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -9.975* t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -9.975* t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -9.975* t2+(float)floorz*2.*t2, -7 * t2);
    glEnd();
    glBegin(GL_QUADS);

    glVertex3f(-7 * t2, -10.025 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -10.025* t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -10.025* t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -10.025* t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -10.025* t2+(float)floorz*2.*t2, -7 * t2);
    glEnd();
    glColor3f(1, 1, 1);
    /////////////////

    glBegin(GL_LINE_STRIP);

    glVertex3f(-7 * t2, -9.95 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -9.95 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -9.95 * t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -9.95 * t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -9.95 * t2+(float)floorz*2.*t2, -7 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-5 * t2, -9.95 * t2+(float)floorz*2.*t2, -5 * t2);
    glVertex3f(5 * t2, -9.95 * t2+(float)floorz*2.*t2, -5 * t2);
    glVertex3f(5 * t2, -9.95 * t2+(float)floorz*2.*t2, 5 * t2);
    glVertex3f(-5 * t2, -9.95 * t2+(float)floorz*2.*t2, 5 * t2);
    glVertex3f(-5 * t2, -9.95 * t2+(float)floorz*2.*t2, -5 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-3 * t2, -9.95 * t2+(float)floorz*2.*t2, -3 * t2);
    glVertex3f(3 * t2, -9.95 * t2+(float)floorz*2.*t2, -3 * t2);
    glVertex3f(3 * t2, -9.95 * t2+(float)floorz*2.*t2, 3 * t2);
    glVertex3f(-3 * t2, -9.95 * t2+(float)floorz*2.*t2, 3 * t2);
    glVertex3f(-3 * t2, -9.95 * t2+(float)floorz*2.*t2, -3 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -9.95 * t2+(float)floorz*2.*t2, -1 * t2);
    glVertex3f(7 * t2, -9.95 * t2+(float)floorz*2.*t2, -1 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(-7 * t2, -9.95 * t2+(float)floorz*2.*t2, 1 * t2);
    glVertex3f(7 * t2, -9.95 * t2+(float)floorz*2.*t2, 1 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(t2, -9.95 * t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(t2, -9.95 * t2+(float)floorz*2.*t2, -7 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(-t2, -9.95 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(-t2, -9.95 * t2+(float)floorz*2.*t2, 7 * t2);
    glEnd();


    glBegin(GL_LINE_STRIP);

    glVertex3f(-7 * t2, -10.05 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -10.05 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(7 * t2, -10.05 * t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -10.05 * t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(-7 * t2, -10.05 * t2+(float)floorz*2.*t2, -7 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-5 * t2, -10.05 * t2+(float)floorz*2.*t2, -5 * t2);
    glVertex3f(5 * t2, -10.05 * t2+(float)floorz*2.*t2, -5 * t2);
    glVertex3f(5 * t2, -10.05 * t2+(float)floorz*2.*t2, 5 * t2);
    glVertex3f(-5 * t2, -10.05 * t2+(float)floorz*2.*t2, 5 * t2);
    glVertex3f(-5 * t2, -10.05 * t2+(float)floorz*2.*t2, -5 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-3 * t2, -10.05 * t2+(float)floorz*2.*t2, -3 * t2);
    glVertex3f(3 * t2, -10.05 * t2+(float)floorz*2.*t2, -3 * t2);
    glVertex3f(3 * t2, -10.05 * t2+(float)floorz*2.*t2, 3 * t2);
    glVertex3f(-3 * t2, -10.05 * t2+(float)floorz*2.*t2, 3 * t2);
    glVertex3f(-3 * t2, -10.05 * t2+(float)floorz*2.*t2, -3 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -10.05 * t2+(float)floorz*2.*t2, -1 * t2);
    glVertex3f(7 * t2, -10.05 * t2+(float)floorz*2.*t2, -1 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(-7 * t2, -10.05 * t2+(float)floorz*2.*t2, 1 * t2);
    glVertex3f(7 * t2, -10.05 * t2+(float)floorz*2.*t2, 1 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(t2, -10.05 * t2+(float)floorz*2.*t2, 7 * t2);
    glVertex3f(t2, -10.05 * t2+(float)floorz*2.*t2, -7 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(-t2, -10.05 * t2+(float)floorz*2.*t2, -7 * t2);
    glVertex3f(-t2, -10.05 * t2+(float)floorz*2.*t2, 7 * t2);
    glEnd();

    /////////////////
    glBegin(GL_LINE_STRIP);

    glVertex3f(-7 * t2, 6 * t2, -7 * t2);
    glVertex3f(7 * t2, 6 * t2, -7 * t2);
    glVertex3f(7 * t2, 6 * t2, 7 * t2);
    glVertex3f(-7 * t2, 6 * t2, 7 * t2);
    glVertex3f(-7 * t2, 6 * t2, -7 * t2);
    glEnd();
if(floorz==0) {
    glBegin(GL_LINE_STRIP);

    glVertex3f(-7 * t2, -9.95 * t2, -7 * t2);
    glVertex3f(7 * t2, -9.95 * t2, -7 * t2);
    glVertex3f(7 * t2, -9.95 * t2, 7 * t2);
    glVertex3f(-7 * t2, -9.95 * t2, 7 * t2);
    glVertex3f(-7 * t2, -9.95 * t2, -7 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-5 * t2, -9.95 * t2, -5 * t2);
    glVertex3f(5 * t2, -9.95 * t2, -5 * t2);
    glVertex3f(5 * t2, -9.95 * t2, 5 * t2);
    glVertex3f(-5 * t2, -9.95 * t2, 5 * t2);
    glVertex3f(-5 * t2, -9.95 * t2, -5 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-3 * t2, -9.95 * t2, -3 * t2);
    glVertex3f(3 * t2, -9.95 * t2, -3 * t2);
    glVertex3f(3 * t2, -9.95 * t2, 3 * t2);
    glVertex3f(-3 * t2, -9.95 * t2, 3 * t2);
    glVertex3f(-3 * t2, -9.95 * t2, -3 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -9.95 * t2, -1 * t2);
    glVertex3f(7 * t2, -9.95 * t2, -1 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -9.95 * t2, 1 * t2);
    glVertex3f(7 * t2, -9.95 * t2, 1 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(t2, -9.95 * t2, 7 * t2);
    glVertex3f(t2, -9.95 * t2, -7 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(-t2, -9.95 * t2, -7 * t2);
    glVertex3f(-t2, -9.95 * t2, 7 * t2);
    glEnd();

}
    glBegin(GL_LINE_STRIP);

    glVertex3f(-7 * t2, -10.05 * t2, -7 * t2);
    glVertex3f(7 * t2, -10.05 * t2, -7 * t2);
    glVertex3f(7 * t2, -10.05 * t2, 7 * t2);
    glVertex3f(-7 * t2, -10.05 * t2, 7 * t2);
    glVertex3f(-7 * t2, -10.05 * t2, -7 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-5 * t2, -10.05 * t2, -5 * t2);
    glVertex3f(5 * t2, -10.05 * t2, -5 * t2);
    glVertex3f(5 * t2, -10.05 * t2, 5 * t2);
    glVertex3f(-5 * t2, -10.05 * t2, 5 * t2);
    glVertex3f(-5 * t2, -10.05 * t2, -5 * t2);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f(-3 * t2, -10.05 * t2, -3 * t2);
    glVertex3f(3 * t2, -10.05 * t2, -3 * t2);
    glVertex3f(3 * t2, -10.05 * t2, 3 * t2);
    glVertex3f(-3 * t2, -10.05 * t2, 3 * t2);
    glVertex3f(-3 * t2, -10.05 * t2, -3 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -10.05 * t2, -1 * t2);
    glVertex3f(7 * t2, -10.05 * t2, -1 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(-7 * t2, -10.05 * t2, 1 * t2);
    glVertex3f(7 * t2, -10.05 * t2, 1 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(t2, -10.05 * t2, 7 * t2);
    glVertex3f(t2, -10.05 * t2, -7 * t2);
    glEnd();glBegin(GL_LINES);
    glVertex3f(-t2, -10.05 * t2, -7 * t2);
    glVertex3f(-t2, -10.05 * t2, 7 * t2);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -10.05 * t2, -7 * t2);
    glVertex3f(-7 * t2, 6 * t2, -7 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(7 * t2, -10.05 * t2, -7 * t2);
    glVertex3f(7 * t2, 6 * t2, -7 * t2);
    glEnd();
    glColor3f(1, 0, 1);
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, -10.05 * t2, 7 * t2);
    glVertex3f(-7 * t2, 6 * t2, 7 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(7 * t2, -10.05 * t2, 7 * t2);
    glVertex3f(7 * t2, 6 * t2, 7 * t2);
    glEnd();
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, 15 * t2, -7 * t2);
    glVertex3f(-7 * t2, 6 * t2, -7 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(7 * t2, 15 * t2, -7 * t2);
    glVertex3f(7 * t2, 6 * t2, -7 * t2);
    glEnd();
    glColor3f(0.5, 0, 0.5);
    glBegin(GL_LINES);
    glVertex3f(-7 * t2, 15 * t2, 7 * t2);
    glVertex3f(-7 * t2, 6 * t2, 7 * t2);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(7 * t2, 15 * t2, 7 * t2);
    glVertex3f(7 * t2, 6 * t2, 7 * t2);
    glEnd();
    unsigned char x, y, z;
    unsigned char t;
    //blk[10][10][30];

    for (x = 0; x<7; x++) {
        for (y = 0; y<7; y++) {
            for (z = 0; z<26; z++) {
                t = blk[x][y][z] & 127;

                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z, t >> 4, (t >> 2) & 3, t & 3, t, t2);
                }
            }
        }
    }
    if(hideblock==0)
    for (x = 0; x<7; x++) {
        for (y = 0; y<7; y++) {
            for (z = 0; z<7; z++) {
                t = nowblock[x][y][z] & 127;


                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x + blockpos[0], y + blockpos[1], z + blockpos[2], t >> 4, (t >> 2) & 3, t & 3, t, t2);
                }
            }
        }
    }

    ////("%d %d %d %d \n",(int)blk[x][y][z],(int)(blk[x][y][z]%64/16),(int)(blk[x][y][z]%16/4),(int)(blk[x][y][z]%4));

    glViewport(0, activitysizey / 40, activitysizex, activitysizey * 41 / 40);
    glLoadIdentity();
    glTranslatef(0, -0.2, -4);
    glScalef(1.8, 1.8, 1.8);
    //glScalef(t2, t2, t2);
    glPushMatrix();
    glTranslatef(-0.1, 0.2,0);

    glColor3f(1, 1, 1);
    graph();
    glPopMatrix();

    {
        double  d1x2, d1y2, d2x2, d2y2;
        double r1, r2;
        r1 = HUGE_VAL;
        r2 = HUGE_VAL;
        d1x2 = (cos(wAngleY / tr)* cos(wAngleZ / tr));
        d1y2 = (cos(wAngleZ / tr)* sin(wAngleX / tr)* sin(wAngleY / tr) + cos(wAngleX / tr)* sin(wAngleZ / tr));

        d2x2 = (sin(wAngleY / tr));
        d2y2 = (-cos(wAngleY / tr)* sin(wAngleX / tr));
        if (d1x2*d1x2 + d1y2 * d1y2 > 0.0001) {
            r1 = atan2(d1y2, d1x2);

        }
        if (d2x2*d2x2 + d2y2 * d2y2 > 0.0001) {
            r2 = atan2(d2y2, d2x2);

        }
        if (r1 == HUGE_VAL) {
            r1 = r2 - PI / 2;
        }
        if (r2 == HUGE_VAL) {
            r2 = r1 - PI / 2;
        }

        if (r1 > r2 &&r1 - r2 > PI) r2 += 2 * PI;
        if (r1 < r2 && r2 - r1 > PI) r1 += 2 * PI;
        if (r1 > r2) {
            r1o = (r1 + r2) / 2 + PI / 4;
            r2o = (r1 + r2) / 2 - PI / 4;
        }
        else {
            r1o = (r1 + r2) / 2 - PI / 4;
            r2o = (r1 + r2) / 2 + PI / 4;
        }

    }

if(spinlock==0) {
    glPushMatrix();
    glTranslatef(-0.45, -1.0, 0);
    //glRotatef(0, 0, 0, 1);
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(0.115, -0.095, 1);
    glVertex3f(0.305, -0.095, 1);
    glVertex3f(0.305, 0.095, 1);
    glVertex3f(0.115, 0.095, 1);
    glVertex3f(0.115, -0.095, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(0.210, 0, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            for (int z = 0; z < 7; z++) {
                int t = rbt[0][0][x][y][z] & 127;


                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                }
            }
        }
    }
    glPopMatrix();
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(0.115, 0.115, 1);
    glVertex3f(0.305, 0.115, 1);
    glVertex3f(0.305, 0.305, 1);
    glVertex3f(0.115, 0.305, 1);
    glVertex3f(0.115, 0.115, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(0.210, 0.210, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            for (int z = 0; z < 7; z++) {
                int t = rbt[0][1][x][y][z] & 127;
                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                }
            }
        }
    }
    glPopMatrix();
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.095, -0.095, 1);
    glVertex3f(0.095, -0.095, 1);
    glVertex3f(0.095, 0.095, 1);
    glVertex3f(-0.095, 0.095, 1);
    glVertex3f(-0.095, -0.095, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(0, 0, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            for (int z = 0; z < 7; z++) {
                int t = rbt[1][0][x][y][z] & 127;
                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                }
            }
        }
    }
    glPopMatrix();
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.095, 0.115, 1);
    glVertex3f(0.095, 0.115, 1);
    glVertex3f(0.095, 0.305, 1);
    glVertex3f(-0.095, 0.305, 1);
    glVertex3f(-0.095, 0.115, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(0, 0.210, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            for (int z = 0; z < 7; z++) {
                int t = rbt[1][1][x][y][z] & 127;
                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                }
            }
        }
    }
    glPopMatrix();
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.115, -0.095, 1);
    glVertex3f(-0.305, -0.095, 1);
    glVertex3f(-0.305, 0.095, 1);
    glVertex3f(-0.115, 0.095, 1);
    glVertex3f(-0.115, -0.095, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(-0.210, 0, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            for (int z = 0; z < 7; z++) {
                int t = rbt[2][0][x][y][z] & 127;
                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                }
            }
        }
    }
    glPopMatrix();
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.115, 0.115, 1);
    glVertex3f(-0.305, 0.115, 1);
    glVertex3f(-0.305, 0.305, 1);
    glVertex3f(-0.115, 0.305, 1);
    glVertex3f(-0.115, 0.115, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(-0.210, 0.210, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    for (int x = 0; x < 7; x++) {
        for (int y = 0; y < 7; y++) {
            for (int z = 0; z < 7; z++) {
                int t = rbt[2][1][x][y][z] & 127;
                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                }
            }
        }
    }

    glPopMatrix();

    glPopMatrix();
}
    if(hidenext==0) {
        glPushMatrix();
        glTranslatef(-0.45, -1.0, 0);
        glColor3f(1, 0, 1);
        glBegin(GL_LINE_STRIP);
        glVertex3f(0.390, 0.120, 1);
        glVertex3f(0.570, 0.120, 1);
        glVertex3f(0.570, 0.300, 1);
        glVertex3f(0.390, 0.300, 1);
        glVertex3f(0.390, 0.120, 1);
        glEnd();
        glPushMatrix();
        glTranslatef(0.480, 0.210, 0);
        glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
        glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
        glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
        for (int x = 0; x < 7; x++) {
            for (int y = 0; y < 7; y++) {
                for (int z = 0; z < 7; z++) {
                    int t = holdblock[x][y][z] & 127;
                    if (t != 0) {
                        (t != 0) ? (t ^= 64) : 1;
                        dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.02);
                    }
                }
            }
        }
        glPopMatrix();
        glPopMatrix();
    }
    glPushMatrix();
    glTranslatef(0.5, -0.925, 0);
    glRotatef(r1o*tr, 0, 0, 1);
    glScalef(1.3,1.3,1.3);
    glColor3f(1, 1, 0);
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.46 - 0.375, -0.99 + 0.925, 1);
    glVertex3f(0.59 - 0.375, -0.99 + 0.925, 1);
    glVertex3f(0.59 - 0.375, -0.86 + 0.925, 1);
    glVertex3f(0.46 - 0.375, -0.86 + 0.925, 1);
    glVertex3f(0.46 - 0.375, -0.99 + 0.925, 1);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.16 - 0.375, -0.99 + 0.925, 1);
    glVertex3f(0.29 - 0.375, -0.99 + 0.925, 1);
    glVertex3f(0.29 - 0.375, -0.86 + 0.925, 1);
    glVertex3f(0.16 - 0.375, -0.86 + 0.925, 1);
    glVertex3f(0.16 - 0.375, -0.99 + 0.925, 1);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.31 - 0.375, -1.01 + 0.925, 1);
    glVertex3f(0.44 - 0.375, -1.01 + 0.925, 1);
    glVertex3f(0.44 - 0.375, -1.14 + 0.925, 1);
    glVertex3f(0.31 - 0.375, -1.14 + 0.925, 1);
    glVertex3f(0.31 - 0.375, -1.01 + 0.925, 1);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.31 - 0.375, -0.84 + 0.925, 1);
    glVertex3f(0.44 - 0.375, -0.84 + 0.925, 1);
    glVertex3f(0.44 - 0.375, -0.71 + 0.925, 1);
    glVertex3f(0.31 - 0.375, -0.71 + 0.925, 1);
    glVertex3f(0.31 - 0.375, -0.84 + 0.925, 1);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.355 - 0.375, -0.795 + 0.925, 1);
    glVertex3f(0.375 - 0.375, -0.775 + 0.925, 1);
    glVertex3f(0.395 - 0.375, -0.795 + 0.925, 1);

    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.355 - 0.375, -1.055 + 0.925, 1);
    glVertex3f(0.375 - 0.375, -1.075 + 0.925, 1);
    glVertex3f(0.395 - 0.375, -1.055 + 0.925, 1);

    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.225 - 0.375, -0.945 + 0.925, 1);
    glVertex3f(0.205 - 0.375, -0.925 + 0.925, 1);
    glVertex3f(0.225 - 0.375, -0.905 + 0.925, 1);

    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.525 - 0.375, -0.945 + 0.925, 1);
    glVertex3f(0.545 - 0.375, -0.925 + 0.925, 1);
    glVertex3f(0.525 - 0.375, -0.905 + 0.925, 1);

    glEnd();
    glPopMatrix();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.13, -0.90, 1);
    glVertex3f(-0.07, -0.90, 1);
    glVertex3f(-0.07, -1.10, 1);
    glVertex3f(0.13, -1.10, 1);
    glVertex3f(0.13, -0.90, 1);
    glEnd();
    glPushMatrix();
    glTranslatef(-0.015,-0.03,0);
    glScalef(1.2,1.2,1.2);

    glBegin(GL_LINE_STRIP);
    glVertex3f(0.4, 0.9, 0);
    glVertex3f(0.415, 0.9, 0);
    glVertex3f(0.415, 0.97, 0);
    glVertex3f(0.4, 0.97, 0);
    glVertex3f(0.4, 0.9, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.43, 0.9, 0);
    glVertex3f(0.445, 0.9, 0);
    glVertex3f(0.445, 0.97, 0);
    glVertex3f(0.43, 0.97, 0);
    glVertex3f(0.43, 0.9, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(0.3475, 0.86, 0);
    glVertex3f(0.4975, 0.86, 0);
    glVertex3f(0.4975, 1.01, 0);
    glVertex3f(0.3475, 1.01, 0);
    glVertex3f(0.3475, 0.86, 0);
    glEnd();
    glPopMatrix();
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    //glEnable(GL_BLEND);
    glViewport(0, 0, activitysizex, activitysizey);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -4);
    glRotatef(0, 0.0f, 0.0f, 0.0f);
    glColor4f(0.4, 0.4, 0.4, 1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

    glVertex3f(-10., 20., -9.9);
    glVertex3f(-10., -20., -9.9);
    glVertex3f(10., -20., -9.9);
    glVertex3f(10., 20., -9.9);
    glVertexPointer(3, GL_FLOAT, 0, glbuffer);
    glTexCoordPointer(2, GL_SHORT, 0, texcoord);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(gltemp, 0, glcounter);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
    //glFinish();
    eglSwapBuffers();
    END:;
}
void draw3() {
    glLineWidth(2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glViewport(0, activitysizey / 40, activitysizex, activitysizey * 41 / 40);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -4);

    glRotatef(0, 0.0f, 0.0f, 0.0f);
    glColor3f(0.5, 0.5, 0.5);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

    glVertex3f(-1., 2., -1.);
    glVertex3f(-1., -2., -1.);
    glVertex3f(1., -2., -1.);
    glVertex3f(1., 2., -1.);
    glVertexPointer(3, GL_FLOAT, 0, glbuffer);
    glTexCoordPointer(2, GL_SHORT, 0, texcoord);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(gltemp, 0, glcounter);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    float xt = -0.35, yt = -1.2;

    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.55, -1.35, 0);
    glVertex3f(0.55, -1.35, 0);
    glVertex3f(0.55, -1.05, 0);
    glVertex3f(-0.55, -1.05, 0);
    glVertex3f(-0.55, -1.35, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.05, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();//m
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.05, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt + 0.025, yt, 0);
    glVertex3f(xt + 0.075, yt, 0);
    glEnd();//a
    xt += 0.2;
    glBegin(GL_LINES);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt + 0.05, yt + 0.1, 0);
    glVertex3f(xt + 0.05, yt - 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();//i
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();//n
    xt = -0.45;yt = -0.8;

    glBegin(GL_LINE_STRIP);
    glVertex3f(-0.55, -0.95, 0);
    glVertex3f(0.55, -0.95, 0);
    glVertex3f(0.55, -0.65, 0);
    glVertex3f(-0.55, -0.65, 0);
    glVertex3f(-0.55, -0.95, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);

    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINES);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt + 0.05, yt + 0.1, 0);
    glVertex3f(xt + 0.05, yt - 0.1, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);

    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.05, yt + 0.03, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt + 0.05, yt + 0.03, 0);
    glVertex3f(xt + 0.05, yt - 0.1, 0);
    glEnd();

    glLineWidth(4.0);
    xt = -0.8;yt = 0.5;
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glVertex3f(xt + 0.05, yt, 0);
    glEnd();//g
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.05, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(xt + 0.025, yt, 0);
    glVertex3f(xt + 0.075, yt, 0);
    glEnd();//a
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.05, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();//m
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glEnd();


    xt += 0.4;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.05, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glEnd();
    xt += 0.2;

    glBegin(GL_LINE_STRIP);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glEnd();
    xt += 0.2;
    glBegin(GL_LINE_STRIP);

    glVertex3f(xt, yt - 0.1, 0);
    glVertex3f(xt, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt + 0.1, 0);
    glVertex3f(xt + 0.1, yt, 0);
    glVertex3f(xt, yt, 0);
    glVertex3f(xt + 0.1, yt - 0.1, 0);

    glEnd();
    yt -= 0.3;
#define glVertex3f3(x,y,z) glVertex3f(-3+x,y+0.3+yt,z)
#define glVertex3f4(x,y,z) glVertex3f3(1.5+x,y+0.3,z)
    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    glVertex3f3(17 * 0.1, -2 * 0.1, 0);
    glVertex3f3(16 * 0.1, -2 * 0.1, 0);
    glVertex3f3(16 * 0.1, -3 * 0.1, 0);
    glVertex3f3(17 * 0.1, -3 * 0.1, 0);
    glVertex3f3(17 * 0.1, -4 * 0.1, 0);
    glVertex3f3(16 * 0.1, -4 * 0.1, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f3(19 * 0.1, -2 * 0.1, 0);
    glVertex3f3(18 * 0.1, -2 * 0.1, 0);
    glVertex3f3(18 * 0.1, -4 * 0.1, 0);
    glVertex3f3(19 * 0.1, -4 * 0.1, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f3(21 * 0.1, -2 * 0.1, 0);
    glVertex3f3(20 * 0.1, -2 * 0.1, 0);
    glVertex3f3(20 * 0.1, -4 * 0.1, 0);
    glVertex3f3(21 * 0.1, -4 * 0.1, 0);
    glVertex3f3(21 * 0.1, -2 * 0.1, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f3(22 * 0.1, -4 * 0.1, 0);
    glVertex3f3(22 * 0.1, -2 * 0.1, 0);
    glVertex3f3(23 * 0.1, -2 * 0.1, 0);
    glVertex3f3(23 * 0.1, -3 * 0.1, 0);
    glVertex3f3(22 * 0.1, -3 * 0.1, 0);
    glVertex3f3(22.5*0.1, -3 * 0.1, 0);
    glVertex3f3(23 * 0.1, -4 * 0.1, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f3(25 * 0.1, -2 * 0.1, 0);
    glVertex3f3(24 * 0.1, -2 * 0.1, 0);
    glVertex3f3(24 * 0.1, -3 * 0.1, 0);
    glVertex3f3(25 * 0.1, -3 * 0.1, 0);
    glVertex3f3(24 * 0.1, -3 * 0.1, 0);
    glVertex3f3(24 * 0.1, -4 * 0.1, 0);
    glVertex3f3(25 * 0.1, -4 * 0.1, 0);
    glEnd();
    glBegin(GL_POINTS);
    glVertex3f3(26 * 0.1, -2.5*0.1, 0);
    glVertex3f3(26 * 0.1, -3.5*0.1, 0);
    glEnd();


    char sct[30];
    sprintf(sct,"!!!!!!!!!!!!!!!!!!");
    sprintf(sct+ilog(oscore)/2, "%d", (int)(oscore % 1000000000));
    //("%s\n", sct);

    for (int w = 0;w < 9;w++) {
        switch (sct[w] - '0') {
            case 0:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glEnd();
                break;
            case 1:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((12.5 + 2 * (float)w)*0.1, -5 * 0.1, 0);
                glVertex3f4((12.5 + 2 * (float)w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
            case 2:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
            case 3:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
            case 4:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
            case 5:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
            case 6:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glEnd();
                break;
            case 7:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
            case 8:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glEnd();
                break;
            case 9:
                glBegin(GL_LINE_STRIP);
                glVertex3f4((float)(13 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -6 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -5 * 0.1, 0);
                glVertex3f4((float)(13 + 2 * w)*0.1, -7 * 0.1, 0);
                glVertex3f4((float)(12 + 2 * w)*0.1, -7 * 0.1, 0);
                glEnd();
                break;
        }


    }

    glPopMatrix();
    eglSwapBuffers();
}
void draw4() {
    glLineWidth(2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glViewport(0, activitysizey / 40, activitysizex, activitysizey * 41 / 40);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -4);

    glRotatef(0, 0.0f, 0.0f, 0.0f);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glEnable(GL_BLEND);

    glEnable(GL_TEXTURE_2D);
    switch (about) {
        case 0:
            glColor4f(0.5, 0.5, 0.5, 1);
            glBindTexture(GL_TEXTURE_2D, texture[0]);
            glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

            glVertex3f(-1., 2., -1.);
            glVertex3f(-1., -2., -1.);
            glVertex3f(1., -2., -1.);
            glVertex3f(1., 2., -1.);
            break;
        case 1:
            glColor4f(1, 1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, texture[2]);
            glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

            glVertex3f(-1.5, 2.25, -1.);
            glVertex3f(-1.5, -3.75, -1.);
            glVertex3f(1.5, -3.75, -1.);
            glVertex3f(1.5, 2.25, -1.);
            break;
        case 2:
            glColor4f(1, 1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, texture[3]);
            glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

            glVertex3f(-1.5, 3.075, -1.);
            glVertex3f(-1.5, -2.925, -1.);
            glVertex3f(1.5, -2.925, -1.);
            glVertex3f(1.5, 3.075, -1.);
            break;
        case 3:
            glColor4f(1, 1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, texture[4]);
            glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

            glVertex3f(-1.5, 3.075, -1.);
            glVertex3f(-1.5, -2.925, -1.);
            glVertex3f(1.5, -2.925, -1.);
            glVertex3f(1.5, 3.075, -1.);
            break;
        default:
            glColor4f(1, 1, 1, 1);
            glBindTexture(GL_TEXTURE_2D, texture[5]);
            glcounter = 0;gltemp = GL_TRIANGLE_FAN;glEnableClientState(GL_VERTEX_ARRAY);

            glVertex3f(-1.5, 3.075, -1.);
            glVertex3f(-1.5, -2.925, -1.);
            glVertex3f(1.5, -2.925, -1.);
            glVertex3f(1.5, 3.075, -1.);
            break;
    }


    glVertexPointer(3, GL_FLOAT, 0, glbuffer);
    glTexCoordPointer(2, GL_SHORT, 0, texcoord);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(gltemp, 0, glcounter);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
    float xt, yt;
    glColor3f(1, 1, 1);
    if (!about) {
        glBegin(GL_LINE_STRIP);
        glVertex3f(-0.55, -1.37, 0);
        glVertex3f(0.55, -1.37, 0);
        glVertex3f(0.55, -1.07, 0);
        glVertex3f(-0.55, -1.07, 0);
        glVertex3f(-0.55, -1.37, 0);
        glEnd();
        xt = -0.45;yt = -1.22;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.05, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(xt + 0.025, yt, 0);
        glVertex3f(xt + 0.075, yt, 0);
        glEnd();//a
        xt += 0.2;
        glBegin(GL_LINE_STRIP);

        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.07, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.05, 0);
        glVertex3f(xt + 0.07, yt - 0, 0);
        glVertex3f(xt + 0.1, yt + 0.05, 0);
        glVertex3f(xt + 0.07, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(xt, yt, 0);
        glVertex3f(xt + 0.07, yt, 0);
        glEnd();//b
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();//o
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();//u
        xt += 0.2;
        glBegin(GL_LINES);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(xt + 0.05, yt + 0.1, 0);
        glVertex3f(xt + 0.05, yt - 0.1, 0);
        glEnd();//t
        xt = -0.45, yt = -0.8;
        //glColor3f(1,1,1);
        glBegin(GL_LINE_STRIP);
        glVertex3f(-0.55, -0.95, 0);
        glVertex3f(0.55, -0.95, 0);
        glVertex3f(0.55, -0.65, 0);
        glVertex3f(-0.55, -0.65, 0);
        glVertex3f(-0.55, -0.95, 0);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt, 0);
        glVertex3f(xt + 0.1, yt, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glEnd();//s
        xt += 0.2;
        glBegin(GL_LINES);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(xt + 0.05, yt + 0.1, 0);
        glVertex3f(xt + 0.05, yt - 0.1, 0);
        glEnd();//t
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.05, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(xt + 0.025, yt, 0);
        glVertex3f(xt + 0.075, yt, 0);
        glEnd();//a
        xt += 0.2;
        glBegin(GL_LINE_STRIP);

        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt, 0);
        glVertex3f(xt, yt, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glEnd();
        xt += 0.2;
        glBegin(GL_LINES);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(xt + 0.05, yt + 0.1, 0);
        glVertex3f(xt + 0.05, yt - 0.1, 0);
        glEnd();
    }
    if (about<2) {
        glLineWidth(4.0);
        xt = -0.8;yt = 1.8;
        glColor3f(1, 1, 1);
        glBegin(GL_LINE_STRIP);

        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt + 0.1, yt, 0);
        glVertex3f(xt, yt, 0);//p


        glEnd();
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();//o
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glEnd();//l
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt + 0.05, yt + 0.04, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(xt + 0.05, yt + 0.04, 0);
        glVertex3f(xt + 0.05, yt - 0.1, 0);
        glEnd();//y


        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glEnd();//c
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glEnd();//u
        xt += 0.2;

        glBegin(GL_LINE_STRIP);

        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.07, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.05, 0);
        glVertex3f(xt + 0.07, yt - 0, 0);
        glVertex3f(xt + 0.1, yt + 0.05, 0);
        glVertex3f(xt + 0.07, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(xt, yt, 0);
        glVertex3f(xt + 0.07, yt, 0);
        glEnd();//b
        xt += 0.2;
        glBegin(GL_LINE_STRIP);
        glVertex3f(xt + 0.1, yt + 0.1, 0);
        glVertex3f(xt, yt + 0.1, 0);
        glVertex3f(xt, yt - 0.1, 0);
        glVertex3f(xt + 0.1, yt - 0.1, 0);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(xt, yt, 0);
        glVertex3f(xt + 0.1, yt, 0);
        glEnd();
        yt -= 0.3;
    }
    glPopMatrix();
    eglSwapBuffers();
}
void graph() {

    glBegin(GL_LINE_STRIP);//s
    glVertex3f(17 * 0.03, -2 * 0.03, 0);
    glVertex3f(16 * 0.03, -2 * 0.03, 0);
    glVertex3f(16 * 0.03, -3 * 0.03, 0);
    glVertex3f(17 * 0.03, -3 * 0.03, 0);
    glVertex3f(17 * 0.03, -4 * 0.03, 0);
    glVertex3f(16 * 0.03, -4 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//c
    glVertex3f(19 * 0.03, -2 * 0.03, 0);
    glVertex3f(18 * 0.03, -2 * 0.03, 0);
    glVertex3f(18 * 0.03, -4 * 0.03, 0);
    glVertex3f(19 * 0.03, -4 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//o
    glVertex3f(21 * 0.03, -2 * 0.03, 0);
    glVertex3f(20 * 0.03, -2 * 0.03, 0);
    glVertex3f(20 * 0.03, -4 * 0.03, 0);
    glVertex3f(21 * 0.03, -4 * 0.03, 0);
    glVertex3f(21 * 0.03, -2 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//r
    glVertex3f(22 * 0.03, -4 * 0.03, 0);
    glVertex3f(22 * 0.03, -2 * 0.03, 0);
    glVertex3f(23 * 0.03, -2 * 0.03, 0);
    glVertex3f(23 * 0.03, -3 * 0.03, 0);
    glVertex3f(22 * 0.03, -3 * 0.03, 0);
    glVertex3f(22.5*0.03, -3 * 0.03, 0);
    glVertex3f(23 * 0.03, -4 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//e
    glVertex3f(25 * 0.03, -2 * 0.03, 0);
    glVertex3f(24 * 0.03, -2 * 0.03, 0);
    glVertex3f(24 * 0.03, -3 * 0.03, 0);
    glVertex3f(25 * 0.03, -3 * 0.03, 0);
    glVertex3f(24 * 0.03, -3 * 0.03, 0);
    glVertex3f(24 * 0.03, -4 * 0.03, 0);
    glVertex3f(25 * 0.03, -4 * 0.03, 0);
    glEnd();
    glBegin(GL_POINTS);//:
    glVertex3f(26 * 0.03, -2.5*0.03, 0);
    glVertex3f(26 * 0.03, -3.5*0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//l
    glVertex3f(16 * 0.03, 5 * 0.03, 0);
    glVertex3f(16 * 0.03, 3 * 0.03, 0);
    glVertex3f(17 * 0.03, 3 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);//i
    glVertex3f(18 * 0.03, 5 * 0.03, 0);
    glVertex3f(19 * 0.03, 5 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(18.5 * 0.03, 5 * 0.03, 0);
    glVertex3f(18.5 * 0.03, 3 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(18 * 0.03, 3 * 0.03, 0);
    glVertex3f(19 * 0.03, 3 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//n
    glVertex3f(20 * 0.03, 3 * 0.03, 0);
    glVertex3f(20 * 0.03, 5 * 0.03, 0);
    glVertex3f(21 * 0.03, 3 * 0.03, 0);
    glVertex3f(21 * 0.03, 5 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);//e
    glVertex3f(23 * 0.03, 3 * 0.03, 0);
    glVertex3f(22 * 0.03, 3 * 0.03, 0);
    glVertex3f(22 * 0.03, 5 * 0.03, 0);
    glVertex3f(23 * 0.03, 5 * 0.03, 0);

    glEnd();
    glBegin(GL_LINES);
    glVertex3f(22 * 0.03, 4 * 0.03, 0);
    glVertex3f(23 * 0.03, 4 * 0.03, 0);

    glEnd();
    glBegin(GL_LINE_STRIP);//s
    glVertex3f(25 * 0.03, 5 * 0.03, 0);
    glVertex3f(24 * 0.03, 5 * 0.03, 0);
    glVertex3f(24 * 0.03, 4 * 0.03, 0);
    glVertex3f(25 * 0.03, 4 * 0.03, 0);
    glVertex3f(25 * 0.03, 3 * 0.03, 0);
    glVertex3f(24 * 0.03, 3 * 0.03, 0);
    glEnd();
    glBegin(GL_POINTS);//:
    glVertex3f(26 * 0.03, 5 * 0.03, 0);
    glVertex3f(26 * 0.03, 3 * 0.03, 0);
    glEnd();
    {
        char sct[30];
        sprintf(sct,"!!!!!!!!!!!!!!!!!!");
        sprintf(sct+ilog(lines)/2, "%d", (int)(lines % 10000000));
        //("%s\n", sct);

        for (int w = 0;w < 7;w++) {
            switch (sct[w] - '0') {
                case 0:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glEnd();
                    break;
                case 1:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((12.5 + 2 * (float)w)*0.03, 2 * 0.03, 0);
                    glVertex3f((12.5 + 2 * (float)w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
                case 2:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
                case 3:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
                case 4:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
                case 5:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
                case 6:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glEnd();
                    break;
                case 7:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
                case 8:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glEnd();
                    break;
                case 9:
                    glBegin(GL_LINE_STRIP);
                    glVertex3f((float)(13 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 1 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 2 * 0.03, 0);
                    glVertex3f((float)(13 + 2 * w)*0.03, 0 * 0.03, 0);
                    glVertex3f((float)(12 + 2 * w)*0.03, 0 * 0.03, 0);
                    glEnd();
                    break;
            }


        }
    }
    if(hidenext==0) {
        glBegin(GL_LINE_STRIP);//n
        glVertex3f(16 * 0.03, 24 * 0.03, 0);
        glVertex3f(16 * 0.03, 26 * 0.03, 0);
        glVertex3f(17 * 0.03, 24 * 0.03, 0);
        glVertex3f(17 * 0.03, 26 * 0.03, 0);

        glEnd();
        glBegin(GL_LINE_STRIP);//e
        glVertex3f(19 * 0.03, 24 * 0.03, 0);
        glVertex3f(18 * 0.03, 24 * 0.03, 0);
        glVertex3f(18 * 0.03, 26 * 0.03, 0);
        glVertex3f(19 * 0.03, 26 * 0.03, 0);

        glEnd();
        glBegin(GL_LINES);
        glVertex3f(19 * 0.03, 25 * 0.03, 0);
        glVertex3f(18 * 0.03, 25 * 0.03, 0);

        glEnd();
        glBegin(GL_LINES);//x
        glVertex3f(21 * 0.03, 26 * 0.03, 0);
        glVertex3f(20 * 0.03, 24 * 0.03, 0);

        glEnd();
        glBegin(GL_LINES);
        glVertex3f(21 * 0.03, 24 * 0.03, 0);
        glVertex3f(20 * 0.03, 26 * 0.03, 0);

        glEnd();
        glBegin(GL_LINES);//t
        glVertex3f(22 * 0.03, 26 * 0.03, 0);
        glVertex3f(23 * 0.03, 26 * 0.03, 0);

        glEnd();
        glBegin(GL_LINES);
        glVertex3f(22.5 * 0.03, 24 * 0.03, 0);
        glVertex3f(22.5 * 0.03, 26 * 0.03, 0);

        glEnd();
        glBegin(GL_POINTS);//:
        glVertex3f(24 * 0.03, 25.5 * 0.03, 0);
        glVertex3f(24 * 0.03, 24.5 * 0.03, 0);
        glEnd();

    }

    char sct[30];
    sprintf(sct,"!!!!!!!!!!!!!!!!!!");
    if(gt)sprintf(sct+ilog(score)/2, "+%d", (int)(gt % 1000000000));

    else sprintf(sct+ilog(score)/2, "%d", (int)(score % 1000000000));



    //("%s\n", sct);

    for (int w = 0;w < 9;w++) {
        switch (sct[w] - '0') {
            case -5:
                glBegin(GL_LINES);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12.5 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12.5 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 0:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glEnd();
                break;
            case 1:
                glBegin(GL_LINE_STRIP);
                glVertex3f((12.5 + 2 * (float)w)*0.03, -5 * 0.03, 0);
                glVertex3f((12.5 + 2 * (float)w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 2:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 3:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 4:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 5:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 6:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glEnd();
                break;
            case 7:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
            case 8:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glEnd();
                break;
            case 9:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -6 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -5 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, -7 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, -7 * 0.03, 0);
                glEnd();
                break;
        }


    }
    glBegin(GL_LINES);
    glVertex3f(12 * 0.03, 11 * 0.03, 0);
    glVertex3f(13 * 0.03, 11 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(12 * 0.03, 10 * 0.03, 0);
    glVertex3f(12 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(13 * 0.03, 10 * 0.03, 0);
    glVertex3f(13 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(14 * 0.03, 10 * 0.03, 0);
    glVertex3f(15 * 0.03, 10 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(14 * 0.03, 12 * 0.03, 0);
    glVertex3f(15 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(14.5 * 0.03, 10 * 0.03, 0);
    glVertex3f(14.5 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(17 * 0.03, 12 * 0.03, 0);
    glVertex3f(16 * 0.03, 12 * 0.03, 0);
    glVertex3f(16 * 0.03, 10 * 0.03, 0);
    glVertex3f(17 * 0.03, 10 * 0.03, 0);
    glVertex3f(17 * 0.03, 11 * 0.03, 0);
    glVertex3f(16.5 * 0.03, 11 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(18 * 0.03, 11 * 0.03, 0);
    glVertex3f(19 * 0.03, 11 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(18 * 0.03, 10 * 0.03, 0);
    glVertex3f(18 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(19 * 0.03, 10 * 0.03, 0);
    glVertex3f(19 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(21 * 0.03, 12 * 0.03, 0);
    glVertex3f(20 * 0.03, 12 * 0.03, 0);
    glVertex3f(20 * 0.03, 11 * 0.03, 0);
    glVertex3f(21 * 0.03, 11 * 0.03, 0);
    glVertex3f(21 * 0.03, 10 * 0.03, 0);
    glVertex3f(20 * 0.03, 10 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(23 * 0.03, 12 * 0.03, 0);
    glVertex3f(22 * 0.03, 12 * 0.03, 0);
    glVertex3f(22 * 0.03, 10 * 0.03, 0);
    glVertex3f(23 * 0.03, 10 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(25 * 0.03, 12 * 0.03, 0);
    glVertex3f(24 * 0.03, 12 * 0.03, 0);
    glVertex3f(24 * 0.03, 10 * 0.03, 0);
    glVertex3f(25 * 0.03, 10 * 0.03, 0);
    glVertex3f(25 * 0.03, 12 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(26 * 0.03, 10 * 0.03, 0);
    glVertex3f(26 * 0.03, 12 * 0.03, 0);
    glVertex3f(27 * 0.03, 12 * 0.03, 0);
    glVertex3f(27 * 0.03, 11 * 0.03, 0);
    glVertex3f(26 * 0.03, 11 * 0.03, 0);
    glVertex3f(26.5*0.03, 11 * 0.03, 0);
    glVertex3f(27 * 0.03, 10 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(29 * 0.03, 12 * 0.03, 0);
    glVertex3f(28 * 0.03, 12 * 0.03, 0);
    glVertex3f(28 * 0.03, 11 * 0.03, 0);
    glVertex3f(29 * 0.03, 11 * 0.03, 0);
    glVertex3f(28 * 0.03, 11 * 0.03, 0);
    glVertex3f(28 * 0.03, 10 * 0.03, 0);
    glVertex3f(29 * 0.03, 10 * 0.03, 0);
    glEnd();
    glBegin(GL_POINTS);
    glVertex3f(30 * 0.03, 11.5*0.03, 0);
    glVertex3f(30 * 0.03, 10.5*0.03, 0);
    glEnd();
    //	char sct[10];
    sprintf(sct,"!!!!!!!!!!!!!!!!!!");
    sprintf(sct+ilog(oh)/2, "%d", (int)(oh % 1000000000));
    //("%s\n", sct);

    for (int w = 0;w < 9;w++) {
        switch (sct[w] - '0') {
            case 0:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glEnd();
                break;
            case 1:
                glBegin(GL_LINE_STRIP);
                glVertex3f((12.5 + 2 * (float)w)*0.03, 9 * 0.03, 0);
                glVertex3f((12.5 + 2 * (float)w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
            case 2:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
            case 3:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
            case 4:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
            case 5:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
            case 6:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glEnd();
                break;
            case 7:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
            case 8:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glEnd();
                break;
            case 9:
                glBegin(GL_LINE_STRIP);
                glVertex3f((float)(13 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 8 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 9 * 0.03, 0);
                glVertex3f((float)(13 + 2 * w)*0.03, 7 * 0.03, 0);
                glVertex3f((float)(12 + 2 * w)*0.03, 7 * 0.03, 0);
                glEnd();
                break;
        }


    }

    glBegin(GL_LINE_STRIP);
    glVertex3f(18 * 0.03, -8 * 0.03, 0);
    glVertex3f(18 * 0.03, -10 * 0.03, 0);
    glVertex3f(19 * 0.03, -10 * 0.03, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(19.5*0.03, -9 * 0.03, 0);
    glVertex3f(20 * 0.03, -10 * 0.03, 0);
    glVertex3f(20.5*0.03, -9 * 0.03, 0);
    glEnd();glBegin(GL_POINTS);
    glVertex3f(21 * 0.03, -10 * 0.03, 0);
    glEnd();
    switch (level) {
        case 1:
            glBegin(GL_LINE_STRIP);
            glVertex3f((14.5 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((14.5 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
        case 2:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
        case 3:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
        case 4:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
        case 5:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
        case 6:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glEnd();
            break;
        case 7:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
        case 8:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glEnd();
            break;
        default:
            glBegin(GL_LINE_STRIP);
            glVertex3f((float)(15 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -9 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -8 * 0.03, 0);
            glVertex3f((float)(15 + 8)*0.03, -10 * 0.03, 0);
            glVertex3f((float)(14 + 8)*0.03, -10 * 0.03, 0);
            glEnd();
            break;
    }
    {
        glPushMatrix();
        glTranslatef(0.6, -0.55, 0);
        glRotatef(r1o*tr, 0, 0, 1);
        glScalef(0.8,0.8,0.8);
        unsigned char t;
        u = rand() % 15;
        bool failing=0;
        for (int x = 0; x<7; x++) {
            for (int y = 0; y<7; y++) {
                for (int z = 25; z>-1+floorz; z--) {
                    failing=0;
                    t = blk[x][y][z] & 127;
                    if(-blockpos[0]+x>=0 && -blockpos[0]+x<7&&-blockpos[1]+y>=0 && -blockpos[1]+y<7&&-blockpos[2]+z>=0 && -blockpos[2]+z<7&&(nowblock[-blockpos[0]+x][y-blockpos[1]][z-blockpos[2]] & 127)!=0){
                        t = nowblock[-blockpos[0]+x][y-blockpos[1]][z-blockpos[2]] & 127;
                        failing=1;
                    }
                    if (t != 0) {
                        (t != 0) ? (t ^= 64) : 1;
                        float r, g, b;
                        if (u == 0 && failing==1)glColor3f(1. - (float)((t >> 4) + 1.2) / 4.4, 1. - (float)(((t >> 2) & 3) + 1.2) / 4.4, 1. - (float)((t & 3) + 1.2) / 4.4);
                        else
                            glColor3f((float)((t >> 4) + 1.2) / 4.4, (float)(((t >> 2) & 3) + 1.2) / 4.4, (float)((t & 3) + 1.2) / 4.4);
                        glBegin(GL_TRIANGLE_FAN);
                        glVertex3f((x - 3.5) * 0.06, -(y - 3.5) * 0.06, 0);
                        glVertex3f((x - 2.5) * 0.06, -(y - 3.5) * 0.06, 0);
                        glVertex3f((x - 2.5) * 0.06, -(y - 2.5) * 0.06, 0);
                        glVertex3f((x - 3.5) * 0.06, -(y - 2.5) * 0.06, 0);
                        glEnd();
                        break;
                    }
                }
            }
        }


        glColor3f(1, 1, 1);
        for (float i = -3.5; i <= 3.5; i++) {
            glBegin(GL_LINES);
            glVertex3f(-3.5 * 0.06, i * 0.06, 0);
            glVertex3f(3.5 * 0.06, i * 0.06, 0);
            glEnd();
        }
        for (float i = -3.5; i <= 3.5; i++) {
            glBegin(GL_LINES);
            glVertex3f(i * 0.06, -3.5 * 0.06, 0);
            glVertex3f(i * 0.06, 3.5 * 0.06, 0);
            glEnd();
        }
        glPopMatrix();
    }
    glPushMatrix();
    glTranslatef(0.6, 0.55, 0);
    glRotatef(wAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(wAngleY, 0.0f, 1.0f, 0.0f);
    glRotatef(wAngleZ, 0.0f, 0.0f, 1.0f);
    if(hidenext==0)
    for (int x = 0; x<7; x++) {
        for (int y = 0; y<7; y++) {
            for (int z = 0; z<7; z++) {
                int t = nextblock[x][y][z] & 127;
                if (t != 0) {
                    (t != 0) ? (t ^= 64) : 1;
                    dc(x, y, z + 8 - 6.5, t >> 4, (t >> 2) & 3, t & 3, t, 0.03);
                }
            }
        }
    }
    glPopMatrix();



}
//116 2- 0.25% 82
//117 2+ 0.25% 164
//118 vc 1.25% 574 (4*4)
//119 ac 0.05% 590
//104 mono15 0.20% 657
//120~123 bomb 0.6% 854
//124 3- 0.20% 901
//125 + 0.8% 1163
//91 spinlock 0.2% 1228
//102 up del 0.05% 1244
//126 xz del 0.6% 1441 (3)
//105 yz del 0.6% 1638 (3)
//127 bombcretor 0.02% 1644
//65 +2
//66 hide faling block
//69 erase item
//68 score x2
//70 hide next
void dc(float X, float Y, float Z, unsigned char R, unsigned char G, unsigned char B, unsigned char pic, float t) {
    GLfloat x, y, z;
    x = (X - 3.) * 2 * t;
    z = (Y - 3.) * 2 * t;
    y = (Z - 4.5) * 2 * t;
    float r, g, b;
    r = (float)(R + 0.6) / 4.8;
    g = (float)(G + 0.8) / 4.4;
    b = (float)(B + 0.8) / 4.4;
    if (R == 0 && G == 0 && B == 0) {
        r = 0.02;g = 0.02;b = 0.02;
    }
if(pic==95){
    glLineWidth(2.0);
    glColor3f(r, g, b);

glBegin(GL_LINE_STRIP);

glVertex3f(-t + x, t + y, t + z);
glVertex3f(t + x, t + y, t + z);
glVertex3f(t + x, t + y, -t + z);
glVertex3f(-t + x, t + y, -t + z);
glVertex3f(-t + x, t + y, t + z);
glEnd();
glBegin(GL_LINE_STRIP);

glVertex3f(-t + x, -t + y, t + z);
glVertex3f(t + x, -t + y, t + z);
glVertex3f(t + x, -t + y, -t + z);
glVertex3f(-t + x, -t + y, -t + z);
glVertex3f(-t + x, -t + y, t + z);
glEnd();
glBegin(GL_LINE_STRIP);

glVertex3f(t + x, -t + y, t + z);
glVertex3f(t + x, t + y, t + z);
glVertex3f(t + x, t + y, -t + z);
glVertex3f(t + x, -t + y, -t + z);
glVertex3f(t + x, -t + y, t + z);
glEnd();
glBegin(GL_LINE_STRIP);

glVertex3f(-t + x, -t + y, t + z);
glVertex3f(-t + x, t + y, t + z);
glVertex3f(-t + x, t + y, -t + z);
glVertex3f(-t + x, -t + y, -t + z);
glVertex3f(-t + x, -t + y, t + z);
glEnd();
glBegin(GL_LINE_STRIP);

glVertex3f(t + x, -t + y, t + z);
glVertex3f(t + x, t + y, t + z);
glVertex3f(-t + x, t + y, t + z);
glVertex3f(-t + x, -t + y, t + z);
glVertex3f(t + x, -t + y, t + z);
glEnd();
glBegin(GL_LINE_STRIP);

glVertex3f(t + x, -t + y, -t + z);
glVertex3f(t + x, t + y, -t + z);
glVertex3f(-t + x, t + y, -t + z);
glVertex3f(-t + x, -t + y, -t + z);
glVertex3f(t + x, -t + y, -t + z);
glEnd();

    return;
    }

    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_STRIP);

    glVertex3f(-t + x, t + y, t + z);
    glVertex3f(-t + x, -t + y, t + z);
    glVertex3f(t + x, t + y, t + z);
    glVertex3f(t + x, -t + y, t + z);
    glVertex3f(t + x, t + y, -t + z);
    glVertex3f(t + x, -t + y, -t + z);
    glVertex3f(-t + x, t + y, -t + z);
    glVertex3f(-t + x, -t + y, -t + z);
    glVertex3f(-t + x, t + y, t + z);
    glVertex3f(-t + x, -t + y, t + z);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);

    glVertex3f(-t + x, t + y, t + z);
    glVertex3f(t + x, t + y, t + z);
    glVertex3f(t + x, t + y, -t + z);
    glVertex3f(-t + x, t + y, -t + z);
    glEnd();
    glBegin(GL_TRIANGLE_FAN);

    glVertex3f(-t + x, -t + y, t + z);
    glVertex3f(t + x, -t + y, t + z);
    glVertex3f(t + x, -t + y, -t + z);
    glVertex3f(-t + x, -t + y, -t + z);
    glEnd();

    //116-128
    //91 splock 0.5%
    //102 fbhid 0.25%

    glColor3f((double)rand() / 16384., (double)rand() / 16384., (double)rand() / 16384.);


    glLineWidth(2.0);
    if (pic == 27) {
        glColor3f((double)rand() / 16384., (double)rand() / 32768., (double)rand() / 65536.);
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.6 + x, -t*0.6 + y, 1.0005*t + z);
        glVertex3f(t*0.6 + x, t*0.6 + y, 1.0005*t + z);
        glVertex3f(-t*0.6 + x, t*0.6 + y, 1.0005*t + z);
        glVertex3f(-t*0.6 + x, -t*0.6 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.6 + y, 1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.4 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.6 + y, 1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.8 + y, 1.0005*t + z);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(t*0.8 + x, t*0.8 + y, 1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.8 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.8 + x, t*0.8 + y, 1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.8 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.6 + x, -t*0.6 + y, -1.0005*t + z);
        glVertex3f(t*0.6 + x, t*0.6 + y, -1.0005*t + z);
        glVertex3f(-t*0.6 + x, t*0.6 + y, -1.0005*t + z);
        glVertex3f(-t*0.6 + x, -t*0.6 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.6 + y, -1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.4 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.6 + y, -1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.8 + y, -1.0005*t + z);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(t*0.8 + x, t*0.8 + y, -1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.8 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.8 + x, t*0.8 + y, -1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.8 + y, -1.0005*t + z);
        glEnd();

    }
    else if (pic == 38) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.7 + x, 1.0005*t + y, t*0.7 + z);
        glVertex3f(t*0.7 + x, 1.0005*t + y, t*0.7 + z);
        glVertex3f(t*0.7 + x, 1.0005*t + y, -t*0.7 + z);
        glVertex3f(-t*0.7 + x, 1.0005*t + y, -t*0.7 + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, y, t*0.7 + z);
        glVertex3f(1.0005*t + x, t*0.7 + y, t*0.7 + z);
        glVertex3f(1.0005*t + x, t*0.7 + y, -t*0.7 + z);
        glVertex3f(1.0005*t + x, y, -t*0.7 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-1.0005*t + x, y, t*0.7 + z);
        glVertex3f(-1.0005*t + x, t*0.7 + y, t*0.7 + z);
        glVertex3f(-1.0005*t + x, t*0.7 + y, -t*0.7 + z);
        glVertex3f(-1.0005*t + x, y, -t*0.7 + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.7 + x, y, 1.0005*t + z);
        glVertex3f(t*0.7 + x, y, 1.0005*t + z);
        glVertex3f(t*0.7 + x, t*0.7 + y, 1.0005*t + z);
        glVertex3f(-t*0.7 + x, t*0.7 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.7 + x, y, -1.0005*t + z);
        glVertex3f(t*0.7 + x, y, -1.0005*t + z);
        glVertex3f(t*0.7 + x, t*0.7 + y, -1.0005*t + z);
        glVertex3f(-t*0.7 + x, t*0.7 + y, -1.0005*t + z);
        glEnd();
    }
    else if (pic == 40) {
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.6 + x, -1.0005*t + y, t*0.6 + z);
        glVertex3f(t*0.6 + x, -1.0005*t + y, t*0.6 + z);
        glVertex3f(t*0.6 + x, -1.0005*t + y, -t*0.6 + z);
        glVertex3f(-t*0.6 + x, -1.0005*t + y, -t*0.6 + z);
        glVertex3f(-t*0.6 + x, -1.0005*t + y, t*0.6 + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.6 + x, 1.0005*t + y, t*0.6 + z);
        glVertex3f(t*0.6 + x, 1.0005*t + y, t*0.6 + z);
        glVertex3f(t*0.6 + x, 1.0005*t + y, -t*0.6 + z);
        glVertex3f(-t*0.6 + x, 1.0005*t + y, -t*0.6 + z);
        glVertex3f(-t*0.6 + x, 1.0005*t + y, t*0.6 + z);
        glEnd();glBegin(GL_LINE_STRIP);
        glVertex3f(1.0005*t + x, -t*0.6 + y, t*0.6 + z);
        glVertex3f(1.0005*t + x, t*0.6 + y, t*0.6 + z);
        glVertex3f(1.0005*t + x, t*0.6 + y, -t*0.6 + z);
        glVertex3f(1.0005*t + x, -t*0.6 + y, -t*0.6 + z);
        glVertex3f(1.0005*t + x, -t*0.6 + y, t*0.6 + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-1.0005*t + x, -t*0.6 + y, t*0.6 + z);
        glVertex3f(-1.0005*t + x, t*0.6 + y, t*0.6 + z);
        glVertex3f(-1.0005*t + x, t*0.6 + y, -t*0.6 + z);
        glVertex3f(-1.0005*t + x, -t*0.6 + y, -t*0.6 + z);
        glVertex3f(-1.0005*t + x, -t*0.6 + y, t*0.6 + z);
        glEnd();glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.6 + x, -t*0.6 + y, 1.0005*t + z);
        glVertex3f(t*0.6 + x, -t*0.6 + y, 1.0005*t + z);
        glVertex3f(t*0.6 + x, t*0.6 + y, 1.0005*t + z);
        glVertex3f(-t*0.6 + x, t*0.6 + y, 1.0005*t + z);
        glVertex3f(-t*0.6 + x, -t*0.6 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.6 + x, -t*0.6 + y, -1.0005*t + z);
        glVertex3f(t*0.6 + x, -t*0.6 + y, -1.0005*t + z);
        glVertex3f(t*0.6 + x, t*0.6 + y, -1.0005*t + z);
        glVertex3f(-t*0.6 + x, t*0.6 + y, -1.0005*t + z);
        glVertex3f(-t*0.6 + x, -t*0.6 + y, -1.0005*t + z);
        glEnd();
    }
    else if (pic == 56 || pic == 57 || pic == 58 || pic == 59 || pic == 63||pic==96) {
        glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);
        glBegin(GL_LINE_STRIP);
        glVertex3f(x, t + y, t + z);
        glVertex3f(x, -t + y, t + z);
        glVertex3f(x, -t + y, -t + z);
        glVertex3f(x, t + y, -t + z);
        glVertex3f(x, t + y, t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t + x, y, t + z);
        glVertex3f(-t + x, y, t + z);
        glVertex3f(-t + x, y, -t + z);
        glVertex3f(t + x, y, -t + z);
        glVertex3f(t + x, y, t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t + x, t + y, z);
        glVertex3f(-t + x, t + y, z);
        glVertex3f(-t + x, -t + y, z);
        glVertex3f(t + x, -t + y, z);
        glVertex3f(t + x, t + y, z);
        glEnd();
    }

    else if (pic == 62) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.3 + x, t*0.5 + y, -1.005*t + z);
        glVertex3f(x, t*0.7 + y, -1.005*t + z);
        glVertex3f(x, -t*0.7 + y, -1.005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.5 + y, -1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.3 + x, t*0.5 + y, 1.005*t + z);
        glVertex3f(x, t*0.7 + y, 1.005*t + z);
        glVertex3f(x, -t*0.7 + y, 1.005*t + z);
        glVertex3f(t*0.3 + x, -t*0.5 + y, 1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.3 + x, t*0.5 + y, 1.005*t + z);
        glVertex3f(x, t*0.7 + y, 1.005*t + z);
        glVertex3f(x, -t*0.7 + y, 1.005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.5 + y, 1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.5 + x, -t*0.3 + y, -1.005*t + z);
        glVertex3f(t*0.7 + x, y, -1.005*t + z);
        glVertex3f(-t*0.7 + x, y, -1.005*t + z);
        glVertex3f(-t*0.5 + x, t*0.3 + y, -1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.5 + x, t*0.3 + y, -1.005*t + z);
        glVertex3f(t*0.7 + x, y, -1.005*t + z);
        glVertex3f(-t*0.7 + x, y, -1.005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.3 + y, -1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.5 + x, -t*0.3 + y, 1.005*t + z);
        glVertex3f(t*0.7 + x, y, 1.005*t + z);
        glVertex3f(-t*0.7 + x, y, 1.005*t + z);
        glVertex3f(-t*0.5 + x, t*0.3 + y, 1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.5 + x, t*0.3 + y, 1.005*t + z);
        glVertex3f(t*0.7 + x, y, 1.005*t + z);
        glVertex3f(-t*0.7 + x, y, 1.005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.3 + y, 1.005*t + z);
        glEnd();
    }
    else if (pic == 41) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);
        glBegin(GL_LINE_STRIP);

        glVertex3f(-1.005*t + x, t*0.5 + y, -t*0.3 + z);
        glVertex3f(-1.005*t + x, t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.5 + y, t*0.3 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(-1.005*t + x, t*0.5 + y, t*0.3 + z);
        glVertex3f(-1.005*t + x, t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.5 + y, -t*0.3 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(1.005*t + x, t*0.5 + y, -t*0.3 + z);
        glVertex3f(1.005*t + x, t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.5 + y, t*0.3 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(1.005*t + x, t*0.5 + y, t*0.3 + z);
        glVertex3f(1.005*t + x, t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.5 + y, -t*0.3 + z);
        glEnd();

        glBegin(GL_LINE_STRIP);

        glVertex3f(-1.005*t + x, -t*0.3 + y, t*0.5 + z);
        glVertex3f(-1.005*t + x, y, t*0.7 + z);
        glVertex3f(-1.005*t + x, y, -t*0.7 + z);
        glVertex3f(-1.005*t + x, t*0.3 + y, -t*0.5 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(-1.005*t + x, t*0.3 + y, t*0.5 + z);
        glVertex3f(-1.005*t + x, y, t*0.7 + z);
        glVertex3f(-1.005*t + x, y, -t*0.7 + z);
        glVertex3f(-1.005*t + x, -t*0.3 + y, -t*0.5 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(1.005*t + x, -t*0.3 + y, t*0.5 + z);
        glVertex3f(1.005*t + x, y, t*0.7 + z);
        glVertex3f(1.005*t + x, y, -t*0.7 + z);
        glVertex3f(1.005*t + x, t*0.3 + y, -t*0.5 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(1.005*t + x, t*0.3 + y, t*0.5 + z);
        glVertex3f(1.005*t + x, y, t*0.7 + z);
        glVertex3f(1.005*t + x, y, -t*0.7 + z);
        glVertex3f(1.005*t + x, -t*0.3 + y, -t*0.5 + z);
        glEnd();

    }



    else if (pic == 54) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);
        glBegin(GL_LINE_STRIP);

        glVertex3f(-t*0.3 + x, t*0.5 + y, -1.005*t + z);
        glVertex3f(x, t*0.7 + y, -1.005*t + z);
        glVertex3f(x, -t*0.7 + y, -1.005*t + z);
        glVertex3f(t*0.3 + x, -t*0.5 + y, -1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);

        glVertex3f(t*0.3 + x, t*0.5 + y, -1.005*t + z);
        glVertex3f(x, t*0.7 + y, -1.005*t + z);
        glVertex3f(x, -t*0.7 + y, -1.005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.5 + y, -1.005*t + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(-t*0.3 + x, t*0.5 + y, 1.005*t + z);
        glVertex3f(x, t*0.7 + y, 1.005*t + z);
        glVertex3f(x, -t*0.7 + y, 1.005*t + z);
        glVertex3f(t*0.3 + x, -t*0.5 + y, 1.005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);

        glVertex3f(t*0.3 + x, t*0.5 + y, 1.005*t + z);
        glVertex3f(x, t*0.7 + y, 1.005*t + z);
        glVertex3f(x, -t*0.7 + y, 1.005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.5 + y, 1.005*t + z);
        glEnd();

        glBegin(GL_LINE_STRIP);

        glVertex3f(-1.005*t + x, t*0.5 + y, -t*0.3 + z);
        glVertex3f(-1.005*t + x, t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.5 + y, t*0.3 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(-1.005*t + x, t*0.5 + y, t*0.3 + z);
        glVertex3f(-1.005*t + x, t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.7 + y, z);
        glVertex3f(-1.005*t + x, -t*0.5 + y, -t*0.3 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(1.005*t + x, t*0.5 + y, -t*0.3 + z);
        glVertex3f(1.005*t + x, t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.5 + y, t*0.3 + z);
        glEnd();glBegin(GL_LINE_STRIP);

        glVertex3f(1.005*t + x, t*0.5 + y, t*0.3 + z);
        glVertex3f(1.005*t + x, t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.7 + y, z);
        glVertex3f(1.005*t + x, -t*0.5 + y, -t*0.3 + z);
        glEnd();


    }
        else if(pic==66){
        glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);
        glBegin(GL_LINE_STRIP);

        glVertex3f(t*0.3 + x,  -t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.3 + x,  t*0.2 + y,-1.0005*t + z);
        glVertex3f(x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.3 + x,  t*0.2 + y,-1.0005*t + z);
        glVertex3f(-t*0.3 + x,  -t*0.4 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.15 + x,  t*0.1 + y,-1.0005*t + z);
        glVertex3f(t*0.05 + x,  t*0.1 + y,-1.0005*t + z);
        glVertex3f(t*0.05 + x,  -t*0.1+ y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.05 + x,  t*0.1 + y,-1.0005*t + z);
        glVertex3f(-t*0.15 + x,  t*0.1 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);

        glVertex3f(t*0.3 + x,  -t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.3 + x,  t*0.2 + y,1.0005*t + z);
        glVertex3f(x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.3 + x,  t*0.2 + y,1.0005*t + z);
        glVertex3f(-t*0.3 + x,  -t*0.4 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.15 + x,  t*0.1 + y,1.0005*t + z);
        glVertex3f(-t*0.05 + x,  t*0.1 + y,1.0005*t + z);
        glVertex3f(-t*0.05 + x,  -t*0.1+ y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(t*0.05 + x,  t*0.1 + y,1.0005*t + z);
        glVertex3f(t*0.15 + x,  t*0.1 + y,1.0005*t + z);
        glEnd();
    }
    else if(pic==69){
        glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);

        glBegin(GL_LINES);
        glVertex3f(t*0.4 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.2 + x,  t*0.2 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(t*0.4 + x,  -t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.2 + x,  -t*0.2 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.4 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.2 + x,  t*0.2 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.4 + x,  -t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.2 + x,  -t*0.2 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(t*0.4 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.2 + x,  t*0.2 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(t*0.4 + x,  -t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.2 + x,  -t*0.2 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.4 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.2 + x,  t*0.2 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.4 + x,  -t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.2 + x,  -t*0.2 + y,-1.0005*t + z);
        glEnd();
    }
    else if(pic==68){
        glColor3f((double)rand() / 65536., (double)rand() / 16384., (double)rand() / 65536.);

        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.4 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.4 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.4 + x,  -t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.4 + x,  -t*0.4 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(x,  t*0.5 + y,1.0005*t + z);
        glVertex3f(x,  -t*0.5 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(t*0.4 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.4 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.4 + x,  -t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.4 + x,  -t*0.4 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(x,  t*0.5 + y,-1.0005*t + z);
        glVertex3f(x,  -t*0.5 + y,-1.0005*t + z);
        glEnd();
    }
    else if(pic==70){
        glColor3f((double)rand() / 65536., (double)rand() / 16384., (double)rand() / 65536.);

        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.2 + x,  -t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.2 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.2 + x,  -t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.2 + x,  t*0.4 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(t*0.5+x,  t*0.5 + y,1.0005*t + z);
        glVertex3f(-t*0.5+x,  -t*0.5 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.5+x,  t*0.5 + y,1.0005*t + z);
        glVertex3f(t*0.5+x,  -t*0.5 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.2 + x,  -t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.2 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.2 + x,  -t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.2 + x,  t*0.4 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(t*0.5+x,  t*0.5 + y,-1.0005*t + z);
        glVertex3f(-t*0.5+x,  -t*0.5 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(-t*0.5+x,  t*0.5 + y,-1.0005*t + z);
        glVertex3f(t*0.5+x,  -t*0.5 + y,-1.0005*t + z);
        glEnd();
    }
    else if(pic==94){
    glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);
        glBegin(GL_LINES);

        glVertex3f(t*0.3 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(-t*0.3 + x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(x,  t*0.4 + y,-1.0005*t + z);
        glVertex3f(x,  t*0.1 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.3 + x,  t*0.1 + y,-1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.4 + y,-1.0005*t + z);
        glVertex3f(t*0.3 + x,  -t*0.4+ y,-1.0005*t + z);
        glVertex3f(t*0.3 + x,  t*0.1+ y,-1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.1 + y,-1.0005*t + z);
        glEnd();
        glBegin(GL_LINES);

        glVertex3f(t*0.3 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(-t*0.3 + x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(x,  t*0.4 + y,1.0005*t + z);
        glVertex3f(x,  t*0.1 + y,1.0005*t + z);
        glEnd();
        glBegin(GL_LINE_STRIP);
        glVertex3f(-t*0.3 + x,  t*0.1 + y,1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.4 + y,1.0005*t + z);
        glVertex3f(t*0.3 + x,  -t*0.4+ y,1.0005*t + z);
        glVertex3f(t*0.3 + x,  t*0.1+ y,1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.1 + y,1.0005*t + z);
        glEnd();
        }
    else if (pic == 55) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, -1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.5 + x, -1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.5 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.5 + x, -1.0005*t + y, t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, -1.0005*t + y, -t*0.5 + z);
        glVertex3f(-t*0.5 + x, -1.0005*t + y, -t*0.5 + z);
        glVertex3f(-t*0.5 + x, -1.0005*t + y, -t*0.3 + z);
        glVertex3f(t*0.5 + x, -1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.5 + x, -1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.3 + x, -1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.3 + x, -1.0005*t + y, -t*0.5 + z);
        glVertex3f(-t*0.5 + x, -1.0005*t + y, -t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, 1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.5 + x, 1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.5 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.5 + x, 1.0005*t + y, t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, 1.0005*t + y, -t*0.5 + z);
        glVertex3f(-t*0.5 + x, 1.0005*t + y, -t*0.5 + z);
        glVertex3f(-t*0.5 + x, 1.0005*t + y, -t*0.3 + z);
        glVertex3f(t*0.5 + x, 1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.5 + x, 1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.3 + x, 1.0005*t + y, t*0.5 + z);
        glVertex3f(-t*0.3 + x, 1.0005*t + y, -t*0.5 + z);
        glVertex3f(-t*0.5 + x, 1.0005*t + y, -t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, t*0.5 + y, -1.0005*t + z);
        glVertex3f(-t*0.5 + x, t*0.5 + y, -1.0005*t + z);
        glVertex3f(-t*0.5 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.5 + x, t*0.3 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, -t*0.5 + y, -1.0005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.5 + y, -1.0005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.5 + x, -t*0.3 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, t*0.5 + y, -1.0005*t + z);
        glVertex3f(t*0.3 + x, t*0.5 + y, -1.0005*t + z);
        glVertex3f(t*0.3 + x, -t*0.5 + y, -1.0005*t + z);
        glVertex3f(t*0.5 + x, -t*0.5 + y, -1.0005*t + z);
        glEnd();

        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.5 + x, t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.5 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.5 + x, t*0.3 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.5 + x, -t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.5 + x, -t*0.3 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.5 + x, t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.5 + y, 1.0005*t + z);
        glVertex3f(-t*0.5 + x, -t*0.5 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.5 + y, t*0.5 + z);
        glVertex3f(-1.0005*t + x, t*0.5 + y, -t*0.5 + z);
        glVertex3f(-1.0005*t + x, t*0.3 + y, -t*0.5 + z);
        glVertex3f(-1.0005*t + x, t*0.3 + y, t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, -t*0.5 + y, t*0.5 + z);
        glVertex3f(-1.0005*t + x, -t*0.5 + y, -t*0.5 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, -t*0.5 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.5 + y, -t*0.5 + z);
        glVertex3f(-1.0005*t + x, t*0.5 + y, -t*0.3 + z);
        glVertex3f(-1.0005*t + x, -t*0.5 + y, -t*0.3 + z);
        glVertex3f(-1.0005*t + x, -t*0.5 + y, -t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.5 + y, t*0.5 + z);
        glVertex3f(1.0005*t + x, t*0.5 + y, -t*0.5 + z);
        glVertex3f(1.0005*t + x, t*0.3 + y, -t*0.5 + z);
        glVertex3f(1.0005*t + x, t*0.3 + y, t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, -t*0.5 + y, t*0.5 + z);
        glVertex3f(1.0005*t + x, -t*0.5 + y, -t*0.5 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, -t*0.5 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, t*0.5 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.5 + y, t*0.5 + z);
        glVertex3f(1.0005*t + x, t*0.5 + y, t*0.3 + z);
        glVertex3f(1.0005*t + x, -t*0.5 + y, t*0.3 + z);
        glVertex3f(1.0005*t + x, -t*0.5 + y, t*0.5 + z);
        glEnd();

    }
    else if (pic==65) {
    glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(t*0.3 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.3 + x, -t*0.1 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.3 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.3 + x, -t*0.1 + y, 1.0005*t + z);

        glEnd();

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(t*0.1 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.1 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.1 + x, -t*0.3 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.1 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.1 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.1 + x, -t*0.3 + y, 1.0005*t + z);

        glEnd();

}
    else if (pic == 61 ) {
        glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.3 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.3 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.3 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.3 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.3 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.3 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.3 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.3 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(t*0.3 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.3 + x, -t*0.1 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.3 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.3 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.3 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.3 + x, -t*0.1 + y, 1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.3 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.3 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.3 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.3 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.3 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.3 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.1 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(-t*0.1 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(-t*0.1 + x, -1.0005*t + y, -t*0.3 + z);
        glVertex3f(t*0.1 + x, -1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.1 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.1 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.1 + x, 1.0005*t + y, -t*0.3 + z);
        glVertex3f(-t*0.1 + x, 1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(t*0.1 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.1 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.1 + x, -t*0.3 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.1 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.1 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.1 + x, -t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.1 + x, -t*0.3 + y, 1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.3 + y, t*0.1 + z);
        glVertex3f(-1.0005*t + x, t*0.3 + y, -t*0.1 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, -t*0.1 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, t*0.3 + y, t*0.1 + z);
        glVertex3f(1.0005*t + x, t*0.3 + y, -t*0.1 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, -t*0.1 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, t*0.1 + z);
        glEnd();
    }
    else if (pic == 60) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.8 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.4 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.4 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.8 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.4 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.4 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.4 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.4 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.1 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.8 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.4 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.4 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.1 + y, -1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.8 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.8 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.8 + z);
        glEnd();


        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.4 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.4 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.8 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.4 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.4 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.4 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.4 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.1 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.8 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.4 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.4 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.1 + y, 1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.8 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.8 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.8 + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.2 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.2 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.2 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.2 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();	glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.2 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.2 + x, -t*0.1 + y, -1.0005*t + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.2 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.2 + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.2 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.2 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.2 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.2 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.2 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.2 + x, -t*0.1 + y, 1.0005*t + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.2 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.2 + z);
        glEnd();
    }
    else if (pic == 52) {
        glColor3f((double)rand() / 65536., (double)rand() / 65536., (double)rand() / 16384.);
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.8 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.8 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.2 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.2 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.2 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.2 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.1 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.8 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.1 + y, -1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.8 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.8 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.8 + z);
        glEnd();


        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.8 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.2 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.2 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.2 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.2 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.1 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.8 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.1 + y, 1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.8 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.8 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.8 + z);
        glEnd();
    }
    else if (pic == 53) {
        glColor3f((double)rand() / 16384., (double)rand() / 65536., (double)rand() / 65536.);
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.8 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.8 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();

        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.2 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, -1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, -1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.2 + x, -1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.2 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.2 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.1 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.8 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.1 + y, -1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.1 + y, -1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.8 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, -t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, -t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.8 + z);
        glVertex3f(-1.0005*t + x, t*0.1 + y, t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.2 + z);
        glVertex3f(-1.0005*t + x, -t*0.1 + y, t*0.8 + z);
        glEnd();


        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(-t*0.2 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(-t*0.8 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.2 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, 1.0005*t + y, t*0.1 + z);
        glVertex3f(t*0.8 + x, 1.0005*t + y, -t*0.1 + z);
        glVertex3f(t*0.2 + x, 1.0005*t + y, -t*0.1 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.8 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.2 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.2 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(-t*0.8 + x, -t*0.1 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.8 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.2 + x, -t*0.1 + y, 1.0005*t + z);
        glVertex3f(t*0.8 + x, -t*0.1 + y, 1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.8 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, -t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, -t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.8 + z);
        glVertex3f(1.0005*t + x, t*0.1 + y, t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.2 + z);
        glVertex3f(1.0005*t + x, -t*0.1 + y, t*0.8 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-t*0.6 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(-t*0.4 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(-t*0.4 + x, -1.0005*t + y, -t*0.3 + z);
        glVertex3f(-t*0.6 + x, -1.0005*t + y, -t*0.3 + z);
        glEnd();

        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.4 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.6 + x, -1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.6 + x, -1.0005*t + y, -t*0.3 + z);
        glVertex3f(t*0.4 + x, -1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.6 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.4 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.4 + x, -t*0.3 + y, -1.0005*t + z);
        glVertex3f(-t*0.6 + x, -t*0.3 + y, -1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.6 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.4 + x, t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.4 + x, -t*0.3 + y, -1.0005*t + z);
        glVertex3f(t*0.6 + x, -t*0.3 + y, -1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(-1.0005*t + x, t*0.3 + y, -t*0.6 + z);
        glVertex3f(-1.0005*t + x, t*0.3 + y, -t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, -t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, -t*0.6 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-1.0005*t + x, t*0.3 + y, t*0.6 + z);
        glVertex3f(-1.0005*t + x, t*0.3 + y, t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, t*0.4 + z);
        glVertex3f(-1.0005*t + x, -t*0.3 + y, t*0.6 + z);
        glEnd();


        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.6 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(-t*0.4 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(-t*0.4 + x, 1.0005*t + y, -t*0.3 + z);
        glVertex3f(-t*0.6 + x, 1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.4 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.6 + x, 1.0005*t + y, t*0.3 + z);
        glVertex3f(t*0.6 + x, 1.0005*t + y, -t*0.3 + z);
        glVertex3f(t*0.4 + x, 1.0005*t + y, -t*0.3 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-t*0.6 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.4 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.4 + x, -t*0.3 + y, 1.0005*t + z);
        glVertex3f(-t*0.6 + x, -t*0.3 + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(t*0.6 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.4 + x, t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.4 + x, -t*0.3 + y, 1.0005*t + z);
        glVertex3f(t*0.6 + x, -t*0.3 + y, 1.0005*t + z);

        glEnd();
        glBegin(GL_TRIANGLE_FAN);

        glVertex3f(1.0005*t + x, t*0.3 + y, -t*0.6 + z);
        glVertex3f(1.0005*t + x, t*0.3 + y, -t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, -t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, -t*0.6 + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, t*0.3 + y, t*0.6 + z);
        glVertex3f(1.0005*t + x, t*0.3 + y, t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, t*0.4 + z);
        glVertex3f(1.0005*t + x, -t*0.3 + y, t*0.6 + z);
        glEnd();
    }
    else {
        float tlf = t*0.85;

        glBegin(GL_TRIANGLE_FAN);
        glColor3f(r*1.2, g*1.2, b*1.2);
        glVertex3f(-tlf + x, -1.0005*t + y, tlf + z);
        glVertex3f(tlf + x, -1.0005*t + y, tlf + z);
        glVertex3f(tlf + x, -1.0005*t + y, -tlf + z);
        glVertex3f(-tlf + x, -1.0005*t + y, -tlf + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-tlf + x, 1.0005*t + y, tlf + z);
        glVertex3f(tlf + x, 1.0005*t + y, tlf + z);
        glVertex3f(tlf + x, 1.0005*t + y, -tlf + z);
        glVertex3f(-tlf + x, 1.0005*t + y, -tlf + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);
        glVertex3f(1.0005*t + x, -tlf + y, tlf + z);
        glVertex3f(1.0005*t + x, tlf + y, tlf + z);
        glVertex3f(1.0005*t + x, tlf + y, -tlf + z);
        glVertex3f(1.0005*t + x, -tlf + y, -tlf + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-1.0005*t + x, -tlf + y, tlf + z);
        glVertex3f(-1.0005*t + x, tlf + y, tlf + z);
        glVertex3f(-1.0005*t + x, tlf + y, -tlf + z);
        glVertex3f(-1.0005*t + x, -tlf + y, -tlf + z);
        glEnd();glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-tlf + x, -tlf + y, 1.0005*t + z);
        glVertex3f(tlf + x, -tlf + y, 1.0005*t + z);
        glVertex3f(tlf + x, tlf + y, 1.0005*t + z);
        glVertex3f(-tlf + x, tlf + y, 1.0005*t + z);
        glEnd();
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(-tlf + x, -tlf + y, -1.0005*t + z);
        glVertex3f(tlf + x, -tlf + y, -1.0005*t + z);
        glVertex3f(tlf + x, tlf + y, -1.0005*t + z);
        glVertex3f(-tlf + x, tlf + y, -1.0005*t + z);
        glEnd();
    }
    glLineWidth(2.0);
}


void gover() {
    oscore = score;
    //exit(0);
    //MessageBox(0, TEXT("망함 ㅋ"), TEXT("cube"), MB_OK);
    LOGI("a");
    FILE *f = fopen(dir, "rb");
    long long t[2], v;
    if (f != NULL) {
        LOGI("b");
        fread(t, 8, 2, f);
        fclose(f);
        LOGI("d");
        v = t[0] * 51231 % 134 + t[0] * 12241 % 142 + t[0] * 1411 % 131 + t[0] * 215 % 13 + t[0] * 2;
        if (v == t[1]) {
            if (t[0] < score) {
                LOGI("g");
                FILE *f = fopen(dir, "wb");
                t[0] = score;
                t[1] = t[0] * 51231 % 134 + t[0] * 12241 % 142 + t[0] * 1411 % 131 + t[0] * 215 % 13 + t[0] * 2;
                LOGI("i");
                fwrite(t, 8, 2, f);
                LOGI("k");
                fclose(f);
                LOGI("m");
            }

        }
        else {
            LOGI("h");

            FILE *f = fopen(dir, "wb");

            t[0] = score;
            t[1] = t[0] * 51231 % 134 + t[0] * 12241 % 142 + t[0] * 1411 % 131 + t[0] * 215 % 13 + t[0] * 2;
            LOGI("j");
            fwrite(t, 8, 2, f);
            LOGI("l");
            fclose(f);
            LOGI("n");
        }

    }
    else {
        LOGI("c");
        FILE *f = fopen(dir, "wb");
        LOGI("!!!!! %d", f);
        t[0] = score;
        t[1] = t[0] * 51231 % 134 + t[0] * 12241 % 142 + t[0] * 1411 % 131 + t[0] * 215 % 13 + t[0] * 2;
        fwrite(t, 8, 2, f);
        LOGI("e");
        fclose(f);
        LOGI("f");
    }
    goverflg = 1;

}
