#include <windows.h>

#include <stdio.h> #include <stdlib.h> #include <string.h>

#include "nvapi.h"

// ============================================================ // Error reporting // ============================================================

static void PrintError( const char* where, NvAPIStatus status) { NvAPIShortString message;

memset(message, 0, sizeof(message));

if (NvAPIGetErrorMessage(status, message) == NVAPIOK) { printf( "ERROR: %s: %s (0x%X)\n", where, message, status); } else { printf( "ERROR: %s: NVAPI status 0x%X\n", where, status); } }

// ============================================================ // Enable NVIDIA performance counters // // This corresponds to: // // NVIDIA Control Panel // -> Desktop // -> Enable Developer Settings // -> Allow every user to use performance counters // // We deliberately obtain the setting ID by name instead of // depending on NvApiDriverSettings.h. // ============================================================

static NvAPI_Status EnablePerformanceCounters() { NvDRSSessionHandle session = NULL; NvDRSProfileHandle profile = NULL;

NvAPI_Status status;

// Create DRS session. status = NvAPIDRSCreateSession(&session);

if (status != NVAPI_OK) return status;

// Load the current NVIDIA driver settings. status = NvAPIDRSLoadSettings(session);

if (status != NVAPIOK) { NvAPIDRS_DestroySession(session); return status; }

// Get the global NVIDIA profile. status = NvAPIDRSGetCurrentGlobalProfile( session, &profile);

if (status != NVAPIOK) { NvAPIDRS_DestroySession(session); return status; }

// Ask NVIDIA for the ID of: // // "Export Performance Counters" // NvU32 settingId = 0;

status = NvAPIDRSGetSettingIdFromName( (NvAPI_UnicodeString)L"Export Performance Counters", &settingId);

if (status != NVAPIOK) { NvAPIDRS_DestroySession(session); return status; }

// Prepare the setting. NVDRS_SETTING setting;

memset(&setting, 0, sizeof(setting));

setting.version = NVDRSSETTINGVER; setting.settingId = settingId; setting.settingType = NVDRSDWORDTYPE;

// NVIDIA defines: // 0 = OFF // 1 = ON setting.u32CurrentValue = 1;

// Apply it to the global profile. status = NvAPIDRSSetSetting( session, profile, &setting);

if (status != NVAPIOK) { NvAPIDRS_DestroySession(session); return status; }

// Persist the setting. status = NvAPIDRSSaveSettings(session);

NvAPIDRSDestroySession(session);

return status; }

// ============================================================ // Main // ============================================================

int main() { printf("NVIDIA Display Helper\n"); printf("=====================\n\n");

// -------------------------------------------------------- // Initialize NVAPI // --------------------------------------------------------

NvAPIStatus status = NvAPIInitialize();

if (status != NVAPIOK) { PrintError( "NvAPIInitialize", status);

return 1; }

// -------------------------------------------------------- // Enable performance counters // --------------------------------------------------------

printf( "Enabling NVIDIA performance counters...\n");

status = EnablePerformanceCounters();

if (status != NVAPI_OK) { PrintError( "EnablePerformanceCounters", status);

NvAPI_Unload();

return 2; }

printf( "[OK] Performance counters enabled.\n");

// -------------------------------------------------------- // Done // --------------------------------------------------------

NvAPI_Unload();

printf("\nDone.\n");

return 0; } :::
