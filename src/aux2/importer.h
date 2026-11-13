#pragma once

#include <windows.h>

// Registers the adjacent SRTrack.lua handler through GCMZDrops2 when that
// prerequisite is available, and the native .srt drop entrance.
void initialize_importer(HINSTANCE instance);
struct HOST_APP_TABLE;
void register_importer(HOST_APP_TABLE* host);
