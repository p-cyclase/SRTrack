#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#include "diagnostics.h"
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
  bool fix = false;
  bool preview_enabled = true;
};

ImporterSettings importer_settings;
constexpr int settings_tab_id = 2010;
HWND settings_page = nullptr;
HWND diagnostics_page = nullptr;

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
  // Preserve the existing user's independent-SRT selection during the
  // pre-release rename, then persist subsequent changes under the new key.
  loaded.fix = read_dword(key, L"importer_srt_fix",
                          read_dword(key, L"importer_srt_standalone", loaded.fix));
  loaded.preview_enabled = read_dword(key, L"preview_enabled", loaded.preview_enabled);
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
                  put(L"importer_srt_fix", settings.fix) &&
                  put(L"preview_enabled", settings.preview_enabled);
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

int srt_length_frames(const wchar_t* path, const EDIT_INFO* info, bool* readable) {
  if (readable) *readable = false;
  if (!info || info->rate <= 0 || info->scale <= 0) return 1;
  std::string contents;
  if (!read_file(path, contents)) return 1;
  if (readable) *readable = true;
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
  if (!edit || !edit->info || !file) {
    add_diagnostic(DiagnosticSeverity::error, L"Importer", L"IMP-SRT-INPUT",
                   L"SRT 単体ドロップの入力情報を取得できません。", L"Object を作成しませんでした。");
    return;
  }
  std::string path = wide_to_utf8(file);
  if (path.empty() || path.find_first_of("\r\n") != std::string::npos) {
    add_diagnostic(DiagnosticSeverity::error, L"Importer", L"IMP-SRT-PATH",
                   L"SRT ファイルのパスを安全に扱えません。", L"Object を作成しませんでした。");
    return;
  }
  bool readable = false;
  const int length = srt_length_frames(file, edit->info, &readable);
  if (!readable) {
    add_diagnostic(DiagnosticSeverity::warning, L"Importer", L"IMP-SRT-READ",
                   L"SRT ファイルを読み込めないため、最小の長さで Object を作成します。",
                   L"Object を作成して設定を続行しました。");
  }
  const wchar_t* effect = importer_settings.fix
      ? L"SRTrackObject(fix)" : L"SRTrackObject";
  OBJECT_HANDLE object = edit->create_object(effect, edit->info->layer,
                                              edit->info->frame, length);
  if (!object) {
    add_diagnostic(DiagnosticSeverity::error, L"Importer", L"IMP-SRT-CREATE",
                   L"SRT 用 Object を作成できません。", L"通常入力へは委譲できませんでした。",
                   effect, edit->info->layer, edit->info->frame,
                   edit->info->frame + length - 1);
    return;
  }
  // This is intentionally not an alias: a lone SRT has no paired media or
  // group structure to reproduce.  It is the same Object a user can insert
  // from AviUtl2's Object menu, with only its initial values supplied here.
  edit->set_object_item_value(object, effect, L"SRT ファイル", path.c_str());
  edit->set_object_item_value(object, effect, L"全長", "0.000");
  edit->set_object_item_value(object, effect, L"字幕プレビュー", "1");
  add_diagnostic(DiagnosticSeverity::info, L"Importer", L"IMP-SRT-CREATE",
                 L"SRT 単体ドロップから Object を作成しました。", L"SRTrack で取り込みました。",
                 effect, edit->info->layer, edit->info->frame,
                 edit->info->frame + length - 1);
}

void refresh_diagnostic_log(HWND dialog) {
  HWND list = diagnostics_page ? GetDlgItem(diagnostics_page, 2005)
                               : GetDlgItem(dialog, 2005);
  if (!list) return;
  SendMessageW(list, LB_RESETCONTENT, 0, 0);
  const auto entries = snapshot_diagnostics();
  if (entries.empty()) {
    SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"診断ログはありません。"));
    return;
  }
  for (const auto& entry : entries) {
    std::wstring line = L"[";
    line += diagnostic_severity_name(entry.severity);
    line += L"] ";
    line += entry.feature;
    line += L" ";
    line += entry.code;
    line += L" — ";
    line += entry.message;
    SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
  }
}

HWND settings_control(HWND dialog, int id) {
  HWND control = GetDlgItem(dialog, id);
  if (control) return control;
  if (settings_page && (control = GetDlgItem(settings_page, id))) return control;
  if (diagnostics_page && (control = GetDlgItem(diagnostics_page, id))) return control;
  return nullptr;
}

void enable_srt_object_choices(HWND dialog) {
  const HWND srt_drop = settings_control(dialog, 2002);
  const BOOL enabled = srt_drop && SendMessageW(srt_drop, BM_GETCHECK, 0, 0) == BST_CHECKED;
  EnableWindow(settings_control(dialog, 2003), enabled);
  EnableWindow(settings_control(dialog, 2004), enabled);
}

void show_settings_tab(HWND dialog, int tab) {
  ShowWindow(settings_page, tab == 0 ? SW_SHOW : SW_HIDE);
  ShowWindow(diagnostics_page, tab == 1 ? SW_SHOW : SW_HIDE);
}

INT_PTR CALLBACK settings_page_dialog(HWND page, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message == WM_COMMAND) return SendMessageW(GetParent(page), message, wparam, lparam);
  return FALSE;
}

INT_PTR CALLBACK importer_settings_dialog(HWND dialog, UINT message,
                                           WPARAM wparam, LPARAM lparam) {
  switch (message) {
    case WM_INITDIALOG: {
      HWND tabs = GetDlgItem(dialog, settings_tab_id);
      TCITEMW item{};
      item.mask = TCIF_TEXT;
      item.pszText = const_cast<LPWSTR>(L"設定");
      SendMessageW(tabs, TCM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
      item.pszText = const_cast<LPWSTR>(L"診断ログ");
      SendMessageW(tabs, TCM_INSERTITEMW, 1, reinterpret_cast<LPARAM>(&item));
      settings_page = CreateDialogParamW(module_instance, MAKEINTRESOURCEW(106), dialog,
                                          settings_page_dialog, 0);
      diagnostics_page = CreateDialogParamW(module_instance, MAKEINTRESOURCEW(107), dialog,
                                             settings_page_dialog, 0);
      RECT area{};
      GetClientRect(tabs, &area);
      TabCtrl_AdjustRect(tabs, FALSE, &area);
      MapWindowPoints(tabs, dialog, reinterpret_cast<POINT*>(&area), 2);
      const int width = area.right - area.left;
      const int height = area.bottom - area.top;
      SetWindowPos(settings_page, HWND_TOP, area.left, area.top, width, height, SWP_NOACTIVATE);
      SetWindowPos(diagnostics_page, HWND_TOP, area.left, area.top, width, height, SWP_NOACTIVATE);
      TabCtrl_SetCurSel(tabs, 0);
      show_settings_tab(dialog, 0);
      SendMessageW(settings_control(dialog, 2001), BM_SETCHECK,
                   importer_settings.video_pair ? BST_CHECKED : BST_UNCHECKED, 0);
      SendMessageW(settings_control(dialog, 2002), BM_SETCHECK,
                   importer_settings.srt_drop ? BST_CHECKED : BST_UNCHECKED, 0);
      SendMessageW(settings_control(dialog, 2008), BM_SETCHECK,
                   importer_settings.preview_enabled ? BST_CHECKED : BST_UNCHECKED, 0);
      CheckRadioButton(settings_page, 2003, 2004, importer_settings.fix ? 2004 : 2003);
      enable_srt_object_choices(dialog);
      refresh_diagnostic_log(dialog);
      return TRUE;
    }
    case WM_NOTIFY: {
      const auto* notification = reinterpret_cast<const NMHDR*>(lparam);
      if (notification && notification->idFrom == settings_tab_id &&
          notification->code == TCN_SELCHANGE) {
        show_settings_tab(dialog, TabCtrl_GetCurSel(GetDlgItem(dialog, settings_tab_id)));
        return TRUE;
      }
      break;
    }
    case WM_COMMAND:
      if (LOWORD(wparam) == 2002) {
        enable_srt_object_choices(dialog);
        return TRUE;
      }
      if (LOWORD(wparam) == 2006) {
        if (!copy_diagnostic_report(dialog)) {
          MessageBoxW(dialog, L"診断情報をクリップボードへコピーできませんでした。",
                      L"SRTrack 診断ログ", MB_ICONWARNING);
        }
        return TRUE;
      }
      if (LOWORD(wparam) == 2007) {
        clear_diagnostics();
        refresh_diagnostic_log(dialog);
        return TRUE;
      }
      if (LOWORD(wparam) == IDOK) {
        ImporterSettings next;
        next.video_pair = SendMessageW(settings_control(dialog, 2001), BM_GETCHECK, 0, 0) == BST_CHECKED;
        next.srt_drop = SendMessageW(settings_control(dialog, 2002), BM_GETCHECK, 0, 0) == BST_CHECKED;
        next.fix = SendMessageW(settings_control(dialog, 2004), BM_GETCHECK, 0, 0) == BST_CHECKED;
        next.preview_enabled = SendMessageW(settings_control(dialog, 2008), BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (!save_importer_settings(next)) {
          MessageBoxW(dialog, L"設定を保存できませんでした。",
                      L"SRTrack 設定", MB_ICONERROR);
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
  if (!gcmz) {
    add_diagnostic(DiagnosticSeverity::warning, L"Importer", L"IMP-HANDLER-UNAVAILABLE",
                   L"GCMZDrops2 を取得できないため、動画ペア取込を登録できません。",
                   L"通常の動画ドロップへ委ねます。");
    return;
  }
  const auto add_handler = reinterpret_cast<AddHandlerScriptFile>(
      GetProcAddress(gcmz, "AddHandlerScriptFile"));
  if (!add_handler) {
    add_diagnostic(DiagnosticSeverity::warning, L"Importer", L"IMP-HANDLER-API",
                   L"GCMZDrops2 の公開ハンドラー API を取得できません。",
                   L"通常の動画ドロップへ委ねます。");
    return;
  }
  wchar_t script_path[32768];
  if (!get_script_path(script_path, _countof(script_path))) {
    add_diagnostic(DiagnosticSeverity::error, L"Importer", L"IMP-HANDLER-PATH",
                   L"動画ペア取込用スクリプトの場所を取得できません。",
                   L"通常の動画ドロップへ委ねます。");
    return;
  }
  registered = add_handler(script_path);
  add_diagnostic(registered ? DiagnosticSeverity::info : DiagnosticSeverity::warning,
                 L"Importer", registered ? L"IMP-HANDLER-REGISTERED" : L"IMP-HANDLER-REGISTER",
                 registered ? L"動画ペア取込ハンドラーを登録しました。"
                            : L"動画ペア取込ハンドラーを登録できません。",
                 registered ? L"次回以降の動画ドロップで利用します。"
                            : L"通常の動画ドロップへ委ねます。");
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
    host->register_config_menu(L"SRTrack 設定…", open_importer_settings);
  }
}
