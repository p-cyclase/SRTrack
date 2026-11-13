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
constexpr wchar_t registry_key[] = L"Software\\SRTrack";

struct ImporterSettings {
  bool video_pair = true;
  bool srt_drop = true;
  bool standalone = false;
};

ImporterSettings importer_settings;

bool read_dword(HKEY key, const wchar_t* name, bool fallback) {
  DWORD type = 0, value = 0, size = sizeof(value);
  if (RegQueryValueExW(key, name, nullptr, &type,
                       reinterpret_cast<BYTE*>(&value), &size) != ERROR_SUCCESS ||
      type != REG_DWORD || size != sizeof(value)) {
    return fallback;
  }
  return value != 0;
}

void load_importer_settings() {
  ImporterSettings loaded;
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, registry_key, 0, KEY_QUERY_VALUE,
                    &key) != ERROR_SUCCESS) {
    importer_settings = loaded;
    return;
  }
  loaded.video_pair = read_dword(key, L"importer_video_pair", loaded.video_pair);
  loaded.srt_drop = read_dword(key, L"importer_srt_drop", loaded.srt_drop);
  loaded.standalone = read_dword(key, L"importer_srt_standalone", loaded.standalone);
  RegCloseKey(key);
  importer_settings = loaded;
}

bool save_importer_settings(const ImporterSettings& settings) {
  HKEY key = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, registry_key, 0, nullptr, 0,
                      KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
    return false;
  }
  const auto put = [key](const wchar_t* name, bool value) {
    const DWORD stored = value ? 1 : 0;
    return RegSetValueExW(key, name, 0, REG_DWORD,
                          reinterpret_cast<const BYTE*>(&stored),
                          sizeof(stored)) == ERROR_SUCCESS;
  };
  const bool ok = put(L"importer_video_pair", settings.video_pair) &&
                  put(L"importer_srt_drop", settings.srt_drop) &&
                  put(L"importer_srt_standalone", settings.standalone);
  RegCloseKey(key);
  return ok;
}

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

void on_srt_file_drop(EDIT_SECTION* edit, LPCWSTR file) {
  if (!edit || !edit->info || !file) return;
  std::string path = wide_to_utf8(file);
  if (path.empty() || path.find_first_of("\r\n") != std::string::npos) return;
  const int length = srt_length_frames(file, edit->info);
  const wchar_t* effect = importer_settings.standalone
      ? L"SRTrackObject(Standalone)" : L"SRTrackObject";
  OBJECT_HANDLE object = edit->create_object(effect, edit->info->layer,
                                              edit->info->frame, length);
  if (!object) return;
  // This is intentionally not an alias: a lone SRT has no paired media or
  // group structure to reproduce.  It is the same Object a user can insert
  // from AviUtl2's Object menu, with only its initial values supplied here.
  edit->set_object_item_value(object, effect, L"SRT ファイル", path.c_str());
  if (!importer_settings.standalone) {
    edit->set_object_item_value(object, effect, L"動画全長", "0.000");
  }
  edit->set_object_item_value(object, effect, L"字幕プレビュー", "1");
}

void enable_srt_object_choices(HWND dialog) {
  const BOOL enabled = IsDlgButtonChecked(dialog, 2002) == BST_CHECKED;
  EnableWindow(GetDlgItem(dialog, 2003), enabled);
  EnableWindow(GetDlgItem(dialog, 2004), enabled);
}

INT_PTR CALLBACK importer_settings_dialog(HWND dialog, UINT message,
                                           WPARAM wparam, LPARAM) {
  switch (message) {
    case WM_INITDIALOG:
      CheckDlgButton(dialog, 2001, importer_settings.video_pair ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dialog, 2002, importer_settings.srt_drop ? BST_CHECKED : BST_UNCHECKED);
      CheckRadioButton(dialog, 2003, 2004,
                       importer_settings.standalone ? 2004 : 2003);
      enable_srt_object_choices(dialog);
      return TRUE;
    case WM_COMMAND:
      if (LOWORD(wparam) == 2002) {
        enable_srt_object_choices(dialog);
        return TRUE;
      }
      if (LOWORD(wparam) == IDOK) {
        ImporterSettings next;
        next.video_pair = IsDlgButtonChecked(dialog, 2001) == BST_CHECKED;
        next.srt_drop = IsDlgButtonChecked(dialog, 2002) == BST_CHECKED;
        next.standalone = IsDlgButtonChecked(dialog, 2004) == BST_CHECKED;
        if (!save_importer_settings(next)) {
          MessageBoxW(dialog, L"Importer 設定を保存できませんでした。",
                      L"SRTrack Importer 設定", MB_ICONERROR);
          return TRUE;
        }
        EndDialog(dialog, IDOK);
        return TRUE;
      }
      if (LOWORD(wparam) == IDCANCEL) {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
      }
      break;
    case WM_CLOSE:
      EndDialog(dialog, IDCANCEL);
      return TRUE;
  }
  return FALSE;
}

void open_importer_settings(HWND hwnd, HINSTANCE dll_hinst) {
  DialogBoxParamW(dll_hinst ? dll_hinst : module_instance,
                  MAKEINTRESOURCEW(102), hwnd, importer_settings_dialog, 0);
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
  load_importer_settings();
  if (!host) return;
  // Each entrance is registered independently. Disabled entrances do not
  // participate in the host's file-drop dispatch at all.
  if (importer_settings.video_pair) register_handler_script();
  if (importer_settings.srt_drop && host->register_file_drop_handler) {
    host->register_file_drop_handler(L"SRTrack SRT", L"SRT 字幕 (*.srt)\0*.srt\0\0",
                                     on_srt_file_drop);
  }
  if (host->register_config_menu) {
    host->register_config_menu(L"SRTrack Importer 設定…", open_importer_settings);
  }
}
