/**
 * I stole this from somewhere. Can't find it again
 */
#include "platform/platform.h"
#include "renderer/vulkan/vulkanTypes.h"

// Linux platform layer.
#if FSN_PLATFORM_LINUX

#include "core/event.h"
#include "core/input.h"
#include "core/logger.h"
#include "helpers/dinoarray.h"
#include "renderer/vulkan/vulkanPlatform.h"

#include <X11/XKBlib.h>   // sudo apt-get install libx11-dev
#include <X11/Xlib-xcb.h> // sudo apt-get install libxkbcommon-x11-dev
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <xcb/xcb.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h> // nanosleep
#else
#include <unistd.h> // usleep
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Surface Creation
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

typedef struct platformState {
    Display* display;
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_screen_t* screen;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_delete_win;
    VkSurfaceKHR surface;
} platformState;

static platformState* systemPtr;

keys translateKeycodeWinToXCB(u32 x_keycode);

b8 platformStartup(u64* memoryRequirement, void* state, const char* appName,
                   i32 x, i32 y, i32 width, i32 height) {
    *memoryRequirement = sizeof(platformState);
    if (state == 0) {
        return true;
    }
    systemPtr = state;

    // Connect to X
    systemPtr->display = XOpenDisplay(NULL);

    // TODO: Maybe turn this on.
    // Turn off key repeats.
    // XAutoRepeatOff(systemPtr->display);

    // Retrieve the connection from the display.
    systemPtr->connection = XGetXCBConnection(systemPtr->display);

    if (xcb_connection_has_error(systemPtr->connection)) {
        FFATAL("Failed to connect to X server via XCB.");
        return false;
    }

    // Get data from the X server
    const struct xcb_setup_t* setup = xcb_get_setup(systemPtr->connection);

    // Loop through screens using iterator
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(setup);
    int screen_p = 0;
    for (i32 s = screen_p; s > 0; s--) {
        xcb_screen_next(&it);
    }

    // After screens have been looped through, assign it.
    systemPtr->screen = it.data;

    // Allocate a XID for the window to be created.
    systemPtr->window = xcb_generate_id(systemPtr->connection);

    // Register event types.
    // XCB_CW_BACK_PIXEL = filling then window bg with a single color
    // XCB_CW_EVENT_MASK is required.
    u32 eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

    // Listen for keyboard and mouse buttons
    u32 eventVals = XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
                    XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_POINTER_MOTION |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    // Values to be sent over XCB (bg color, events)
    u32 valList[] = {systemPtr->screen->black_pixel, eventVals};

    // Create the window
    xcb_void_cookie_t cookie =
        xcb_create_window(systemPtr->connection,
                          XCB_COPY_FROM_PARENT, // depth
                          systemPtr->window,
                          systemPtr->screen->root,       // parent
                          x,                             // x
                          y,                             // y
                          width,                         // width
                          height,                        // height
                          0,                             // No border
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
                          systemPtr->screen->root_visual, eventMask, valList);

    // Change the title
    xcb_change_property(systemPtr->connection, XCB_PROP_MODE_REPLACE,
                        systemPtr->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                        8, // data should be viewed 8 bits at a time
                        strlen(appName), appName);

    // Tell the server to notify when the window manager
    // attempts to destroy the window.
    xcb_intern_atom_cookie_t wm_delete_cookie =
        xcb_intern_atom(systemPtr->connection, 0, strlen("WM_DELETE_WINDOW"),
                        "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(
        systemPtr->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* wm_delete_reply =
        xcb_intern_atom_reply(systemPtr->connection, wm_delete_cookie, NULL);
    xcb_intern_atom_reply_t* wm_protocols_reply =
        xcb_intern_atom_reply(systemPtr->connection, wm_protocols_cookie, NULL);
    systemPtr->wm_delete_win = wm_delete_reply->atom;
    systemPtr->wm_protocols = wm_protocols_reply->atom;

    xcb_change_property(systemPtr->connection, XCB_PROP_MODE_REPLACE,
                        systemPtr->window, wm_protocols_reply->atom, 4, 32, 1,
                        &wm_delete_reply->atom);

    // Map the window to the screen
    xcb_map_window(systemPtr->connection, systemPtr->window);

    // Flush the stream
    i32 stream_result = xcb_flush(systemPtr->connection);
    if (stream_result <= 0) {
        FFATAL("An error occurred when flusing the stream: %d", stream_result);
        return false;
    }
    FINFO("Platform inited");

    return true;
}

void platformShutdown() {
    // Turn key repeats back on since this is global for the OS... just... wow.
    // XAutoRepeatOn(systemPtr->display);

    xcb_destroy_window(systemPtr->connection, systemPtr->window);
}

b8 platformPumpMessages() {
    xcb_generic_event_t* event;
    xcb_client_message_event_t* cm;

    b8 quitFlagged = false;

    // Poll for events until null is returned.
    while (event != 0) {
        event = xcb_poll_for_event(systemPtr->connection);
        if (event == 0) {
            break;
        }

        // Input events
        switch (event->response_type & ~0x80) {
            case XCB_KEY_PRESS:
            case XCB_KEY_RELEASE: {
                // Key press event - xcb_key_press_event_t and
                // xcb_key_release_event_t are the same
                xcb_key_press_event_t* kb_event = (xcb_key_press_event_t*)event;
                b8 pressed = event->response_type == XCB_KEY_PRESS;
                xcb_keycode_t code = kb_event->detail;
                KeySym key_sym =
                    XkbKeycodeToKeysym(systemPtr->display,
                                       (KeyCode)code, // event.xkey.keycode,
                                       0, code & ShiftMask ? 1 : 0);

                keys key = translateKeycodeWinToXCB(key_sym);

                // Pass to the input subsystem for processing.
                inputProcessKey(key, pressed);
            } break;
            case XCB_BUTTON_PRESS:
            case XCB_BUTTON_RELEASE: {
                xcb_button_press_event_t* mouse_event =
                    (xcb_button_press_event_t*)event;
                b8 pressed = event->response_type == XCB_BUTTON_PRESS;
                buttons mouse_button = BUTTON_MAX_BUTTONS;
                switch (mouse_event->detail) {
                    case XCB_BUTTON_INDEX_1:
                        mouse_button = BUTTON_LEFT;
                        break;
                    case XCB_BUTTON_INDEX_2:
                        mouse_button = BUTTON_MIDDLE;
                        break;
                    case XCB_BUTTON_INDEX_3:
                        mouse_button = BUTTON_RIGHT;
                        break;
                }

                // Pass over to the input subsystem.
                if (mouse_button != BUTTON_MAX_BUTTONS) {
                    inputProcessButton(mouse_button, pressed);
                }
            }
            case XCB_MOTION_NOTIFY: {
                // Mouse move
                xcb_motion_notify_event_t* move_event =
                    (xcb_motion_notify_event_t*)event;

                // Pass over to the input subsystem.
                inputProcessMouseMove(move_event->event_x, move_event->event_y);
                break;
            }

            case XCB_CONFIGURE_NOTIFY: {
                // Resizing - note that this is also triggered by moving the
                // window, but should be
                //  passed anyway since a change in the x/y could mean an
                //  upper-left resize.
                // The app layer can decide what to do with it.
                xcb_configure_notify_event_t* configure_event =
                    (xcb_configure_notify_event_t*)event;
                // Fire an event. the app layer should pick this up, but not
                // handle it
                //  as it shouldnt be visible to other parts of the app
                eventContext ec;
                ec.data.u16[0] = configure_event->width;
                ec.data.u16[1] = configure_event->height;
                eventFire(EVENT_CODE_RESIZED, 0, ec);
            }

            case XCB_CLIENT_MESSAGE: {
                cm = (xcb_client_message_event_t*)event;
                FDEBUG("Window Message %d", cm->data.data32[0]);
                FDEBUG("Delete: %d", systemPtr->wm_delete_win);

                // Window close
                if (cm->data.data32[0] == systemPtr->wm_delete_win) {
                    FDEBUG("Window Closed");
                    quitFlagged = true;
                    eventContext ec;
                    ec.data.u8[0] = true;
                    eventFire(EVENT_CODE_APPLICATION_QUIT, 0, ec);
                }
            } break;
            default:
                // Something else
                break;
        }

        free(event);
    }
    return !quitFlagged;
}

void* platformAllocate(u64 size, b8 aligned) {
    return malloc(size);
}
void platformFree(void* block, b8 aligned) {
    free(block);
}
void* platformZeroMemory(void* block, u64 size) {
    return memset(block, 0, size);
}
void* platformCopyMemory(void* dest, const void* source, u64 size) {
    return memcpy(dest, source, size);
}
void* platformSetMemory(void* dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

void platformConsoleWrite(const char* message, u8 color) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char* colorStrs[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colorStrs[color], message);
}
void platformConsoleWriteError(const char* message, u8 color) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char* colorStrs[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colorStrs[color], message);
}

f64 platformGetAbsoluteTime() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 0.000000001;
}

void platformSleep(u64 ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

void platformGetRequiredExts(const char*** array) {
    dinoPush(*array, &"VK_KHR_xcb_surface");
}

// Surface creation for Vulkan
b8 platformCreateVulkanSurface(VulkanInfo* header) {
    VkXcbSurfaceCreateInfoKHR createInfo = {
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    createInfo.connection = systemPtr->connection;
    createInfo.window = systemPtr->window;

    VkResult result = vkCreateXcbSurfaceKHR(
        header->instance, &createInfo, header->allocator, &systemPtr->surface);
    if (result != VK_SUCCESS) {
        FFATAL("Vulkan surface creation failed.");
        return false;
    }

    header->surface = systemPtr->surface;
    return true;
}

// Key translation
keys translateKeycodeWinToXCB(u32 x_keycode) {
    switch (x_keycode) {
        case XK_BackSpace:
            return KEY_BACKSPACE;
        case XK_Return:
            return KEY_ENTER;
        case XK_Tab:
            return KEY_TAB;
            // case XK_Shift: return KEY_SHIFT;
            // case XK_Control: return KEY_CONTROL;

        case XK_Pause:
            return KEY_PAUSE;
        case XK_Caps_Lock:
            return KEY_CAPITAL;

        case XK_Escape:
            return KEY_ESCAPE;

            // Not supported
            // case : return KEY_CONVERT;
            // case : return KEY_NONCONVERT;
            // case : return KEY_ACCEPT;

        case XK_Mode_switch:
            return KEY_MODECHANGE;

        case XK_space:
            return KEY_SPACE;
        case XK_Prior:
            return KEY_PRIOR;
        case XK_Next:
            return KEY_NEXT;
        case XK_End:
            return KEY_END;
        case XK_Home:
            return KEY_HOME;
        case XK_Left:
            return KEY_LEFT;
        case XK_Up:
            return KEY_UP;
        case XK_Right:
            return KEY_RIGHT;
        case XK_Down:
            return KEY_DOWN;
        case XK_Select:
            return KEY_SELECT;
        case XK_Print:
            return KEY_PRINT;
        case XK_Execute:
            return KEY_EXECUTE;
        // case XK_snapshot: return KEY_SNAPSHOT; // not supported
        case XK_Insert:
            return KEY_INSERT;
        case XK_Delete:
            return KEY_DELETE;
        case XK_Help:
            return KEY_HELP;

        case XK_Meta_L:
            return KEY_LWIN; // TODO: not sure this is right
        case XK_Meta_R:
            return KEY_RWIN;
            // case XK_apps: return KEY_APPS; // not supported

            // case XK_sleep: return KEY_SLEEP; //not supported

        case XK_KP_0:
            return KEY_NUMPAD0;
        case XK_KP_1:
            return KEY_NUMPAD1;
        case XK_KP_2:
            return KEY_NUMPAD2;
        case XK_KP_3:
            return KEY_NUMPAD3;
        case XK_KP_4:
            return KEY_NUMPAD4;
        case XK_KP_5:
            return KEY_NUMPAD5;
        case XK_KP_6:
            return KEY_NUMPAD6;
        case XK_KP_7:
            return KEY_NUMPAD7;
        case XK_KP_8:
            return KEY_NUMPAD8;
        case XK_KP_9:
            return KEY_NUMPAD9;
        case XK_multiply:
            return KEY_MULTIPLY;
        case XK_KP_Add:
            return KEY_ADD;
        case XK_KP_Separator:
            return KEY_SEPARATOR;
        case XK_KP_Subtract:
            return KEY_SUBTRACT;
        case XK_KP_Decimal:
            return KEY_DECIMAL;
        case XK_KP_Divide:
            return KEY_DIVIDE;
        case XK_F1:
            return KEY_F1;
        case XK_F2:
            return KEY_F2;
        case XK_F3:
            return KEY_F3;
        case XK_F4:
            return KEY_F4;
        case XK_F5:
            return KEY_F5;
        case XK_F6:
            return KEY_F6;
        case XK_F7:
            return KEY_F7;
        case XK_F8:
            return KEY_F8;
        case XK_F9:
            return KEY_F9;
        case XK_F10:
            return KEY_F10;
        case XK_F11:
            return KEY_F11;
        case XK_F12:
            return KEY_F12;
        case XK_F13:
            return KEY_F13;
        case XK_F14:
            return KEY_F14;
        case XK_F15:
            return KEY_F15;
        case XK_F16:
            return KEY_F16;
        case XK_F17:
            return KEY_F17;
        case XK_F18:
            return KEY_F18;
        case XK_F19:
            return KEY_F19;
        case XK_F20:
            return KEY_F20;
        case XK_F21:
            return KEY_F21;
        case XK_F22:
            return KEY_F22;
        case XK_F23:
            return KEY_F23;
        case XK_F24:
            return KEY_F24;

        case XK_Num_Lock:
            return KEY_NUMLOCK;
        case XK_Scroll_Lock:
            return KEY_SCROLL;

        case XK_KP_Equal:
            return KEY_NUMPAD_EQUAL;

        case XK_Shift_L:
            return KEY_LSHIFT;
        case XK_Shift_R:
            return KEY_RSHIFT;
        case XK_Control_L:
            return KEY_LCONTROL;
        case XK_Control_R:
            return KEY_RCONTROL;
        case XK_Alt_L:
            return KEY_LALT;
        case XK_Alt_R:
            return KEY_RALT;

        case XK_semicolon:
            return KEY_SEMICOLON;
        case XK_plus:
            return KEY_PLUS;
        case XK_comma:
            return KEY_COMMA;
        case XK_minus:
            return KEY_MINUS;
        case XK_period:
            return KEY_PERIOD;
        case XK_slash:
            return KEY_SLASH;
        case XK_grave:
            return KEY_GRAVE;

        case XK_a:
        case XK_A:
            return KEY_A;
        case XK_b:
        case XK_B:
            return KEY_B;
        case XK_c:
        case XK_C:
            return KEY_C;
        case XK_d:
        case XK_D:
            return KEY_D;
        case XK_e:
        case XK_E:
            return KEY_E;
        case XK_f:
        case XK_F:
            return KEY_F;
        case XK_g:
        case XK_G:
            return KEY_G;
        case XK_h:
        case XK_H:
            return KEY_H;
        case XK_i:
        case XK_I:
            return KEY_I;
        case XK_j:
        case XK_J:
            return KEY_J;
        case XK_k:
        case XK_K:
            return KEY_K;
        case XK_l:
        case XK_L:
            return KEY_L;
        case XK_m:
        case XK_M:
            return KEY_M;
        case XK_n:
        case XK_N:
            return KEY_N;
        case XK_o:
        case XK_O:
            return KEY_O;
        case XK_p:
        case XK_P:
            return KEY_P;
        case XK_q:
        case XK_Q:
            return KEY_Q;
        case XK_r:
        case XK_R:
            return KEY_R;
        case XK_s:
        case XK_S:
            return KEY_S;
        case XK_t:
        case XK_T:
            return KEY_T;
        case XK_u:
        case XK_U:
            return KEY_U;
        case XK_v:
        case XK_V:
            return KEY_V;
        case XK_w:
        case XK_W:
            return KEY_W;
        case XK_x:
        case XK_X:
            return KEY_X;
        case XK_y:
        case XK_Y:
            return KEY_Y;
        case XK_z:
        case XK_Z:
            return KEY_Z;

        default:
            return 0;
    }
}

#endif
