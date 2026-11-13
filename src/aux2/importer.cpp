#include <windows.h>

#include <cstdint>
#include <cstring>
#include <cwchar>

#include "../../third_party/aviutl2-sdk/plugin2.h"
#include "../../third_party/aviutl2-sdk/module2.h"
#include "importer.h"

namespace {

using AddHandlerScriptFile = bool(__cdecl*)(LPCWSTR filepath);

HINSTANCE module_instance = nullptr;
bool registered = false;
bool settings_module_registered = false;
bool auto_import_enabled = true;
SingleSrtDropMode single_srt_drop_mode = SingleSrtDropMode::PassThrough;
constexpr wchar_t registry_key[] = L"Software\\SRTrack";

bool read_registry_bool(HKEY key, const wchar_t* name, bool fallback) {
  wchar_t value[16]{};
  DWORD type = 0;
  DWORD size = sizeof(value);
  if (RegQueryValueExW(key, name, nullptr, &type,
                       reinterpret_cast<BYTE*>(value), &size) != ERROR_SUCCESS ||
      type != REG_SZ) {
    return fallback;
  }
  if (wcscmp(value, L"1") == 0 || wcscmp(value, L"true") == 0) return true;
  if (wcscmp(value, L"0") == 0 || wcscmp(value, L"false") == 0) return false;
  return fallback;
}

SingleSrtDropMode read_registry_mode(HKEY key, const wchar_t* name,
                                     SingleSrtDropMode fallback) {
  wchar_t value[16]{};
  DWORD type = 0;
  DWORD size = sizeof(value);
  if (RegQueryValueExW(key, name, nullptr, &type,
                       reinterpret_cast<BYTE*>(value), &size) != ERROR_SUCCESS ||
      type != REG_SZ) {
    return fallback;
  }
  if (wcscmp(value, L"linked") == 0) return SingleSrtDropMode::LinkedObject;
  if (wcscmp(value, L"standalone") == 0) return SingleSrtDropMode::StandaloneObject;
  if (wcscmp(value, L"pass_through") == 0) return SingleSrtDropMode::PassThrough;
  return fallback;
}

const wchar_t* registry_mode(SingleSrtDropMode mode) {
  switch (mode) {
    case SingleSrtDropMode::LinkedObject: return L"linked";
    case SingleSrtDropMode::StandaloneObject: return L"standalone";
    default: return L"pass_through";
  }
}

void get_auto_import_enabled(SCRIPT_MODULE_PARAM* param) {
  param->push_result_boolean(auto_import_enabled);
}

void get_single_srt_drop_mode(SCRIPT_MODULE_PARAM* param) {
  param->push_result_int(static_cast<int>(single_srt_drop_mode));
}

SCRIPT_MODULE_FUNCTION settings_functions[] = {
    {L"auto_import_enabled", get_auto_import_enabled},
    {L"single_srt_drop_mode", get_single_srt_drop_mode},
    {nullptr, nullptr},
};
SCRIPT_MODULE_TABLE settings_module = {
    L"SRTrack Importer Settings 0.1 by p_cyclase", settings_functions};

bool get_script_path(wchar_t* path, DWORD capacity) {
  DWORD length = GetModuleFileNameW(module_instance, path, capacity);
  if (length == 0 || length >= capacity) return false;
  wchar_t* slash = wcsrchr(path, L'\\');
  if (!slash) return false;
  constexpr wchar_t script_name[] = L"SRTrack.lua";
  const size_t directory_length = static_cast<size_t>(slash - path + 1);
  if (directory_length + _countof(script_name) > capacity) return false;
  memcpy(path + directory_length, script_name, sizeof(script_name));
  return true;
}

void register_handler_script() {
  if (registered) return;

  // GCMZDrops2 is already loaded by AviUtl2 as a plugin.  Do not load or
  // inspect its files; use only its documented exported registration entry.
  HMODULE gcmz = GetModuleHandleW(L"GCMZDrops.aux2");
  if (!gcmz) return;
  const auto add_handler = reinterpret_cast<AddHandlerScriptFile>(
      GetProcAddress(gcmz, "AddHandlerScriptFile"));
  if (!add_handler) return;

  wchar_t script_path[32768];
  if (!get_script_path(script_path, _countof(script_path))) return;
  registered = add_handler(script_path);
}

}  // namespace

void initialize_importer(HINSTANCE instance) {
  module_instance = instance;
}

void load_importer_settings() {
  auto_import_enabled = true;
  single_srt_drop_mode = SingleSrtDropMode::PassThrough;
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, registry_key, 0, KEY_QUERY_VALUE,
                    &key) != ERROR_SUCCESS) {
    return;
  }
  auto_import_enabled = read_registry_bool(key, L"importer.auto_import", true);
  single_srt_drop_mode =
      read_registry_mode(key, L"importer.single_srt_drop",
                         SingleSrtDropMode::PassThrough);
  RegCloseKey(key);
}

ImporterSettings get_importer_settings() {
  return {auto_import_enabled, single_srt_drop_mode};
}

bool set_importer_settings(const ImporterSettings& settings) {
  HKEY key = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, registry_key, 0, nullptr, 0,
                      KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
    return false;
  }
  const wchar_t* enabled = settings.auto_import_enabled ? L"1" : L"0";
  const wchar_t* mode = registry_mode(settings.single_srt_drop_mode);
  const bool ok =
      RegSetValueExW(key, L"importer.auto_import", 0, REG_SZ,
                     reinterpret_cast<const BYTE*>(enabled),
                     DWORD((wcslen(enabled) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS &&
      RegSetValueExW(key, L"importer.single_srt_drop", 0, REG_SZ,
                     reinterpret_cast<const BYTE*>(mode),
                     DWORD((wcslen(mode) + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
  RegCloseKey(key);
  if (!ok) return false;
  auto_import_enabled = settings.auto_import_enabled;
  single_srt_drop_mode = settings.single_srt_drop_mode;
  return true;
}

void register_importer(HOST_APP_TABLE* host) {
  if (host && !settings_module_registered) {
    // GCMZDrops2 exposes AviUtl2-compatible script modules to handler Lua.
    // Registering this small read-only bridge keeps preferences in the normal
    // per-user registry while allowing the next drop to see their current
    // values once a future settings dialog updates them.
    host->register_script_module_name(&settings_module,
                                      L"SRTrackImporterConfig");
    settings_module_registered = true;
  }
  // This stage follows normal plugin loading, so GCMZDrops2 can expose its
  // documented registration entry before SRTrack tries to use it.
  register_handler_script();
}
