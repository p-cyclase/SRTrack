#pragma once

#include <windows.h>
#include "../../third_party/aviutl2-sdk/plugin2.h"

enum class SingleSrtDropMode {
  PassThrough = 0,
  LinkedObject = 1,
  StandaloneObject = 2,
};

struct ImporterSettings {
  bool auto_import_enabled = true;
  SingleSrtDropMode single_srt_drop_mode = SingleSrtDropMode::PassThrough;
};

// Importer is a logical component inside SRTrack.aux2. It registers the
// adjacent SRTrack.lua with GCMZDrops2 when that prerequisite is available.
// The settings UI is intentionally separate; this component owns the
// persisted importer defaults and exposes their current values to Lua.
void initialize_importer(HINSTANCE instance);
void load_importer_settings();
ImporterSettings get_importer_settings();
bool set_importer_settings(const ImporterSettings& settings);
void register_importer(HOST_APP_TABLE* host);
