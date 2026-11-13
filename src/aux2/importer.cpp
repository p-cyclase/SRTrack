#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#include "importer.h"
#include "../../third_party/aviutl2-sdk/plugin2.h"

namespace {

using AddHandlerScriptFile = bool(__cdecl*)(LPCWSTR filepath);

HINSTANCE module_instance = nullptr;
bool registered = false;

std::string wide_to_utf8(const wchar_t* value) {
  if (!value || !*value) return {};
  const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
                                       nullptr, 0, nullptr, nullptr);
  if (size <= 1) return {};
  std::string result(static_cast<size_t>(size), '\0');
  if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
                           result.data(), size, nullptr, nullptr)) {
    return {};
  }
  result.pop_back();
  return result;
}

bool read_file(const wchar_t* path, std::string& output) {
  HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER size{};
  bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 &&
            size.QuadPart <= 16 * 1024 * 1024;
  if (ok) {
    output.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    ok = output.empty() ||
         (ReadFile(file, output.data(), static_cast<DWORD>(output.size()),
                   &read, nullptr) && read == output.size());
  }
  CloseHandle(file);
  return ok;
}

int srt_length_frames(const wchar_t* path, const EDIT_INFO* info) {
  if (!info || info->rate <= 0 || info->scale <= 0) return 1;
  std::string contents;
  if (!read_file(path, contents)) return 1;
  int64_t largest_ms = 0;
  size_t pos = 0;
  while ((pos = contents.find("-->", pos)) != std::string::npos) {
    int hours = 0, minutes = 0, seconds = 0, milliseconds = 0;
    if (sscanf_s(contents.c_str() + pos, "--> %d:%d:%d,%d", &hours, &minutes,
                 &seconds, &milliseconds) == 4 ||
        sscanf_s(contents.c_str() + pos, "--> %d:%d:%d.%d", &hours, &minutes,
                 &seconds, &milliseconds) == 4) {
      largest_ms = (std::max)(largest_ms,
          (((static_cast<int64_t>(hours) * 60 + minutes) * 60 + seconds) * 1000 + milliseconds));
    }
    pos += 3;
  }
  const int64_t frames = (largest_ms * info->rate + 999 * info->scale) /
                         (1000 * info->scale);
  return static_cast<int>((std::max)(int64_t{1}, frames));
}

void append_standard_drawing(std::string& alias) {
  alias += "[0.1]\r\n"
           "effect.name=標準描画\r\n"
           "X=0.00\r\nY=0.00\r\nZ=0.00\r\nGroup=1\r\n"
           "中心X=0.00\r\n中心Y=0.00\r\n中心Z=0.00\r\nGroup3=1\r\n"
           "X軸回転=0.00\r\nY軸回転=0.00\r\nZ軸回転=0.00\r\nGroup2=1\r\n"
           "拡大率=100.000\r\n縦横比=0.000\r\n透明度=0.00\r\n合成モード=通常\r\n";
}

void on_srt_file_drop(EDIT_SECTION* edit, LPCWSTR file) {
  if (!edit || !edit->info || !file) return;
  std::string path = wide_to_utf8(file);
  if (path.empty() || path.find_first_of("\r\n") != std::string::npos) return;
  const int length = srt_length_frames(file, edit->info);
  std::string alias = "[0]\r\nlayer=1\r\nframe=0," +
      std::to_string(length - 1) +
      "\r\n[0.0]\r\neffect.name=SRTrackObject\r\nSRT ファイル=" + path +
      "\r\n動画全長=0.000\r\n字幕プレビュー=1\r\n";
  append_standard_drawing(alias);
  edit->create_object_from_alias(alias.c_str(), edit->info->layer,
                                 edit->info->frame, length);
}

bool get_script_path(wchar_t* path, DWORD capacity) {
  const DWORD length = GetModuleFileNameW(module_instance, path, capacity);
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
  const HMODULE gcmz = GetModuleHandleW(L"GCMZDrops.aux2");
  if (!gcmz) return;
  const auto add_handler = reinterpret_cast<AddHandlerScriptFile>(
      GetProcAddress(gcmz, "AddHandlerScriptFile"));
  if (!add_handler) return;
  wchar_t script_path[32768];
  if (!get_script_path(script_path, _countof(script_path))) return;
  registered = add_handler(script_path);
}

}  // namespace

void initialize_importer(HINSTANCE instance) { module_instance = instance; }
void register_importer(HOST_APP_TABLE* host) {
  register_handler_script();
  // Register only .srt here. Video media remains exclusively on AviUtl2's
  // normal input-plugin path, where GCMZDrops2 replaces matching pairs.
  if (host && host->register_file_drop_handler) {
    host->register_file_drop_handler(L"SRTrack SRT", L"SRT 字幕 (*.srt)\0*.srt\0\0",
                                     on_srt_file_drop);
  }
}
