#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <string>

#include "../../third_party/aviutl2-sdk/plugin2.h"
#include "monitor.h"

namespace {

constexpr UINT k_sync_message = WM_APP + 0x4d;
constexpr UINT_PTR k_sync_timer = 1;
constexpr UINT k_sync_delay_ms = 80;

HINSTANCE module_instance = nullptr;
EDIT_HANDLE* edit_handle = nullptr;
HWND monitor_window = nullptr;
std::atomic_bool sync_queued = false;

std::string get_item(EDIT_SECTION* edit, OBJECT_HANDLE object, LPCWSTR effect, LPCWSTR item) {
  const char* value = edit->get_object_item_value(object, effect, item);
  return value ? value : "";
}

bool is_true(const std::string& value) {
  return value == "1" || value == "true";
}

bool utf8_to_wide(const std::string& input, std::wstring& output) {
  if (input.empty()) { output.clear(); return true; }
  const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), nullptr, 0);
  if (!length) return false;
  output.resize(static_cast<size_t>(length));
  return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), output.data(), length) == length;
}

bool wide_to_utf8(const std::wstring& input, std::string& output) {
  const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
  if (!length && !input.empty()) return false;
  output.resize(static_cast<size_t>(length));
  return length == 0 || WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, input.data(), static_cast<int>(input.size()), output.data(), length, nullptr, nullptr) == length;
}

bool split_video_range(const std::string& value, std::string& position, std::string& end_position) {
  const size_t first = value.find(',');
  if (first == std::string::npos || first == 0) return false;
  const size_t second = value.find(',', first + 1);
  if (second == std::string::npos || second == first + 1) return false;
  position = value.substr(0, first);
  end_position = value.substr(first + 1, second - first - 1);
  return true;
}

std::wstring sibling_srt(const std::wstring& video_path) {
  const size_t slash = video_path.find_last_of(L"\\/");
  const size_t dot = video_path.find_last_of(L'.');
  if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash)) return L"";
  return video_path.substr(0, dot) + L".srt";
}

std::wstring filename(const std::wstring& path) {
  const size_t slash = path.find_last_of(L"\\/");
  return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

bool regular_file_exists(const std::wstring& path) {
  if (path.empty()) return false;
  const DWORD attributes = GetFileAttributesW(path.c_str());
  return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

void set_item_if_changed(EDIT_SECTION* edit, OBJECT_HANDLE object, LPCWSTR effect, LPCWSTR item, const std::string& value) {
  if (get_item(edit, object, effect, item) != value) edit->set_object_item_value(object, effect, item, value.c_str());
}

void set_name_if_changed(EDIT_SECTION* edit, OBJECT_HANDLE object, const std::wstring& name) {
  const LPCWSTR current = edit->get_object_name(object);
  if (name.empty()) {
    if (current && *current) edit->set_object_name(object, nullptr);
  } else if (!current || name != current) {
    edit->set_object_name(object, name.c_str());
  }
}

void sync_file_and_name(EDIT_SECTION* edit, OBJECT_HANDLE object, const std::string& video_file) {
  std::wstring video_path;
  if (!utf8_to_wide(video_file, video_path)) return;
  const std::wstring srt_path = sibling_srt(video_path);
  if (regular_file_exists(srt_path)) {
    std::string utf8_path;
    if (!wide_to_utf8(srt_path, utf8_path)) return;
    set_item_if_changed(edit, object, L"SRTrack", L"SRT ファイル", utf8_path);
    set_name_if_changed(edit, object, filename(srt_path));
  } else {
    set_item_if_changed(edit, object, L"SRTrack", L"SRT ファイル", "");
    set_name_if_changed(edit, object, L"");
  }
}

void sync_pair(EDIT_SECTION* edit, OBJECT_HANDLE object, OBJECT_HANDLE video) {
  const std::string range = get_item(edit, video, L"動画ファイル", L"再生位置");
  const std::string speed = get_item(edit, video, L"動画ファイル", L"再生速度");
  const std::string loop = get_item(edit, video, L"動画ファイル", L"ループ再生");
  const std::string file = get_item(edit, video, L"動画ファイル", L"ファイル");
  std::string position, end_position;
  if (!split_video_range(range, position, end_position) || speed.empty() || loop.empty() || file.empty()) return;

  set_item_if_changed(edit, object, L"SRTrack", L"再生位置", position);
  set_item_if_changed(edit, object, L"SRTrack", L"再生範囲終端", end_position);
  set_item_if_changed(edit, object, L"SRTrack", L"再生速度", speed);
  set_item_if_changed(edit, object, L"SRTrack", L"ループ再生", is_true(loop) ? "1" : "0");
  sync_file_and_name(edit, object, file);

  const OBJECT_LAYER_FRAME video_range = edit->get_object_layer_frame(video);
  OBJECT_LAYER_FRAME object_range = edit->get_object_layer_frame(object);
  if (object_range.layer != video_range.layer + 1 || object_range.start != video_range.start) {
    edit->move_object(object, video_range.layer + 1, video_range.start);
    object_range = edit->get_object_layer_frame(object);
  }
  if (object_range.end != video_range.end) {
    const int section_num = edit->get_object_section_num(object);
    if (section_num >= 1) edit->move_object_section(object, section_num, video_range.end + 1);
  }
}

void synchronize_all(EDIT_SECTION* edit) {
  if (!edit || !edit->info) return;
  try {
    for (int layer = 1; layer <= edit->info->layer_max; ++layer) {
      int frame = 0;
      while (frame <= edit->info->frame_max) {
        OBJECT_HANDLE object = edit->find_object(layer, frame);
        if (!object) break;
        const OBJECT_LAYER_FRAME object_range = edit->get_object_layer_frame(object);
        if (object_range.layer != layer || object_range.end < frame || object_range.end == INT_MAX) break;
        frame = object_range.end + 1;

        if (edit->count_object_effect(object, L"SRTrack") == 0 ||
            !is_true(get_item(edit, object, L"SRTrack", L"動画同期"))) continue;

        OBJECT_HANDLE video = edit->find_object(layer - 1, object_range.start);
        if (!video || edit->count_object_effect(video, L"動画ファイル") == 0) continue;
        const OBJECT_LAYER_FRAME video_range = edit->get_object_layer_frame(video);
        const bool overlaps = video_range.start <= object_range.end && object_range.start <= video_range.end;
        if (!overlaps) continue;
        sync_pair(edit, object, video);
      }
    }
  } catch (...) {
    // A malformed or concurrently changing project must never interrupt editing.
  }
}

LRESULT CALLBACK monitor_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message == k_sync_message) {
    SetTimer(window, k_sync_timer, k_sync_delay_ms, nullptr);
    return 0;
  }
  if (message == WM_TIMER && wparam == k_sync_timer) {
    KillTimer(window, k_sync_timer);
    sync_queued.store(false);
    if (edit_handle) edit_handle->call_edit_section(synchronize_all);
    return 0;
  }
  return DefWindowProcW(window, message, wparam, lparam);
}

void on_update_object(void*) {
  if (monitor_window && !sync_queued.exchange(true)) PostMessageW(monitor_window, k_sync_message, 0, 0);
}

}  // namespace

void initialize_monitor(HINSTANCE instance) {
  module_instance = instance;
}

void register_monitor(HOST_APP_TABLE* host) {
  if (!host || monitor_window) return;
  edit_handle = host->create_edit_handle();
  if (!edit_handle) return;

  WNDCLASSW window_class{};
  window_class.hInstance = module_instance;
  window_class.lpfnWndProc = monitor_window_proc;
  window_class.lpszClassName = L"SRTrackMonitorWindow";
  RegisterClassW(&window_class);
  monitor_window = CreateWindowExW(0, window_class.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, module_instance, nullptr);
  if (!monitor_window) return;
  host->register_event_listener(EVENT_TYPE::UPDATE_OBJECT, nullptr, on_update_object);
}
