#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

int main()
{
    DISPLAY_DEVICEW display = {};
    display.cb = sizeof(display);

    WCHAR primaryName[32] = {};

    // Find the Windows primary display.
    bool foundDisplay = false;

    for (DWORD i = 0; EnumDisplayDevicesW(NULL, i, &display, 0); ++i)
    {
        if ((display.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) &&
            (display.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
        {
            wcscpy_s(primaryName, display.DeviceName);
            foundDisplay = true;
            break;
        }

        ZeroMemory(&display, sizeof(display));
        display.cb = sizeof(display);
    }

    if (!foundDisplay)
    {
        printf("ERROR: Could not find the primary display.\n");
        return 1;
    }

    printf("Primary display: %ls\n", primaryName);
    printf("Searching for existing 3840x2160 @ 100 Hz mode...\n");

    DEVMODEW mode = {};
    mode.dmSize = sizeof(mode);

    bool foundMode = false;

    // Enumerate modes already supplied by Windows/display driver.
    for (DWORD i = 0;
         EnumDisplaySettingsExW(primaryName, i, &mode, 0);
         ++i)
    {
        if (mode.dmPelsWidth == 3840 &&
            mode.dmPelsHeight == 2160 &&
            mode.dmDisplayFrequency == 100)
        {
            foundMode = true;
            break;
        }

        ZeroMemory(&mode, sizeof(mode));
        mode.dmSize = sizeof(mode);
    }

    if (!foundMode)
    {
        printf("ERROR: Existing 3840x2160 @ 100 Hz mode was not found.\n");
        printf("No display configuration was changed.\n");
        return 2;
    }

    printf("Found existing mode:\n");
    printf("  Resolution : %lu x %lu\n",
           mode.dmPelsWidth,
           mode.dmPelsHeight);
    printf("  Refresh    : %lu Hz\n",
           mode.dmDisplayFrequency);
    printf("  BPP        : %lu\n",
           mode.dmBitsPerPel);

    // Apply the existing mode.
    LONG result = ChangeDisplaySettingsExW(
        primaryName,
        &mode,
        NULL,
        CDS_UPDATEREGISTRY,
        NULL
    );

    if (result != DISP_CHANGE_SUCCESSFUL)
    {
        printf("ERROR: Failed to apply display mode.\n");
        printf("Windows error code: %ld\n", result);
        return 3;
    }

    printf("\nSUCCESS\n");
    printf("3840x2160 @ 100 Hz is now active.\n");
    printf("No custom display mode was created.\n");

    return 0;
}
Compile exactly as you were compiling before:
