#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include "../../third_party/aviutl2-sdk/module2.h"
#include "../../third_party/aviutl2-sdk/plugin2.h"

static bool utf8_to_wide(const char* in,std::wstring& out){if(!in)return false;int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,in,-1,nullptr,0);if(!n)return false;out.resize(n);if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,in,-1,out.data(),n))return false;out.pop_back();return true;}
static void read_utf8_file(SCRIPT_MODULE_PARAM* p){std::wstring path;if(!utf8_to_wide(p->get_param_string(0),path)||path.empty()){p->set_error(u8"SRT ファイルのパスが不正です");return;}HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE){p->set_error(u8"SRT ファイルを読み込めません");return;}LARGE_INTEGER size{};if(!GetFileSizeEx(f,&size)||size.QuadPart<0||size.QuadPart>16*1024*1024){CloseHandle(f);p->set_error(u8"SRT ファイルのサイズを取得できないか、大きすぎます");return;}std::string bytes(size_t(size.QuadPart),'\0');DWORD read=0;bool ok=bytes.empty()||(ReadFile(f,bytes.data(),DWORD(bytes.size()),&read,nullptr)&&read==bytes.size());CloseHandle(f);if(!ok||bytes.find('\0')!=std::string::npos){p->set_error(u8"SRT ファイルの内容を取得できません");return;}p->push_result_string(bytes.c_str());}
static void preview_enabled(SCRIPT_MODULE_PARAM* p){DWORD value=1,type=0,size=sizeof(value);HKEY key=nullptr;bool enabled=true;if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\SRTrack",0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS){if(RegQueryValueExW(key,L"preview_enabled",nullptr,&type,reinterpret_cast<BYTE*>(&value),&size)==ERROR_SUCCESS&&type==REG_DWORD&&size==sizeof(value))enabled=value!=0;RegCloseKey(key);}p->push_result_boolean(enabled);}
static bool parse_first_number(const char* text,double& value){
 if(!text)return false;errno=0;char* end=nullptr;value=std::strtod(text,&end);
 return end!=text&&errno!=ERANGE&&std::isfinite(value);
}
static double frame_milliseconds(double frame_rate,int frame){return double(frame)*1000.0/frame_rate;}
static bool read_video_speed(EDIT_SECTION* edit,OBJECT_HANDLE object,int frame,double& speed){
 if(edit->get_object_track_value&&edit->get_object_track_value(object,L"動画ファイル",L"再生速度",frame,&speed)&&std::isfinite(speed)&&speed>0)return true;
 return parse_first_number(edit->get_object_item_value(object,L"動画ファイル",L"再生速度"),speed)&&speed>0;
}
static const char* unavailable_preview_api(EDIT_SECTION* edit){
 if(!edit)return "edit";
 if(!edit->find_object)return "find_object";
 if(!edit->count_object_effect)return "count_effect";
 if(!edit->get_object_layer_frame)return "object_location";
 if(!edit->get_object_item_value)return "item_value";
 if(!edit->get_object_section_num)return "section_num";
 if(!edit->get_object_section_frame)return "section_frame";
 return nullptr;
}
struct PreviewTimingResult { int video_layer=-1; int section_count=0; };
static bool preview_video_source_time(EDIT_SECTION* edit,int srtrack_layer,int frame,double frame_rate,double& source_time,PreviewTimingResult* result=nullptr,std::string* failure=nullptr){
 auto fail=[&](const char* phase){if(failure)*failure=phase;return false;};
 if(!std::isfinite(frame_rate)||frame_rate<=0)return fail("frame_rate");
 if(const char* unavailable=unavailable_preview_api(edit))return fail(unavailable);
 for(int layer=srtrack_layer-2;layer>=0;--layer){
  const OBJECT_HANDLE object=edit->find_object(layer,frame);if(!object)continue;
  const OBJECT_LAYER_FRAME location=edit->get_object_layer_frame(object);
  if(location.layer!=layer||frame<location.start||frame>location.end||edit->count_object_effect(object,L"動画ファイル")<=0)continue;
  if(result)result->video_layer=layer+1;
  double position=0;if(!parse_first_number(edit->get_object_item_value(object,L"動画ファイル",L"再生位置"),position)||position<0)return fail("playback_position");
  const int count=edit->get_object_section_num(object);if(count<1)return fail("section_count");if(result)result->section_count=count;
  std::vector<int> boundaries{location.start};
  for(int section=1;section<count;++section){const int boundary=edit->get_object_section_frame(object,section);if(boundary<=location.start||boundary>location.end)return fail("section_frame");boundaries.push_back(boundary);}
  boundaries.push_back(location.end+1);source_time=position*1000.0;
  for(size_t i=0;i+1<boundaries.size();++i){
   const int start=boundaries[i],end=boundaries[i+1];double speed=0;
   if(end<=start||!read_video_speed(edit,object,start,speed))return fail("playback_speed");
   const int until=std::min(frame,end);
   source_time+=(speed/100.0)*(frame_milliseconds(frame_rate,until)-frame_milliseconds(frame_rate,start));
   if(frame<end)return true;
  }
  return fail("section_range");
 }
 return fail("video_not_found");
}
static void preview_source_time(SCRIPT_MODULE_PARAM* p){
 if(!p||p->get_param_num()<3)return;
 double source_time=0;if(preview_video_source_time(p->edit,p->get_param_int(0),p->get_param_int(1),p->get_param_double(2),source_time))p->push_result_double(source_time);
}
static SCRIPT_MODULE_FUNCTION functions[]={{L"read_utf8_file",read_utf8_file},{L"preview_enabled",preview_enabled},{L"preview_source_time",preview_source_time},{nullptr,nullptr}};
static SCRIPT_MODULE_TABLE table={L"SRTrack Module 0.2 by p_cyclase",functions};
extern "C" __declspec(dllexport) SCRIPT_MODULE_TABLE* GetScriptModuleTable(){return &table;}
