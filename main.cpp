:::writing{variant="standard" id="58321" title="Replacement main.cpp"} #include <windows.h> #include <stdio.h> #include <stdlib.h> #include <string.h>

#include "nvapi.h" #include "NvApiDriverSettings.h"

// ============================================================ // Configuration // ============================================================

static const NvU32 TARGETWIDTH = 3840; static const NvU32 TARGETHEIGHT = 2160;

// NVAPI stores refresh rate in 1/1000 Hz. // 100000 = 100 Hz. static const NvU32 TARGETREFRESH1K = 100000;

// ============================================================ // Error handling // ============================================================

static void PrintError( const char* where, NvAPIStatus status) { NvAPIShortString message; memset(message, 0, sizeof(message));

if (NvAPIGetErrorMessage(status, message) == NVAPIOK) { printf( "ERROR: %s: %s (0x%X)\n", where, message, status); } else { printf( "ERROR: %s: NVAPI status 0x%X\n", where, status); } }

// ============================================================ // Display configuration cleanup // ============================================================

static void Cleanup( NVDISPLAYCONFIGPATH_INFO* paths, NvU32 pathCount) { if (!paths) return;

for (NvU32 i = 0; i < pathCount; ++i) { if (paths[i].targetInfo) { for (NvU32 j = 0; j < paths[i].targetInfoCount; ++j) { free(paths[i].targetInfo[j].details); }

free(paths[i].targetInfo); }

free(paths[i].sourceModeInfo); }

free(paths); }

// ============================================================ // Get NVIDIA display configuration // ============================================================

static NvAPIStatus GetDisplayConfig( NvU32 pathCount, NVDISPLAYCONFIGPATHINFO* outPaths) { NvAPI_Status status; NvU32 count = 0;

NVDISPLAYCONFIGPATH_INFO* paths = NULL;

status = NvAPIDISPGetDisplayConfig(&count, NULL);

if (status != NVAPI_OK) return status;

if (count == 0) return NVAPIDATANOT_FOUND;

paths = (NVDISPLAYCONFIGPATHINFO) calloc(count, sizeof(NVDISPLAYCONFIGPATH_INFO));

if (!paths) return NVAPIOUTOF_MEMORY;

for (NvU32 i = 0; i < count; ++i) { paths[i].version = NVDISPLAYCONFIGPATHINFOVER; }

status = NvAPIDISPGetDisplayConfig(&count, paths);

if (status != NVAPI_OK) { Cleanup(paths, count); return status; }

for (NvU32 i = 0; i < count; ++i) { #ifdef NVDISPLAYCONFIGPATHINFOVER3

if (paths[i].version == NVDISPLAYCONFIGPATHINFOVER3) { if (paths[i].sourceModeInfoCount == 0) { Cleanup(paths, count); return NVAPIDATANOT_FOUND; }

paths[i].sourceModeInfo = (NVDISPLAYCONFIGSOURCEMODEINFO*) calloc( paths[i].sourceModeInfoCount, sizeof(NVDISPLAYCONFIGSOURCEMODEINFO)); } else

#endif { paths[i].sourceModeInfo = (NVDISPLAYCONFIGSOURCEMODEINFO*) calloc( 1, sizeof(NVDISPLAYCONFIGSOURCEMODEINFO)); }

if (!paths[i].sourceModeInfo) { Cleanup(paths, count); return NVAPIOUTOF_MEMORY; }

if (paths[i].targetInfoCount > 0) { paths[i].targetInfo = (NVDISPLAYCONFIGPATHTARGETINFO*) calloc( paths[i].targetInfoCount, sizeof(NVDISPLAYCONFIGPATHTARGETINFO));

if (!paths[i].targetInfo) { Cleanup(paths, count); return NVAPIOUTOF_MEMORY; }

for (NvU32 j = 0; j < paths[i].targetInfoCount; ++j) { paths[i].targetInfo[j].details = (NVDISPLAYCONFIGPATHADVANCEDTARGETINFO) calloc( 1, sizeof(NVDISPLAYCONFIGPATHADVANCEDTARGET_INFO));

if (!paths[i].targetInfo[j].details) { Cleanup(paths, count); return NVAPIOUTOF_MEMORY; }

paths[i].targetInfo[j].details->version = NVDISPLAYCONFIGPATHADVANCEDTARGETINFOVER; } } }

status = NvAPIDISPGetDisplayConfig(&count, paths);

if (status != NVAPI_OK) { Cleanup(paths, count); return status; }

pathCount = count; outPaths = paths;

return NVAPI_OK; }

// ============================================================ // Enable: // "Allow every user to use performance counters" // ============================================================

static NvAPI_Status EnablePerformanceCounters() { NvDRSSessionHandle session = NULL; NvDRSProfileHandle profile = NULL;

NvAPI_Status status;

status = NvAPIDRSCreateSession(&session);

if (status != NVAPI_OK) return status;

status = NvAPIDRSLoadSettings(session);

if (status != NVAPIOK) { NvAPIDRS_DestroySession(session); return status; }

status = NvAPIDRSGetCurrentGlobalProfile( session, &profile);

if (status != NVAPIOK) { NvAPIDRS_DestroySession(session); return status; }

NVDRS_SETTING setting;

memset(&setting, 0, sizeof(setting));

setting.version = NVDRSSETTINGVER; setting.settingId = EXPORTPERFCOUNTERSID; setting.settingType = NVDRSDWORDTYPE; setting.u32CurrentValue = EXPORTPERFCOUNTERSON;

status = NvAPIDRSSetSetting( session, profile, &setting);

if (status == NVAPIOK) { status = NvAPIDRS_SaveSettings(session); }

NvAPIDRSDestroySession(session);

return status; }

// ============================================================ // Configure display // ============================================================

static NvAPIStatus ConfigureDisplay() { NvU32 pathCount = 0; NVDISPLAYCONFIGPATHINFO* paths = NULL;

NvAPI_Status status = GetDisplayConfig(&pathCount, &paths);

if (status != NVAPI_OK) return status;

printf( "Found %u display path(s).\n", pathCount);

bool changed = false;

for (NvU32 i = 0; i < pathCount; ++i) { if (!paths[i].sourceModeInfo) continue;

if (!paths[i].sourceModeInfo[0].bGDIPrimary) continue;

if (paths[i].targetInfoCount == 0) continue;

if (!paths[i].targetInfo[0].details) continue;

printf("Primary display path found.\n");

// ---------------------------------------------------- // Resolution // ----------------------------------------------------

paths[i].sourceModeInfo[0].resolution.width = TARGET_WIDTH;

paths[i].sourceModeInfo[0].resolution.height = TARGET_HEIGHT;

// ---------------------------------------------------- // Scaling // // NVSCALINGGPUSCANOUTTO_NATIVE = // centered / no scaling. // ----------------------------------------------------

paths[i].targetInfo[0].details->scaling = NVSCALINGGPUSCANOUTTO_NATIVE;

// ---------------------------------------------------- // Refresh rate // // 100000 = 100 Hz. // ----------------------------------------------------

paths[i].targetInfo[0].details->refreshRate1K = TARGETREFRESH1K;

// ---------------------------------------------------- // Deliberately NOT changing: // // - color depth // - rotation // - interlacing // // These are unrelated to your requested settings. // ----------------------------------------------------

printf("\nRequested display configuration:\n"); printf( " Resolution : %ux%u\n", TARGETWIDTH, TARGETHEIGHT);

printf( " Refresh : %u Hz\n", TARGETREFRESH1K / 1000);

printf( " Scaling : No scaling / centered\n");

changed = true; break; }

if (!changed) { Cleanup(paths, pathCount); return NVAPIDATANOT_FOUND; }

// -------------------------------------------------------- // Apply and persist configuration. // --------------------------------------------------------

status = NvAPIDISPSetDisplayConfig( pathCount, paths, NVDISPLAYCONFIGSAVETOPERSISTENCE);

Cleanup(paths, pathCount);

return status; }

// ============================================================ // Main // ============================================================

int main() { printf("NVIDIA Display Helper\n"); printf("=====================\n\n");

// -------------------------------------------------------- // Initialize NVAPI // --------------------------------------------------------

NvAPIStatus status = NvAPIInitialize();

if (status != NVAPIOK) { PrintError( "NvAPIInitialize", status);

return 1; }

// -------------------------------------------------------- // 1. Performance counters // --------------------------------------------------------

printf( "[1/2] Enabling performance counters...\n");

status = EnablePerformanceCounters();

if (status != NVAPI_OK) { PrintError( "EnablePerformanceCounters", status);

printf( "WARNING: Continuing with display configuration.\n\n"); } else { printf( "[OK] Performance counters enabled.\n\n"); }

// -------------------------------------------------------- // 2. Display configuration // --------------------------------------------------------

printf( "[2/2] Configuring primary display...\n");

status = ConfigureDisplay();

if (status != NVAPI_OK) { PrintError( "ConfigureDisplay", status);

NvAPI_Unload();

return 2; }

printf( "\n[OK] Display configuration applied successfully.\n");

// -------------------------------------------------------- // Done // --------------------------------------------------------

printf("\nSetup complete.\n");

NvAPI_Unload();

return 0; } :::
