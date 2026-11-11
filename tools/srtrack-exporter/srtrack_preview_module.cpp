#include <windows.h>
#include <string>
#include "third_party/aviutl2-sdk/module2.h"

static bool utf8_to_wide(const char* input, std::wstring& output) {
    if (!input) return false;
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, nullptr, 0);
    if (!length) return false;
    output.resize(static_cast<size_t>(length));
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, output.data(), length)) return false;
    output.pop_back();
    return true;
}

static void read_utf8_file(SCRIPT_MODULE_PARAM* param) {
    std::wstring path;
    if (!utf8_to_wide(param->get_param_string(0), path) || path.empty()) { param->set_error(u8"SRT ファイルのパスが不正です"); return; }
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { param->set_error(u8"SRT ファイルを読み込めません"); return; }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > 16 * 1024 * 1024) { CloseHandle(file); param->set_error(u8"SRT ファイルのサイズを取得できないか、大きすぎます"); return; }
    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    const bool ok = bytes.empty() || (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size());
    CloseHandle(file);
    if (!ok || bytes.find('\0') != std::string::npos) { param->set_error(u8"SRT ファイルの内容を取得できません"); return; }
    param->push_result_string(bytes.c_str());
}

static SCRIPT_MODULE_FUNCTION functions[] = {{L"read_utf8_file", read_utf8_file}, {nullptr, nullptr}};
static SCRIPT_MODULE_TABLE module_table = {L"SRTrack Preview Module 0.1 by p_cyclase", functions};
extern "C" __declspec(dllexport) SCRIPT_MODULE_TABLE* GetScriptModuleTable() { return &module_table; }
