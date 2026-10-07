#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvapi.h"

static void Cleanup(
    NV_DISPLAYCONFIG_PATH_INFO* paths,
    NvU32 pathCount)
{
    if (!paths)
        return;

    for (NvU32 i = 0; i < pathCount; ++i)
    {
        if (paths[i].targetInfo)
        {
            for (NvU32 j = 0; j < paths[i].targetInfoCount; ++j)
            {
                free(paths[i].targetInfo[j].details);
            }

            free(paths[i].targetInfo);
        }

        free(paths[i].sourceModeInfo);
    }

    free(paths);
}


static NvAPI_Status GetDisplayConfig(
    NvU32* pathCount,
    NV_DISPLAYCONFIG_PATH_INFO** outPaths)
{
    NvAPI_Status status;
    NvU32 count = 0;

    NV_DISPLAYCONFIG_PATH_INFO* paths = NULL;

    status = NvAPI_DISP_GetDisplayConfig(&count, NULL);

    if (status != NVAPI_OK)
        return status;

    paths = (NV_DISPLAYCONFIG_PATH_INFO*)
        calloc(count, sizeof(NV_DISPLAYCONFIG_PATH_INFO));

    if (!paths)
        return NVAPI_OUT_OF_MEMORY;

    for (NvU32 i = 0; i < count; ++i)
        paths[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;

    status = NvAPI_DISP_GetDisplayConfig(&count, paths);

    if (status != NVAPI_OK)
    {
        Cleanup(paths, count);
        return status;
    }

    for (NvU32 i = 0; i < count; ++i)
    {
        /*
         * NVIDIA's current API can report different path
         * structure versions. Allocate the source-mode array
         * according to what the driver reports.
         */

#ifdef NV_DISPLAYCONFIG_PATH_INFO_VER3
        if (paths[i].version == NV_DISPLAYCONFIG_PATH_INFO_VER3)
        {
            paths[i].sourceModeInfo =
                (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)
                calloc(
                    paths[i].sourceModeInfoCount,
                    sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
        }
        else
#endif
        {
            paths[i].sourceModeInfo =
                (NV_DISPLAYCONFIG_SOURCE_MODE_INFO*)
                calloc(
                    1,
                    sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
        }

        if (!paths[i].sourceModeInfo)
        {
            Cleanup(paths, count);
            return NVAPI_OUT_OF_MEMORY;
        }

        paths[i].targetInfo =
            (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)
            calloc(
                paths[i].targetInfoCount,
                sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));

        if (!paths[i].targetInfo)
        {
            Cleanup(paths, count);
            return NVAPI_OUT_OF_MEMORY;
        }

        for (NvU32 j = 0; j < paths[i].targetInfoCount; ++j)
        {
            paths[i].targetInfo[j].details =
                (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)
                calloc(
                    1,
                    sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));

            if (!paths[i].targetInfo[j].details)
            {
                Cleanup(paths, count);
                return NVAPI_OUT_OF_MEMORY;
            }

            paths[i].targetInfo[j].details->version =
                NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
        }
    }

    status = NvAPI_DISP_GetDisplayConfig(&count, paths);

    if (status != NVAPI_OK)
    {
        Cleanup(paths, count);
        return status;
    }

    *pathCount = count;
    *outPaths = paths;

    return NVAPI_OK;
}


static void PrintError(
    const char* where,
    NvAPI_Status status)
{
    NvAPI_ShortString message;

    memset(message, 0, sizeof(message));

    if (NvAPI_GetErrorMessage(status, message) == NVAPI_OK)
    {
        printf(
            "ERROR: %s: %s (0x%X)\n",
            where,
            message,
            status);
    }
    else
    {
        printf(
            "ERROR: %s: NVAPI status 0x%X\n",
            where,
            status);
    }
}


int main()
{
    NvAPI_Status status;

    printf("NVIDIA Display Setup\n");
    printf("====================\n\n");

    status = NvAPI_Initialize();

    if (status != NVAPI_OK)
    {
        PrintError("NvAPI_Initialize", status);
        return 1;
    }

    NvU32 pathCount = 0;
    NV_DISPLAYCONFIG_PATH_INFO* paths = NULL;

    status = GetDisplayConfig(&pathCount, &paths);

    if (status != NVAPI_OK)
    {
        PrintError(
            "NvAPI_DISP_GetDisplayConfig",
            status);

        NvAPI_Unload();
        return 2;
    }

    printf("Found %u display path(s).\n", pathCount);

    bool changed = false;

    /*
     * Find the Windows primary display.
     *
     * We deliberately modify ONLY the path marked GDI primary.
     */
    for (NvU32 i = 0; i < pathCount; ++i)
    {
        if (!paths[i].sourceModeInfo)
            continue;

        if (!paths[i].sourceModeInfo[0].bGDIPrimary)
            continue;

        if (paths[i].targetInfoCount == 0)
            continue;

        printf("Primary display path found.\n");

        /*
         * Desktop/source resolution.
         */
        paths[i].sourceModeInfo[0].resolution.width = 3840;
        paths[i].sourceModeInfo[0].resolution.height = 2160;

        /*
         * 32 = Windows desktop color depth.
         *
         * This does NOT force the HDMI output to 32-bit
         * physical transmission. The NVIDIA driver still
         * determines the actual link format.
         */
        paths[i].sourceModeInfo[0].resolution.colorDepth = 32;

        /*
         * NVIDIA scaling:
         *
         * NV_SCALING_GPU_SCANOUT_TO_NATIVE corresponds to
         * GPU scaling with the image centered at native
         * resolution / no scaling.
         */
        paths[i].targetInfo[0].details->scaling =
            NV_SCALING_GPU_SCANOUT_TO_NATIVE;

        /*
         * 100 Hz.
         *
         * NVAPI stores refresh rate in 1/1000 Hz.
         * 100 Hz = 100000.
         */
        paths[i].targetInfo[0].details->refreshRate1K =
            100000;

        /*
         * Progressive scan.
         */
        paths[i].targetInfo[0].details->interlaced = 0;

        /*
         * No rotation.
         */
        paths[i].targetInfo[0].details->rotation =
            NV_ROTATE_0;

        printf("\nRequested configuration:\n");
        printf("  Resolution : 3840x2160\n");
        printf("  Refresh    : 100 Hz\n");
        printf("  Scaling    : Centered / No Scaling\n");

        changed = true;
        break;
    }

    if (!changed)
    {
        printf(
            "ERROR: Could not find the primary NVIDIA display.\n");

        Cleanup(paths, pathCount);
        NvAPI_Unload();
        return 3;
    }

    /*
     * Apply the configuration.
     *
     * NVIDIA's official sample uses NvAPI_DISP_SetDisplayConfig()
     * to apply the prepared display-path configuration.
     */
    status = NvAPI_DISP_SetDisplayConfig(
        pathCount,
        paths,
        NV_DISPLAYCONFIG_SAVE_TO_PERSISTENCE);

    if (status != NVAPI_OK)
    {
        PrintError(
            "NvAPI_DISP_SetDisplayConfig",
            status);

        Cleanup(paths, pathCount);
        NvAPI_Unload();
        return 4;
    }

    printf("\nDisplay configuration applied successfully.\n");

    Cleanup(paths, pathCount);
    NvAPI_Unload();

    return 0;
}
