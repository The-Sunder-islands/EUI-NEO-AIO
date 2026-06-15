#ifndef GLFW_SHIM_GLFW3_H
#define GLFW_SHIM_GLFW3_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Vulkan forward declarations (minimal, no vulkan.h dependency) */
typedef struct VkInstance_T*          VkInstance;
typedef struct VkSurfaceKHR_T*        VkSurfaceKHR;
typedef int                           VkResult;
struct VkAllocationCallbacks;
typedef void* (*PFN_vkGetInstanceProcAddr)(VkInstance, const char*);
#define GLFWAPI

#define VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR 1000008000

typedef struct GLFWwindow  GLFWwindow;
typedef struct GLFWmonitor GLFWmonitor;
typedef struct GLFWcursor  GLFWcursor;
typedef void (*GLFWglproc)(void);

/* GLFWvidmode MUST be before its first use */
typedef struct GLFWvidmode {
    int width;
    int height;
    int redBits;
    int greenBits;
    int blueBits;
    int refreshRate;
} GLFWvidmode;

typedef struct GLFWimage {
    int width;
    int height;
    unsigned char* pixels;
} GLFWimage;

#define GLFW_TRUE  1
#define GLFW_FALSE 0
#define GLFW_PRESS  1
#define GLFW_RELEASE 0
#define GLFW_REPEAT 2
#define GLFW_KEY_ESCAPE     256
#define GLFW_KEY_ENTER      257
#define GLFW_KEY_KP_ENTER   335
#define GLFW_KEY_TAB        258
#define GLFW_KEY_BACKSPACE  259
#define GLFW_KEY_DELETE     261
#define GLFW_KEY_RIGHT      262
#define GLFW_KEY_LEFT       263
#define GLFW_KEY_DOWN       264
#define GLFW_KEY_UP         265
#define GLFW_KEY_HOME       268
#define GLFW_KEY_END        269
#define GLFW_KEY_A          65
#define GLFW_KEY_C          67
#define GLFW_KEY_V          86
#define GLFW_KEY_X          88
#define GLFW_KEY_Z          90
#define GLFW_KEY_Y          89
#define GLFW_KEY_BACKSLASH  92
#define GLFW_MOD_SHIFT      0x0001
#define GLFW_MOD_CONTROL    0x0002
#define GLFW_MOD_ALT        0x0004
#define GLFW_MOD_SUPER      0x0008
#define GLFW_MOUSE_BUTTON_LEFT   0
#define GLFW_MOUSE_BUTTON_RIGHT  1
#define GLFW_MOUSE_BUTTON_MIDDLE 2
#define GLFW_CURSOR          0x00033001
#define GLFW_STICKY_KEYS     0x00033002
#define GLFW_STICKY_MOUSE_BUTTONS 0x00033003
#define GLFW_CURSOR_NORMAL   0x00034001
#define GLFW_CURSOR_HIDDEN   0x00034002
#define GLFW_ARROW_CURSOR    0x00036001
#define GLFW_IBEAM_CURSOR    0x00036002
#define GLFW_HAND_CURSOR     0x00036004
#define GLFW_DONT_CARE       -1
#define GLFW_RESIZABLE       0x00020003
#define GLFW_VISIBLE         0x00020004
#define GLFW_DECORATED       0x00020005
#define GLFW_FOCUSED         0x00020001
#define GLFW_FLOATING        0x00020007
#define GLFW_MAXIMIZED       0x00020008
#define GLFW_TRANSPARENT_FRAMEBUFFER 0x0002000A
#define GLFW_RED_BITS        0x00021001
#define GLFW_GREEN_BITS      0x00021002
#define GLFW_BLUE_BITS       0x00021003
#define GLFW_ALPHA_BITS      0x00021004
#define GLFW_DEPTH_BITS      0x00021005
#define GLFW_STENCIL_BITS    0x00021006
#define GLFW_SAMPLES         0x0002100D
#define GLFW_REFRESH_RATE    0x0002100F
#define GLFW_CLIENT_API      0x00022001
#define GLFW_CONTEXT_VERSION_MAJOR  0x00022002
#define GLFW_CONTEXT_VERSION_MINOR  0x00022003
#define GLFW_OPENGL_PROFILE  0x00022008
#define GLFW_OPENGL_API      0x00030001
#define GLFW_OPENGL_CORE_PROFILE    0x00032001
#define GLFW_NO_API          0

int  glfwInit(void);
void glfwTerminate(void);
void glfwInitHint(int hint, int value);
void glfwWindowHint(int hint, int value);
void glfwDefaultWindowHints(void);

GLFWwindow* glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share);
void        glfwDestroyWindow(GLFWwindow* window);
int         glfwWindowShouldClose(GLFWwindow* window);
void        glfwSetWindowShouldClose(GLFWwindow* window, int value);
void        glfwSetWindowTitle(GLFWwindow* window, const char* title);
void        glfwSetWindowUserPointer(GLFWwindow* window, void* pointer);
void*       glfwGetWindowUserPointer(GLFWwindow* window);
void        glfwSetWindowSize(GLFWwindow* window, int width, int height);
void        glfwGetWindowSize(GLFWwindow* window, int* width, int* height);
void        glfwGetFramebufferSize(GLFWwindow* window, int* width, int* height);
void        glfwGetWindowPos(GLFWwindow* window, int* xpos, int* ypos);
void        glfwSetWindowPos(GLFWwindow* window, int xpos, int ypos);
void        glfwGetWindowContentScale(GLFWwindow* window, float* xscale, float* yscale);
void        glfwShowWindow(GLFWwindow* window);
void        glfwHideWindow(GLFWwindow* window);
void        glfwIconifyWindow(GLFWwindow* window);
void        glfwRestoreWindow(GLFWwindow* window);
void        glfwMaximizeWindow(GLFWwindow* window);
void        glfwFocusWindow(GLFWwindow* window);

GLFWmonitor*       glfwGetPrimaryMonitor(void);
GLFWmonitor**      glfwGetMonitors(int* count);
void               glfwGetMonitorPos(GLFWmonitor* monitor, int* xpos, int* ypos);
const GLFWvidmode* glfwGetVideoMode(GLFWmonitor* monitor);
GLFWmonitor*       glfwGetWindowMonitor(GLFWwindow* window);

GLFWwindow* glfwGetCurrentContext(void);
void glfwMakeContextCurrent(GLFWwindow* window);
void glfwSwapBuffers(GLFWwindow* window);
void glfwSwapInterval(int interval);

int  glfwGetInputMode(GLFWwindow* window, int mode);
void glfwSetInputMode(GLFWwindow* window, int mode, int value);
int  glfwGetKey(GLFWwindow* window, int key);
int  glfwGetMouseButton(GLFWwindow* window, int button);
void glfwGetCursorPos(GLFWwindow* window, double* xpos, double* ypos);

GLFWglproc  glfwGetProcAddress(const char* procname);
const char* glfwGetClipboardString(void* window);
void        glfwSetClipboardString(void* window, const char* string);
GLFWcursor* glfwCreateStandardCursor(int shape);
void        glfwDestroyCursor(GLFWcursor* cursor);
void        glfwSetCursor(GLFWwindow* window, GLFWcursor* cursor);

void glfwPostEmptyEvent(void);

void glfwSetWindowIcon(GLFWwindow* window, int count, const GLFWimage* images);

typedef void (*GLFWwindowclosefun)(GLFWwindow*);
typedef void (*GLFWwindowsizefun)(GLFWwindow*, int, int);
typedef void (*GLFWframebuffersizefun)(GLFWwindow*, int, int);
typedef void (*GLFWwindowposfun)(GLFWwindow*, int, int);
typedef void (*GLFWwindowrefreshfun)(GLFWwindow*);
typedef void (*GLFWwindowfocusfun)(GLFWwindow*, int);
typedef void (*GLFWwindowiconifyfun)(GLFWwindow*, int);
typedef void (*GLFWwindowcontentsizefun)(GLFWwindow*, float, float);
typedef void (*GLFWkeyfun)(GLFWwindow*, int, int, int, int);
typedef void (*GLFWcharfun)(GLFWwindow*, unsigned int);
typedef void (*GLFWcursorposfun)(GLFWwindow*, double, double);
typedef void (*GLFWmousebuttonfun)(GLFWwindow*, int, int, int);
typedef void (*GLFWscrollfun)(GLFWwindow*, double, double);

GLFWwindowclosefun          glfwSetWindowCloseCallback(GLFWwindow*, GLFWwindowclosefun);
GLFWwindowsizefun           glfwSetWindowSizeCallback(GLFWwindow*, GLFWwindowsizefun);
GLFWframebuffersizefun      glfwSetFramebufferSizeCallback(GLFWwindow*, GLFWframebuffersizefun);
GLFWwindowposfun            glfwSetWindowPosCallback(GLFWwindow*, GLFWwindowposfun);
GLFWwindowrefreshfun        glfwSetWindowRefreshCallback(GLFWwindow*, GLFWwindowrefreshfun);
GLFWwindowfocusfun          glfwSetWindowFocusCallback(GLFWwindow*, GLFWwindowfocusfun);
GLFWwindowiconifyfun        glfwSetWindowIconifyCallback(GLFWwindow*, GLFWwindowiconifyfun);
GLFWwindowcontentsizefun    glfwSetWindowContentScaleCallback(GLFWwindow*, GLFWwindowcontentsizefun);
GLFWkeyfun                  glfwSetKeyCallback(GLFWwindow*, GLFWkeyfun);
GLFWcharfun                 glfwSetCharCallback(GLFWwindow*, GLFWcharfun);
GLFWcursorposfun            glfwSetCursorPosCallback(GLFWwindow*, GLFWcursorposfun);
GLFWmousebuttonfun          glfwSetMouseButtonCallback(GLFWwindow*, GLFWmousebuttonfun);
GLFWscrollfun               glfwSetScrollCallback(GLFWwindow*, GLFWscrollfun);

double glfwGetTime(void);
void glfwPollEvents(void);
void glfwWaitEvents(void);
void glfwWaitEventsTimeout(double timeout);
const char* glfwGetError(int* description);

/* Vulkan integration */
void        glfwInitVulkanLoader(PFN_vkGetInstanceProcAddr loader);
const char** glfwGetRequiredInstanceExtensions(uint32_t* count);
VkResult    glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window,
                                     const VkAllocationCallbacks* allocator,
                                     VkSurfaceKHR* surface);

#ifdef __cplusplus
}
#endif

#endif
