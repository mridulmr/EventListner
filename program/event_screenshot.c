#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32

// Windows headers
#include <windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

void captureScreenshot(const char *filename)
{
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Get the screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create a device context for the screen
    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);

    // Create a bitmap to hold the screenshot
    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, screenWidth, screenHeight);
    HGDIOBJ hOldBitmap = SelectObject(memDC, hBitmap);

    // Copy the screen contents to the bitmap
    BitBlt(memDC, 0, 0, screenWidth, screenHeight, screenDC, 0, 0, SRCCOPY);

    // Save the bitmap as a PNG file
    CLSID clsid;
    GetEncoderClsid(L"image/png", &clsid);
    Gdiplus::Bitmap bitmap(hBitmap, NULL);
    bitmap.Save(filename, &clsid, NULL);

    // Cleanup
    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    // Shutdown GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    printf("Screenshot captured: %s\n", filename);
}

#elif __APPLE__
// macOS headers
#include <ApplicationServices/ApplicationServices.h>
void captureScreenshot(const char *filename)
{
    // Get the main display
    CGDirectDisplayID displayID = kCGDirectMainDisplay;
    CGImageRef screenshot = CGDisplayCreateImage(displayID);

    // Save the image as a PNG file
    CFStringRef utTypePNG = CFSTR("public.png");
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, CFStringCreateWithCString(NULL, filename, kCFStringEncodingUTF8), kCFURLPOSIXPathStyle, false);
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, utTypePNG, 1, NULL);
    CGImageDestinationAddImage(destination, screenshot, NULL);
    CGImageDestinationFinalize(destination);

    // Cleanup
    CFRelease(destination);
    CFRelease(url);
    CGImageRelease(screenshot);

    printf("Screenshot captured: %s\n", filename);
}

#else

// Linux headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>

void captureScreenshot(const char *filename)
{
    // Open the display
    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);

    // Get the screen dimensions
    XWindowAttributes attributes;
    XGetWindowAttributes(display, root, &attributes);
    int width = attributes.width;
    int height = attributes.height;

    // Capture the screen image
    XImage *image = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);

    // Open the output file
    FILE *file = fopen(filename, "wb");
    if (file != NULL)
    {
        // Create the PNG data structures
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png != NULL)
        {
            png_infop info = png_create_info_struct(png);
            if (info != NULL)
            {
                // Set up error handling
                if (setjmp(png_jmpbuf(png)) == 0)
                {
                    // Set the output file
                    png_init_io(png, file);

                    // Set the PNG image information
                    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

                    // Allocate memory for row pointers
                    png_bytep *row_pointers = (png_bytep *)malloc(height * sizeof(png_bytep));
                    for (int y = 0; y < height; ++y)
                    {
                        row_pointers[y] = (png_bytep)(image->data + y * image->bytes_per_line);
                    }

                    // Write the image data
                    png_set_rows(png, info, row_pointers);
                    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);

                    // Cleanup
                    free(row_pointers);
                }
                else
                {
                    printf("Error during PNG write.\n");
                }
            }
            else
            {
                printf("Error creating PNG info struct.\n");
            }
            png_destroy_write_struct(&png, &info);
        }
        else
        {
            printf("Error creating PNG write struct.\n");
        }
        fclose(file);
    }
    else
    {
        printf("Error opening output file.\n");
    }

    // Cleanup
    XDestroyImage(image);
    XCloseDisplay(display);

    printf("Screenshot captured: %s\n", filename);
}

#endif

int main()
{
    const char *filename = "screenshot.png";
    captureScreenshot(filename);

    // Read the screenshot file and write its content to stdout
    FILE *file = fopen(filename, "rb");
    if (file != NULL)
    {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        unsigned char *file_data = (unsigned char *)malloc(file_size);
        fread(file_data, 1, file_size, file);
        fclose(file);

        // Write the file content to stdout
        fwrite(file_data, 1, file_size, stdout);
        fflush(stdout);

        free(file_data);
        // Delete the screenshot file
        remove(filename);
    }

    return 0;
}
