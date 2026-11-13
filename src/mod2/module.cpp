#include <windows.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <string>
#include "../../third_party/aviutl2-sdk/module2.h"

static bool utf8_to_wide(const char* in,std::wstring& out){if(!in)return false;int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,in,-1,nullptr,0);if(!n)return false;out.resize(n);if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,in,-1,out.data(),n))return false;out.pop_back();return true;}
static void read_utf8_file(SCRIPT_MODULE_PARAM* p){std::wstring path;if(!utf8_to_wide(p->get_param_string(0),path)||path.empty()){p->set_error(u8"SRT ファイルのパスが不正です");return;}HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE){p->set_error(u8"SRT ファイルを読み込めません");return;}LARGE_INTEGER size{};if(!GetFileSizeEx(f,&size)||size.QuadPart<0||size.QuadPart>16*1024*1024){CloseHandle(f);p->set_error(u8"SRT ファイルのサイズを取得できないか、大きすぎます");return;}std::string bytes(size_t(size.QuadPart),'\0');DWORD read=0;bool ok=bytes.empty()||(ReadFile(f,bytes.data(),DWORD(bytes.size()),&read,nullptr)&&read==bytes.size());CloseHandle(f);if(!ok||bytes.find('\0')!=std::string::npos){p->set_error(u8"SRT ファイルの内容を取得できません");return;}p->push_result_string(bytes.c_str());}
static SCRIPT_MODULE_FUNCTION functions[]={{L"read_utf8_file",read_utf8_file},{nullptr,nullptr}};
static SCRIPT_MODULE_TABLE table={L"SRTrack Module 0.2 by p_cyclase",functions};
extern "C" __declspec(dllexport) SCRIPT_MODULE_TABLE* GetScriptModuleTable(){return &table;}
