#pragma once

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

enum class DiagnosticSeverity { info, warning, error };

struct DiagnosticEntry {
  SYSTEMTIME time{};
  DiagnosticSeverity severity = DiagnosticSeverity::info;
  std::wstring feature;
  std::wstring code;
  std::wstring message;
  std::wstring result;
  std::wstring object_kind;
  int layer = -1;
  int start_frame = -1;
  int end_frame = -1;
};

inline std::mutex diagnostic_mutex;
inline std::vector<DiagnosticEntry> diagnostic_entries;
inline constexpr size_t diagnostic_capacity = 200;

inline const wchar_t* diagnostic_severity_name(DiagnosticSeverity severity) {
  switch (severity) {
    case DiagnosticSeverity::info: return L"情報";
    case DiagnosticSeverity::warning: return L"警告";
    case DiagnosticSeverity::error: return L"エラー";
  }
  return L"情報";
}

inline void add_diagnostic(DiagnosticSeverity severity, const wchar_t* feature,
                           const wchar_t* code, const std::wstring& message,
                           const std::wstring& result = L"",
                           const wchar_t* object_kind = L"",
                           int layer = -1, int start_frame = -1,
                           int end_frame = -1) {
  DiagnosticEntry entry;
  GetLocalTime(&entry.time);
  entry.severity = severity;
  entry.feature = feature ? feature : L"";
  entry.code = code ? code : L"";
  entry.message = message;
  entry.result = result;
  entry.object_kind = object_kind ? object_kind : L"";
  entry.layer = layer;
  entry.start_frame = start_frame;
  entry.end_frame = end_frame;
  std::lock_guard<std::mutex> lock(diagnostic_mutex);
  diagnostic_entries.push_back(std::move(entry));
  if (diagnostic_entries.size() > diagnostic_capacity) {
    diagnostic_entries.erase(diagnostic_entries.begin(),
                             diagnostic_entries.begin() +
                                 (diagnostic_entries.size() - diagnostic_capacity));
  }
}

inline std::vector<DiagnosticEntry> snapshot_diagnostics() {
  std::lock_guard<std::mutex> lock(diagnostic_mutex);
  return diagnostic_entries;
}

inline void clear_diagnostics() {
  std::lock_guard<std::mutex> lock(diagnostic_mutex);
  diagnostic_entries.clear();
}

inline std::wstring format_diagnostic_entry(const DiagnosticEntry& entry) {
  wchar_t timestamp[32]{};
  swprintf_s(timestamp, L"%04u-%02u-%02u %02u:%02u:%02u", entry.time.wYear,
             entry.time.wMonth, entry.time.wDay, entry.time.wHour,
             entry.time.wMinute, entry.time.wSecond);
  std::wstring text = timestamp;
  text += L" [";
  text += diagnostic_severity_name(entry.severity);
  text += L"] ";
  text += entry.feature;
  text += L" ";
  text += entry.code;
  text += L"\n  ";
  text += entry.message;
  if (!entry.result.empty()) text += L"\n  結果: " + entry.result;
  if (!entry.object_kind.empty()) text += L"\n  Object: " + entry.object_kind;
  if (entry.layer >= 0) text += L"\n  レイヤー: " + std::to_wstring(entry.layer + 1);
  if (entry.start_frame >= 0 || entry.end_frame >= 0) {
    text += L"\n  フレーム: ";
    text += entry.start_frame >= 0 ? std::to_wstring(entry.start_frame) : L"?";
    text += L"-";
    text += entry.end_frame >= 0 ? std::to_wstring(entry.end_frame) : L"?";
  }
  return text;
}

inline std::wstring format_diagnostic_report(const std::vector<DiagnosticEntry>& entries) {
  std::wstring report = L"SRTrack 診断情報\n";
  report += L"SRTrack 版: 開発版\n";
  report += L"AviUtl2 版: 取得不可\n";
  report += L"記録件数: " + std::to_wstring(entries.size()) + L"\n";
  report += L"※ 絶対パス、SRT本文、動画名は記録していません。\n\n";
  for (const auto& entry : entries) {
    report += format_diagnostic_entry(entry);
    report += L"\n\n";
  }
  return report;
}

inline std::wstring format_diagnostic_report() {
  return format_diagnostic_report(snapshot_diagnostics());
}

inline std::wstring format_diagnostics_since(size_t start_index) {
  const auto entries = snapshot_diagnostics();
  if (start_index >= entries.size()) return format_diagnostic_report({});
  return format_diagnostic_report(std::vector<DiagnosticEntry>(entries.begin() + start_index,
                                                               entries.end()));
}

inline bool copy_diagnostic_report(HWND owner) {
  const std::wstring report = format_diagnostic_report();
  if (!OpenClipboard(owner)) return false;
  EmptyClipboard();
  const size_t bytes = (report.size() + 1) * sizeof(wchar_t);
  HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!memory) {
    CloseClipboard();
    return false;
  }
  void* destination = GlobalLock(memory);
  if (!destination) {
    GlobalFree(memory);
    CloseClipboard();
    return false;
  }
  memcpy(destination, report.c_str(), bytes);
  GlobalUnlock(memory);
  if (!SetClipboardData(CF_UNICODETEXT, memory)) {
    GlobalFree(memory);
    CloseClipboard();
    return false;
  }
  CloseClipboard();
  return true;
}
