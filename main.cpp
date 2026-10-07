#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

int main()
{
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);

    WCHAR deviceName[32] = {};
    bool found = false;

    for (DWORD i = 0; EnumDisplayDevicesW(NULL, i, &dd, 0); ++i)
    {
        if ((dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) &&
            (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
        {
            wcscpy_s(deviceName, dd.DeviceName);
            found = true;
            break;
        }

        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);
    }

    if (!found)
    {
        printf("ERROR: Primary display not found.\n");
        return 1;
    }

    printf("Display: %ls\n", deviceName);
    printf("Looking for existing 3840x2160 @ 100Hz...\n");

    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);

    bool modeFound = false;

    for (DWORD i = 0;
         EnumDisplaySettingsExW(deviceName, i, &dm, 0);
         ++i)
    {
        if (dm.dmPelsWidth == 3840 &&
            dm.dmPelsHeight == 2160 &&
            dm.dmDisplayFrequency == 100)
        {
            modeFound = true;
            break;
        }

        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
    }

    if (!modeFound)
    {
        printf("ERROR: 3840x2160 @ 100Hz was not found.\n");
        printf("Nothing was changed.\n");
        return 2;
    }

    printf("Found existing mode: 3840x2160 @ 100Hz\n");

    LONG result = ChangeDisplaySettingsExW(
        deviceName,
        &dm,
        NULL,
        CDS_UPDATEREGISTRY,
        NULL
    );

    if (result != DISP_CHANGE_SUCCESSFUL)
    {
        printf("ERROR: ChangeDisplaySettingsEx failed: %ld\n", result);
        return 3;
    }

    printf("SUCCESS: 3840x2160 @ 100Hz applied.\n");

    return 0;
}
