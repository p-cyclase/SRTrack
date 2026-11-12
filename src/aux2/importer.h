#pragma once

#include <windows.h>

// Importer is a logical component inside SRTrack.aux2. It registers the
// adjacent SRTrack.lua with GCMZDrops2 when that prerequisite is available.
void initialize_importer(HINSTANCE instance);
void register_importer();
