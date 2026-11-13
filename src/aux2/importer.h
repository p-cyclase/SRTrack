#pragma once

#include <windows.h>

// Registers the adjacent SRTrack.lua handler through GCMZDrops2 when that
// prerequisite is available. All import decisions remain in the handler.
void initialize_importer(HINSTANCE instance);
void register_importer();
