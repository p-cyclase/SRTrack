#include <windows.h>

#include <cstdint>
#include <cstring>
#include <cwchar>

#include "../../third_party/aviutl2-sdk/plugin2.h"
#include "importer.h"

namespace {

using AddHandlerScriptFile = bool(__cdecl*)(LPCWSTR filepath);

HINSTANCE module_instance = nullptr;
bool registered = false;

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

void register_importer() {
  // This stage follows normal plugin loading, so GCMZDrops2 can expose its
  // documented registration entry before SRTrack tries to use it.
  register_handler_script();
}
