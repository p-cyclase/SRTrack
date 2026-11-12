#include <windows.h>

#include <cstdint>
#include <cstring>
#include <cwchar>

#include "../srtrack-exporter/third_party/aviutl2-sdk/plugin2.h"

namespace {

using AddHandlerScriptFile = bool(__cdecl*)(LPCWSTR filepath);

HINSTANCE module_instance = nullptr;
bool registered = false;

bool get_script_path(wchar_t* path, DWORD capacity) {
  DWORD length = GetModuleFileNameW(module_instance, path, capacity);
  if (length == 0 || length >= capacity) return false;
  wchar_t* slash = wcsrchr(path, L'\\');
  if (!slash) return false;
  constexpr wchar_t script_name[] = L"SRTrackImporter.lua";
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

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) module_instance = instance;
  return TRUE;
}

extern "C" __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable() {
  static COMMON_PLUGIN_TABLE table{
      L"SRTrack Importer（試作版）",
      L"GCMZDrops2 を通じて同名 SRT を SRTrack Object として追加します"};
  return &table;
}

extern "C" __declspec(dllexport) bool InitializePlugin(DWORD) {
  register_handler_script();
  return true;
}

extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE*) {
  // Try once more at the later registration stage in case plugin startup
  // ordering delayed GCMZDrops2's module initialization.
  register_handler_script();
}
