#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <unistd.h>

#elif defined(_WIN32)
#include <windows.h>
#include <winuser.h>

#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#endif

int pipeFd[2];

// Function to send event data to the pipe
void sendDataToPipe(const char *eventData)
{
    // Write the event data to the pipe
    write(STDOUT_FILENO, eventData, strlen(eventData));
    write(STDOUT_FILENO, "\n", 1);
}

// Function to send event data to Node.js
void sendDataToNode(const char *eventData)
{
    printf("%s\n", eventData);
    fflush(stdout);
}

#if defined(__linux__)
Display *display;
int keyCount = 0;
int leftButtonCount = 0;
int rightButtonCount = 0;

// Event callback function for Linux
int eventCallback(XEvent *event)
{
    if (event->type == KeyPress)
    {
        char eventData[256];
        sprintf(eventData, "Key Pressed (Linux): %u (Count: %d)", event->xkey.keycode, ++keyCount);
        sendDataToPipe("KEYPRESS:");
        sendDataToNode(eventData);
    }
    else if (event->type == ButtonPress)
    {
        if (event->xbutton.button == Button1)
        {
            char eventData[256];
            sprintf(eventData, "Left (Linux) (Count: %d)", ++leftButtonCount);
            sendDataToPipe("KEYPRESS:");
            sendDataToNode(eventData);
        }
        else if (event->xbutton.button == Button3)
        {
            char eventData[256];
            sprintf(eventData, "Right (Linux) (Count: %d)", ++rightButtonCount);
            sendDataToPipe("KEYPRESS:");
            sendDataToNode(eventData);
        }
    }
    // Handle other mouse and keyboard events here

    return 0;
}

#elif defined(_WIN32)
LRESULT CALLBACK eventCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN)
        {
            KBDLLHOOKSTRUCT *hookStruct = (KBDLLHOOKSTRUCT *)lParam;
            char eventData[256];
            sprintf(eventData, "Key Pressed (Windows): %d", hookStruct->vkCode);
            sendDataToPipe("KEYPRESS:");
            sendDataToNode(eventData);
        }
        else if (wParam == WM_LBUTTONDOWN)
        {
            char eventData[256];
            sprintf(eventData, "Left (Windows)");
            sendDataToPipe("KEYPRESS:");
            sendDataToNode(eventData);
        }
        else if (wParam == WM_RBUTTONDOWN)
        {
            char eventData[256];
            sprintf(eventData, "Right (Windows)");
            sendDataToPipe("KEYPRESS:");
            sendDataToNode(eventData);
        }
        // Handle other mouse and keyboard events here
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

#elif defined(__APPLE__)
CGEventRef eventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
    if (type == kCGEventKeyDown)
    {
        char eventData[256];
        sprintf(eventData, "%d", (int)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
        sendDataToPipe("KEYPRESS:");
        sendDataToNode(eventData);
    }
    else if (type == kCGEventLeftMouseDown)
    {
        char eventData[256];
        sprintf(eventData, "Left (macOS)");
        sendDataToPipe("KEYPRESS:");
        sendDataToNode(eventData);
    }
    else if (type == kCGEventRightMouseDown)
    {
        char eventData[256];
        sprintf(eventData, "Right (macOS)");
        sendDataToPipe("KEYPRESS:");
        sendDataToNode(eventData);
    }

    return event;
}
#endif

int main()
{

#if defined(__linux__)
    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);

    XEvent ev;
    XSelectInput(display, root, KeyPressMask | ButtonPressMask);

    while (1)
    {
        XNextEvent(display, &ev);
        eventCallback(&ev);
    }

    XCloseDisplay(display);

#elif defined(_WIN32)
    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, eventCallback, NULL, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(hook);

#elif defined(__APPLE__)
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;

    CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventRightMouseDown);
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, eventMask, eventCallback, NULL);

    if (!eventTap)
    {
        fprintf(stderr, "Failed to create event tap.\n");
        return -1;
    }

    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();

    CFRelease(eventTap);
    CFRelease(runLoopSource);

#endif

    return 0;
}
