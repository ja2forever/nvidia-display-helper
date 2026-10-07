#include <windows.h>
#include <stdio.h>

int main()
{
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);

    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);

    // Find the primary display.
    for (DWORD i = 0; EnumDisplayDevicesW(NULL, i, &dd, 0); ++i)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            break;

        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
    }

    if (!(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
    {
        printf("ERROR: Primary display not found.\n");
        return 1;
    }

    printf("Display: %ls\n", dd.DeviceName);

    // Find the EXISTING 3840x2160 @ 100 Hz mode.
    bool found = false;

    for (DWORD mode = 0;
         EnumDisplaySettingsExW(
             dd.DeviceName,
             mode,
             &dm,
             0);
         ++mode)
    {
        if (dm.dmPelsWidth == 3840 &&
            dm.dmPelsHeight == 2160 &&
            dm.dmDisplayFrequency == 100 &&
            dm.dmBitsPerPel == 32)
        {
            found = true;
            break;
        }

        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
    }

    if (!found)
    {
        printf("ERROR: Existing 3840x2160 @ 100 Hz mode not found.\n");
        return 2;
    }

    printf("Found existing mode: %lux%lu @ %lu Hz\n",
           dm.dmPelsWidth,
           dm.dmPelsHeight,
           dm.dmDisplayFrequency);

    // Apply the EXISTING mode.
    LONG result = ChangeDisplaySettingsExW(
        dd.DeviceName,
        &dm,
        NULL,
        CDS_UPDATEREGISTRY,
        NULL);

    if (result != DISP_CHANGE_SUCCESSFUL)
    {
        printf("ERROR: ChangeDisplaySettingsEx failed: %ld\n", result);
        return 3;
    }

    printf("SUCCESS: 3840x2160 @ 100 Hz applied.\n");
    return 0;
}
