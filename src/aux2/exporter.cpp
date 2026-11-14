#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <set>
#include <string>
#include <vector>
#include "core.h"
#include "diagnostics.h"
#include "../../third_party/aviutl2-sdk/plugin2.h"
#include "importer.h"

static HINSTANCE module_handle;
static EDIT_HANDLE* export_edit_handle;
struct Settings {
 bool all=true, range_all=true, text=true, dialogue=true, srtrack=true;
 bool keep_ruby=true, keep_emoji=true, reflect_timing=true, upper_layer_first=true;
 std::wstring numbers=L"", names=L"", offset=L"0.000", minimum_duration=L"1.000";
};
static Settings settings;
static constexpr wchar_t registry_key[]=L"Software\\SRTrack";

static std::wstring read_string(HKEY k,const wchar_t* name,const std::wstring& fallback) {
 DWORD type=0,size=0;
 if(RegQueryValueExW(k,name,nullptr,&type,nullptr,&size)!=ERROR_SUCCESS || type!=REG_SZ || size>131072 || size<sizeof(wchar_t)) return fallback;
 std::vector<wchar_t> b(size/sizeof(wchar_t)+1,0);
 if(RegQueryValueExW(k,name,nullptr,&type,(BYTE*)b.data(),&size)!=ERROR_SUCCESS) return fallback;
 return b.data();
}
static void load_settings() {
 HKEY k; if(RegOpenKeyExW(HKEY_CURRENT_USER,registry_key,0,KEY_QUERY_VALUE,&k)!=ERROR_SUCCESS)return;
 settings.all=read_string(k,L"scope",L"all")==L"all";
 settings.range_all=read_string(k,L"range",L"all")==L"all";
 settings.text=read_string(k,L"text",L"1")==L"1";
 settings.dialogue=read_string(k,L"dialogue",L"1")==L"1";
 settings.srtrack=read_string(k,L"srtrack",L"1")==L"1";
 settings.keep_ruby=read_string(k,L"keep_ruby",L"1")==L"1";
 settings.keep_emoji=read_string(k,L"keep_emoji",L"1")==L"1";
 settings.reflect_timing=read_string(k,L"reflect_timing",L"1")==L"1";
 settings.upper_layer_first=read_string(k,L"simultaneous_order",L"upper")==L"upper";
 settings.numbers=read_string(k,L"numbers",L"");settings.names=read_string(k,L"names",L"");
 settings.offset=read_string(k,L"offset",L"0.000");settings.minimum_duration=read_string(k,L"final_min",L"1.000");
 RegCloseKey(k);
}
static bool save_settings() {
 HKEY k; if(RegCreateKeyExW(HKEY_CURRENT_USER,registry_key,0,nullptr,0,KEY_SET_VALUE,nullptr,&k,nullptr)!=ERROR_SUCCESS)return false;
 bool ok=true;
 auto put=[&](const wchar_t* n,const std::wstring& v){if(RegSetValueExW(k,n,0,REG_SZ,(const BYTE*)v.c_str(),DWORD((v.size()+1)*sizeof(wchar_t)))!=ERROR_SUCCESS)ok=false;};
 put(L"scope",settings.all?L"all":L"selected");put(L"range",settings.range_all?L"all":L"selected");put(L"text",settings.text?L"1":L"0");put(L"dialogue",settings.dialogue?L"1":L"0");put(L"srtrack",settings.srtrack?L"1":L"0");
 put(L"keep_ruby",settings.keep_ruby?L"1":L"0");put(L"keep_emoji",settings.keep_emoji?L"1":L"0");put(L"reflect_timing",settings.reflect_timing?L"1":L"0");put(L"simultaneous_order",settings.upper_layer_first?L"upper":L"lower");
 put(L"numbers",settings.numbers);put(L"names",settings.names);put(L"offset",settings.offset);put(L"final_min",settings.minimum_duration);
 RegCloseKey(k);return ok;
}
static bool parse_layers(const std::wstring& input,std::set<int>& layers) {
 const wchar_t* p=input.c_str();auto spaces=[&](){while(*p && iswspace(*p))++p;};spaces(); if(!*p)return true;
 while(true){spaces();if(*p<L'0'||*p>L'9')return false;wchar_t* end;long a=wcstol(p,&end,10);p=end;spaces();long b=a;
  if(*p==L'-'){++p;spaces();if(*p<L'0'||*p>L'9')return false;b=wcstol(p,&end,10);p=end;spaces();}
  if(a<1||b<a||b>10000)return false;for(long n=a;n<=b;++n)layers.insert(int(n-1));
  if(!*p)return true;if(*p!=L',')return false;++p;
 }
}
static std::vector<std::wstring> split_names(const std::wstring& input) {
 std::vector<std::wstring> names;size_t p=0;
 while(p<input.size()){size_t end=input.find(L'\n',p);if(end==std::wstring::npos)end=input.size();
  auto s=input.substr(p,end-p);if(!s.empty()&&s.back()==L'\r')s.pop_back();if(!s.empty())names.push_back(s);p=end+1;}
 return names;
}
static constexpr int exporter_tab_id=1030;
static HWND output_target_page;
static HWND subtitle_format_page;
static HWND exporter_control(HWND dialog,int id){
 HWND control=GetDlgItem(dialog,id);if(control)return control;
 if(output_target_page&&(control=GetDlgItem(output_target_page,id)))return control;
 if(subtitle_format_page&&(control=GetDlgItem(subtitle_format_page,id)))return control;
 return nullptr;
}
static std::wstring get_text(HWND h,int id) {
 HWND c=exporter_control(h,id);int n=GetWindowTextLengthW(c);std::vector<wchar_t> b(size_t(n)+1);GetWindowTextW(c,b.data(),n+1);return b.data();
}
static bool is_checked(HWND dialog,int id){return SendMessageW(exporter_control(dialog,id),BM_GETCHECK,0,0)==BST_CHECKED;}
static void set_checked(HWND dialog,int id,bool checked){SendMessageW(exporter_control(dialog,id),BM_SETCHECK,checked?BST_CHECKED:BST_UNCHECKED,0);}
static void show_exporter_tab(HWND dialog,int tab){
 ShowWindow(output_target_page,tab==0?SW_SHOW:SW_HIDE);
 ShowWindow(subtitle_format_page,tab==1?SW_SHOW:SW_HIDE);
}
static std::wstring format_seconds(int64_t milliseconds){
 const bool negative=milliseconds<0;const uint64_t value=negative?uint64_t(-milliseconds):uint64_t(milliseconds);
 wchar_t text[48]{};swprintf_s(text,L"%s%llu.%03llu",negative?L"-":L"",value/1000,value%1000);return text;
}
static void set_seconds_text(HWND h,int id,const std::wstring& value){
 int64_t milliseconds=0;SetWindowTextW(exporter_control(h,id),parse_offset(value,milliseconds)?format_seconds(milliseconds).c_str():value.c_str());
}
static void enable_scope(HWND h){BOOL enabled=is_checked(h,1011);EnableWindow(exporter_control(h,1001),enabled);EnableWindow(exporter_control(h,1012),enabled);}
static WNDPROC original_next_proc=nullptr;
static LRESULT CALLBACK next_button_proc(HWND button,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_GETDLGCODE && l && reinterpret_cast<MSG*>(l)->message==WM_KEYDOWN && w==VK_RETURN)
  return CallWindowProcW(original_next_proc,button,msg,w,l)|DLGC_WANTMESSAGE;
 if((msg==WM_KEYDOWN||msg==WM_KEYUP||msg==WM_CHAR)&&w==VK_RETURN)return 0;
 return CallWindowProcW(original_next_proc,button,msg,w,l);
}
static INT_PTR CALLBACK exporter_page(HWND page,UINT msg,WPARAM w,LPARAM lParam){
 if(msg==WM_COMMAND)return SendMessageW(GetParent(page),msg,w,lParam);
 return FALSE;
}
struct ExportResultDialogData { const std::wstring* text; bool diagnostics; };
static INT_PTR CALLBACK exporter_result_dialog(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_INITDIALOG){
  const auto* data=reinterpret_cast<const ExportResultDialogData*>(l);
  SetWindowLongPtrW(h,GWLP_USERDATA,l);
  SetWindowTextW(GetDlgItem(h,3001),data&&data->text?data->text->c_str():L"");
  EnableWindow(GetDlgItem(h,3002),data&&data->diagnostics);
  return TRUE;
 }
 if(msg==WM_COMMAND&&LOWORD(w)==3002){
  if(!copy_diagnostic_report(h))MessageBoxW(h,L"診断情報をクリップボードへコピーできませんでした。",L"SRTrack Exporter",MB_ICONWARNING);
  return TRUE;
 }
 if((msg==WM_COMMAND&&LOWORD(w)==IDOK)||msg==WM_CLOSE){EndDialog(h,IDOK);return TRUE;}
 return FALSE;
}
static void show_export_result(HWND owner,const std::wstring& text,bool diagnostics){
 ExportResultDialogData data{&text,diagnostics};
 DialogBoxParamW(module_handle,MAKEINTRESOURCEW(105),owner,exporter_result_dialog,reinterpret_cast<LPARAM>(&data));
}
static INT_PTR CALLBACK dialog(HWND h,UINT msg,WPARAM w,LPARAM lParam) {
 if(msg==WM_INITDIALOG){
  HWND tabs=GetDlgItem(h,exporter_tab_id);TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<LPWSTR>(L"出力対象");SendMessageW(tabs,TCM_FIRST+62,0,reinterpret_cast<LPARAM>(&item));item.pszText=const_cast<LPWSTR>(L"字幕の整形");SendMessageW(tabs,TCM_FIRST+62,1,reinterpret_cast<LPARAM>(&item));
  output_target_page=CreateDialogParamW(module_handle,MAKEINTRESOURCEW(103),h,exporter_page,0);subtitle_format_page=CreateDialogParamW(module_handle,MAKEINTRESOURCEW(104),h,exporter_page,0);RECT area{};GetClientRect(tabs,&area);TabCtrl_AdjustRect(tabs,FALSE,&area);MapWindowPoints(tabs,h,reinterpret_cast<POINT*>(&area),2);const int page_width=area.right-area.left,page_height=area.bottom-area.top;SetWindowPos(output_target_page,HWND_TOP,area.left,area.top,page_width,page_height,SWP_NOACTIVATE);SetWindowPos(subtitle_format_page,HWND_TOP,area.left,area.top,page_width,page_height,SWP_NOACTIVATE);TabCtrl_SetCurSel(tabs,0);show_exporter_tab(h,0);
  set_checked(h,1010,settings.all);set_checked(h,1011,!settings.all);set_checked(h,1018,settings.range_all);set_checked(h,1019,!settings.range_all);set_checked(h,1013,settings.text);set_checked(h,1014,settings.dialogue);set_checked(h,1017,settings.srtrack);set_checked(h,1021,settings.keep_ruby);set_checked(h,1022,settings.keep_emoji);set_checked(h,1025,settings.reflect_timing);set_checked(h,1023,settings.upper_layer_first);set_checked(h,1024,!settings.upper_layer_first);
  SetWindowTextW(exporter_control(h,1001),settings.numbers.c_str());SetWindowTextW(exporter_control(h,1012),settings.names.c_str());set_seconds_text(h,1002,settings.offset);set_seconds_text(h,1016,settings.minimum_duration);
  original_next_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(GetDlgItem(h,1020),GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(next_button_proc)));enable_scope(h);return TRUE;}
 if(msg==WM_NOTIFY){const auto* notification=reinterpret_cast<const NMHDR*>(lParam);if(notification&&notification->idFrom==exporter_tab_id&&notification->code==TCN_SELCHANGE){show_exporter_tab(h,TabCtrl_GetCurSel(GetDlgItem(h,exporter_tab_id)));return TRUE;}}
 if(msg==WM_COMMAND&&(LOWORD(w)==1010||LOWORD(w)==1011)){enable_scope(h);return TRUE;}
 if(msg==WM_COMMAND&&LOWORD(w)==IDOK)return TRUE;
 if(msg==WM_COMMAND&&LOWORD(w)==1020&&HIWORD(w)==BN_CLICKED){Settings next=settings;next.all=is_checked(h,1010);next.range_all=is_checked(h,1018);next.text=is_checked(h,1013);next.dialogue=is_checked(h,1014);
  next.numbers=get_text(h,1001);next.names=get_text(h,1012);next.offset=get_text(h,1002);next.minimum_duration=get_text(h,1016);next.srtrack=is_checked(h,1017);next.keep_ruby=is_checked(h,1021);next.keep_emoji=is_checked(h,1022);next.reflect_timing=is_checked(h,1025);next.upper_layer_first=is_checked(h,1023);
  std::set<int> nums;int64_t off,minimum;
  if(!next.text&&!next.dialogue&&!next.srtrack){MessageBoxW(h,L"対象オブジェクトを1つ以上選んでください。",L"入力エラー",MB_ICONWARNING);return TRUE;}
  if(!next.all&&(!parse_layers(next.numbers,nums)||(nums.empty()&&split_names(next.names).empty()))){MessageBoxW(h,L"レイヤー番号または名前を指定してください。番号の例: 3, 5, 8-10",L"入力エラー",MB_ICONWARNING);return TRUE;}
  if(!parse_offset(next.offset,off)||!parse_offset(next.minimum_duration,minimum)||minimum<500){MessageBoxW(h,L"秒数は小数点以下3桁までです。最小表示時間は0.500秒以上にしてください。",L"入力エラー",MB_ICONWARNING);return TRUE;}
  next.offset=format_seconds(off);next.minimum_duration=format_seconds(minimum);
  settings=next;if(!save_settings())MessageBoxW(h,L"設定を保存できませんでした。今回の出力は続行できます。",L"設定保存",MB_ICONWARNING);EndDialog(h,IDOK);return TRUE;}
 if((msg==WM_COMMAND&&LOWORD(w)==IDCANCEL)||msg==WM_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}return FALSE;
}
static std::string decode_value(const char* raw){std::string out;for(size_t i=0;raw[i];++i){if(raw[i]=='\\'&&raw[i+1]){char n=raw[i+1];if(n=='n'){out+='\n';++i;continue;}if(n=='r'){++i;continue;}if(n=='\\'){out+='\\';++i;continue;}}out+=raw[i];}return out;}
static bool empty_text(const std::string& s){return s.find_first_not_of(" \t\r\n")==std::string::npos;}
static bool write_file(const wchar_t* path,const std::string& bytes){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;DWORD n=0;bool ok=bytes.size()<=MAXDWORD&&WriteFile(f,bytes.data(),DWORD(bytes.size()),&n,nullptr)&&n==bytes.size();if(!CloseHandle(f))ok=false;return ok;}
static bool read_file(const wchar_t* path,std::string& bytes,size_t max_bytes=16*1024*1024){HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER size{};bool ok=GetFileSizeEx(f,&size)&&size.QuadPart>=0&&static_cast<unsigned long long>(size.QuadPart)<=max_bytes; if(ok){bytes.resize(size_t(size.QuadPart));DWORD n=0;ok=bytes.empty()|| (ReadFile(f,bytes.data(),DWORD(bytes.size()),&n,nullptr)&&n==bytes.size());}CloseHandle(f);return ok;}
static bool utf8_to_wide(const std::string& input,std::wstring& output){if(input.empty()){output.clear();return true;}int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),int(input.size()),nullptr,0);if(!n)return false;output.resize(n);return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),int(input.size()),output.data(),n)==n;}
static bool find_scene_output_file(const std::string& project,int scene_id,std::string& output_file){
 const std::string section="[scene."+std::to_string(scene_id)+"]";bool in_scene=false;size_t pos=0;
 while(pos<=project.size()){size_t end=project.find('\n',pos);if(end==std::string::npos)end=project.size();size_t line_end=end;if(line_end>pos&&project[line_end-1]=='\r')--line_end;std::string line=project.substr(pos,line_end-pos);
  if(!line.empty()&&line.front()=='['&&line.back()==']')in_scene=line==section;
  else if(in_scene&&line.rfind("output.file=",0)==0){output_file=line.substr(12);return !output_file.empty();}
  if(end==project.size())break;pos=end+1;
 }return false;
}
static bool suggested_srt_path(EDIT_SECTION* edit,std::wstring& output){
 if(!edit||!edit->info||!export_edit_handle||!edit->get_project_file)return false;PROJECT_FILE* project=edit->get_project_file(export_edit_handle);if(!project||!project->get_project_file_path)return false;LPCWSTR raw_path=project->get_project_file_path();if(!raw_path||!*raw_path)return false;std::wstring project_path=raw_path;std::string bytes;if(!read_file(project_path.c_str(),bytes,64*1024*1024))return false;std::string saved_output;if(!find_scene_output_file(bytes,edit->info->scene_id,saved_output)||!utf8_to_wide(saved_output,output)||output.empty())return false;
 size_t slash=output.find_last_of(L"\\/");size_t dot=output.find_last_of(L'.');if(dot==std::wstring::npos||(slash!=std::wstring::npos&&dot<slash))output+=L".srt";else output.replace(dot,std::wstring::npos,L".srt");return true;
}
enum class SrtrackObjectKind { none, video_bound, standalone };
struct SrtrackEffect { SrtrackObjectKind kind=SrtrackObjectKind::none; const wchar_t* name=nullptr; };
static SrtrackEffect find_srtrack_effect(EDIT_SECTION* edit,OBJECT_HANDLE object){
 if(edit->count_object_effect(object,L"SRTrackObject(Standalone)")>0)return {SrtrackObjectKind::standalone,L"SRTrackObject(Standalone)"};
 if(edit->count_object_effect(object,L"SRTrackObject")>0)return {SrtrackObjectKind::video_bound,L"SRTrackObject"};
 return {};
}
static bool get_object_item_string(EDIT_SECTION* edit,OBJECT_HANDLE object,const wchar_t* effect,const wchar_t* item,std::string& value,bool decode_escapes=true){
 const char* raw=edit->get_object_item_value(object,effect,item);
 if(!raw)return false;
 // The SDK invalidates raw on the next string-returning API call.  Copy before
 // asking for any other Object value.
 value=decode_escapes?decode_value(raw):raw;
 return true;
}
static bool parse_number(const std::string& input,double& output){char* end=nullptr;output=std::strtod(input.c_str(),&end);return end&&*end=='\0'&&std::isfinite(output);}
static bool parse_check(const std::string& input,bool& output){if(input=="1"||input=="true"){output=true;return true;}if(input=="0"||input=="false"){output=false;return true;}return false;}
static bool parse_first_number(const std::string& input,double& output){const size_t comma=input.find(',');return parse_number(input.substr(0,comma),output);}
struct VideoPlayback { double position=0, speed=0; bool loop=false; };
static bool read_video_playback(EDIT_SECTION* edit,OBJECT_HANDLE object,VideoPlayback& playback){
 std::string range,speed_value,loop_value;
 if(!get_object_item_string(edit,object,L"動画ファイル",L"再生位置",range)||!get_object_item_string(edit,object,L"動画ファイル",L"再生速度",speed_value))return false;
 get_object_item_string(edit,object,L"動画ファイル",L"ループ再生",loop_value);
 if(!loop_value.empty()&&!parse_check(loop_value,playback.loop))return false;
 return parse_first_number(range,playback.position)&&parse_number(speed_value,playback.speed)&&playback.position>=0&&playback.speed>0;
}
static void export_warning(int& warnings,const wchar_t* code,const std::wstring& message,
                           const std::wstring& result,const wchar_t* object_kind,
                           const OBJECT_LAYER_FRAME* location=nullptr){
 ++warnings;
 add_diagnostic(DiagnosticSeverity::warning,L"Exporter",code,message,result,object_kind,
                location?location->layer:-1,location?location->start:-1,location?location->end:-1);
}
static bool read_srt_source(EDIT_SECTION* edit,OBJECT_HANDLE object,const wchar_t* effect,const OBJECT_LAYER_FRAME& location,std::vector<SrtCaption>& parsed,int& warnings){
 std::string file_value;if(!get_object_item_string(edit,object,effect,L"SRT ファイル",file_value,false)){export_warning(warnings,L"EXP-SRT-PATH",L"SRT ファイルの設定を取得できません。",L"この Object を除外しました。",effect,&location);return false;}std::wstring file;if(!utf8_to_wide(file_value,file)||file.empty()){export_warning(warnings,L"EXP-SRT-PATH",L"SRT ファイルのパスが不正または未指定です。",L"この Object を除外しました。",effect,&location);return false;}std::string bytes;if(!read_file(file.c_str(),bytes)){export_warning(warnings,L"EXP-SRT-READ",L"指定された SRT ファイルを読み込めません。",L"この Object を除外しました。",effect,&location);return false;}size_t rejected=0;parsed=parse_srt(bytes,&rejected);if(rejected)export_warning(warnings,L"EXP-SRT-PARSE",L"SRT 内に読み取れない字幕区間があります。",L"読み取れた字幕だけを出力します。",effect,&location);return true;
}
static void append_video_bound_srt(EDIT_SECTION* edit,OBJECT_HANDLE object,const OBJECT_LAYER_FRAME& location,const wchar_t* effect,const std::vector<SrtCaption>& parsed,int64_t source_duration,int rate,int scale,std::vector<SourceTimedCaption>& sources,size_t& order,int& warnings){
 if(location.layer<=0){export_warning(warnings,L"EXP-VIDEO-MISSING",L"上のレイヤーに参照できる動画がありません。",L"この Object を除外しました。",effect,&location);return;}const int video_layer=location.layer-1;int probe=location.start;
 while(probe<=location.end){OBJECT_HANDLE video=edit->find_object(video_layer,probe);if(!video){export_warning(warnings,L"EXP-VIDEO-MISSING",L"上のレイヤーに参照できる動画がありません。",L"動画がない区間を除外しました。",effect,&location);break;}auto video_location=edit->get_object_layer_frame(video);if(video_location.layer!=video_layer||video_location.end<probe){export_warning(warnings,L"EXP-VIDEO-LOCATION",L"参照動画のタイムライン範囲を取得できません。",L"該当区間を除外しました。",effect,&location);break;}probe=video_location.end+1;if(!edit->count_object_effect(video,L"動画ファイル")){export_warning(warnings,L"EXP-VIDEO-MISSING",L"上のレイヤーの Object は動画ではありません。",L"該当区間を除外しました。",effect,&location);continue;
  }VideoPlayback playback;if(!read_video_playback(edit,video,playback)){export_warning(warnings,L"EXP-VIDEO-PLAYBACK",L"参照動画の再生設定を取得できません。",L"該当区間を除外しました。",effect,&location);continue;}const int segment_start=std::max(location.start,video_location.start),segment_end=std::min(location.end+1,video_location.end+1);if(segment_end<=segment_start)continue;
  const int64_t scene_start=frame_milliseconds(segment_start,rate,scale),scene_end=frame_milliseconds(segment_end,rate,scale),video_start=frame_milliseconds(video_location.start,rate,scale);if(playback.loop&&source_duration<=0){export_warning(warnings,L"EXP-VIDEO-DURATION",L"ループ再生に必要な動画全長を取得できません。",L"該当区間を除外しました。",effect,&location);continue;}
  auto mapped=map_video_bound_srt_segment(parsed,scene_start,scene_end,video_start,int64_t(std::llround(playback.position*1000)),playback.speed/100,playback.loop,source_duration);
  for(auto& block:mapped)sources.push_back({block.start,block.end,std::move(block.text),location.layer,order++});
 }
}
static void append_standalone_srt(EDIT_SECTION* edit,OBJECT_HANDLE object,const OBJECT_LAYER_FRAME& location,const wchar_t* effect,const std::vector<SrtCaption>& parsed,int rate,int scale,std::vector<SourceTimedCaption>& sources,size_t& order,int& warnings){
 std::string position_value,speed_value;double position=0,speed=0;if(!get_object_item_string(edit,object,effect,L"再生位置",position_value)||!get_object_item_string(edit,object,effect,L"再生速度",speed_value)||!parse_number(position_value,position)||!parse_number(speed_value,speed)||position<0||speed<=0){export_warning(warnings,L"EXP-STANDALONE-PLAYBACK",L"Standalone Object の再生設定を取得できません。",L"この Object を除外しました。",effect,&location);return;}
 const int64_t scene_start=frame_milliseconds(location.start,rate,scale),scene_end=frame_milliseconds(location.end+1,rate,scale);auto mapped=map_srt_to_scene(parsed,scene_start,scene_end,int64_t(std::llround(position*1000)),speed/100);for(auto& block:mapped)sources.push_back({block.start,block.end,std::move(block.text),location.layer,order++});
}
static void append_text_source(EDIT_SECTION* edit,OBJECT_HANDLE object,const OBJECT_LAYER_FRAME& location,const wchar_t* effect,int layer,std::vector<SourceTimedCaption>& sources,size_t& order){
 std::string raw_text;if(!get_object_item_string(edit,object,effect,L"テキスト",raw_text))return;
 const int64_t start=frame_milliseconds(location.start,edit->info->rate,edit->info->scale),end=frame_milliseconds(location.end+1,edit->info->rate,edit->info->scale);
 if(!settings.reflect_timing){std::string text=sanitize_aviutl_text(raw_text,settings.keep_ruby,settings.keep_emoji);if(!empty_text(text))sources.push_back({start,end,std::move(text),layer,order++});return;}
 // The standard text object exposes this item.  Some external dialogue
 // objects do not; their documented-safe fallback is instantaneous display.
 double initial_speed=0;std::string speed_value;
 if(get_object_item_string(edit,object,effect,L"表示速度",speed_value)){double parsed=0;if(parse_number(speed_value,parsed)&&parsed>=0)initial_speed=parsed;}
 auto blocks=build_timing_controlled_text(raw_text,start,end,initial_speed,settings.keep_ruby,settings.keep_emoji);
 for(auto& block:blocks)if(!empty_text(block.text))sources.push_back({block.start,block.end,std::move(block.text),layer,order++});
}
static void export_subtitles(EDIT_SECTION* edit){
 const size_t diagnostic_start=snapshot_diagnostics().size();
 HWND owner=GetActiveWindow();if(!edit||!edit->info||edit->info->rate<=0||edit->info->scale<=0){add_diagnostic(DiagnosticSeverity::error,L"Exporter",L"EXP-SCENE-INFO",L"シーン情報を取得できません。",L"出力を中止しました。");MessageBoxW(owner,L"シーン情報を取得できません。",L"SRTrack Exporter",MB_ICONERROR);return;}
 if(DialogBoxParamW(module_handle,MAKEINTRESOURCEW(101),owner,dialog,0)!=IDOK)return;
 std::set<int> layers;std::vector<std::wstring> missing;
 if(settings.all){for(int i=0;i<=edit->info->layer_max;++i)layers.insert(i);}
 else {parse_layers(settings.numbers,layers);auto names=split_names(settings.names);std::vector<bool> found(names.size(),false);
  for(int i=0;i<=edit->info->layer_max;++i){LPCWSTR name=edit->get_layer_name(i);if(!name)continue;for(size_t j=0;j<names.size();++j)if(names[j]==name){layers.insert(i);found[j]=true;}}
  for(size_t j=0;j<names.size();++j)if(!found[j])missing.push_back(names[j]);}
 if(!missing.empty()){std::wstring message=L"次のレイヤー名が見つかりませんでした。\n";for(const auto& n:missing)message+=L"・"+n+L"\n";message+=L"\n見つかった対象で続行しますか？";add_diagnostic(DiagnosticSeverity::warning,L"Exporter",L"EXP-LAYER-NOT-FOUND",L"指定されたレイヤー名が見つかりません。",L"見つかった対象だけで続行を確認します。");if(MessageBoxW(owner,message.c_str(),L"SRTrack Exporter",MB_YESNO|MB_ICONWARNING)!=IDYES)return;}
 if(!settings.range_all&&(edit->info->select_range_start<0||edit->info->select_range_end<edit->info->select_range_start)){add_diagnostic(DiagnosticSeverity::error,L"Exporter",L"EXP-RANGE-MISSING",L"フレーム範囲選択が指定されていません。",L"出力を中止しました。");MessageBoxW(owner,L"フレーム範囲選択を指定してください。",L"SRTrack Exporter",MB_ICONWARNING);return;}
 std::vector<SourceTimedCaption> sources;int warnings=0;size_t source_order=0,caption_count=0,srt_count=0;
 for(int layer:layers){int pos=0;while(pos<=edit->info->frame_max){auto obj=edit->find_object(layer,pos);if(!obj)break;auto loc=edit->get_object_layer_frame(obj);if(loc.layer!=layer||loc.end<pos||loc.end==INT_MAX){export_warning(warnings,L"EXP-OBJECT-LOCATION",L"Object のタイムライン範囲を取得できません。",L"このレイヤーの以降の Object を確認できません。",L"",&loc);break;}
  const wchar_t* effect=nullptr;bool isdialogue=edit->count_object_effect(obj,L"セリフ準備@PSDToolKit")>0;const SrtrackEffect srtrack_effect=find_srtrack_effect(edit,obj);bool issrtrack=srtrack_effect.kind!=SrtrackObjectKind::none;
  if(isdialogue){if(settings.dialogue)effect=L"セリフ準備@PSDToolKit";}else if(settings.text&&edit->count_object_effect(obj,L"テキスト")>0)effect=L"テキスト";
  if(effect){const size_t before=sources.size();append_text_source(edit,obj,loc,effect,layer,sources,source_order);if(sources.size()>before)++caption_count;else{std::string raw_text;if(!get_object_item_string(edit,obj,effect,L"テキスト",raw_text))export_warning(warnings,L"EXP-TEXT-READ",L"テキスト内容を取得できません。",L"この Object を除外しました。",effect,&loc);}}
  if(issrtrack&&settings.srtrack){std::vector<SrtCaption> parsed;if(read_srt_source(edit,obj,srtrack_effect.name,loc,parsed,warnings)){if(srtrack_effect.kind==SrtrackObjectKind::standalone)append_standalone_srt(edit,obj,loc,srtrack_effect.name,parsed,edit->info->rate,edit->info->scale,sources,source_order,warnings);else{std::string duration_value;double duration=0;if(get_object_item_string(edit,obj,srtrack_effect.name,L"動画全長",duration_value)&&parse_number(duration_value,duration)&&duration>=0)append_video_bound_srt(edit,obj,loc,srtrack_effect.name,parsed,int64_t(std::llround(duration*1000)),edit->info->rate,edit->info->scale,sources,source_order,warnings);else export_warning(warnings,L"EXP-VIDEO-DURATION",L"動画全長を取得できません。",L"この Object を除外しました。",srtrack_effect.name,&loc);}++srt_count;}}
  pos=loc.end+1;}}
 if(sources.empty()){add_diagnostic(DiagnosticSeverity::warning,L"Exporter",L"EXP-CAPTION-EMPTY",L"字幕を取得できませんでした。",L"出力を中止しました。");MessageBoxW(owner,L"字幕を取得できませんでした。対象レイヤーとオブジェクトを確認してください。",L"SRTrack Exporter",MB_ICONWARNING);return;}
 int64_t offset=0,minimum=1000;parse_offset(settings.offset,offset);parse_offset(settings.minimum_duration,minimum);
 auto blocks=integrate_timeline(sources,settings.upper_layer_first ? SimultaneousOrder::upper_layer_first : SimultaneousOrder::lower_layer_first);if(!settings.range_all)blocks=clip_and_rebase_timeline(blocks,frame_milliseconds(edit->info->select_range_start,edit->info->rate,edit->info->scale),frame_milliseconds(edit->info->select_range_end+1,edit->info->rate,edit->info->scale));blocks=merge_timed_blocks(blocks,minimum);size_t omitted=0;auto shifted=shift_timeline(blocks,offset,&omitted);auto output=render_srt(shifted);
 if(output.empty()){add_diagnostic(DiagnosticSeverity::warning,L"Exporter",L"EXP-OFFSET-EMPTY",L"オフセット適用後の字幕がありません。",L"出力を中止しました。");MessageBoxW(owner,L"オフセットを適用すると全ての字幕が動画開始前になるため、出力できません。",L"SRTrack Exporter",MB_ICONWARNING);return;}
 wchar_t path[32768]=L"subtitles.srt";std::wstring suggested_path;if(suggested_srt_path(edit,suggested_path)&&suggested_path.size()<std::size(path))wcscpy_s(path,suggested_path.c_str());OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=owner;ofn.lpstrFilter=L"SRT 字幕 (*.srt)\0*.srt\0\0";ofn.lpstrFile=path;ofn.nMaxFile=32768;ofn.lpstrDefExt=L"srt";ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetSaveFileNameW(&ofn))return;if(!write_file(path,output)){add_diagnostic(DiagnosticSeverity::error,L"Exporter",L"EXP-FILE-WRITE",L"SRT ファイルを書き込めません。",L"出力を中止しました。");MessageBoxW(owner,L"ファイルの書き込みに失敗しました。",L"SRTrack Exporter",MB_ICONERROR);return;}
 const std::wstring result=L"対象: "+std::to_wstring(caption_count+srt_count)+L" 件、字幕: "+std::to_wstring(shifted.size())+L" 件、開始前のため除外: "+std::to_wstring(omitted)+L" 件、警告: "+std::to_wstring(warnings)+L" 件";
 add_diagnostic(warnings?DiagnosticSeverity::warning:DiagnosticSeverity::info,L"Exporter",L"EXP-EXPORT-COMPLETE",L"SRT の出力が完了しました。",result);
 std::wstring msg=L"出力しました。\n"+result+L"\n\n"+path;
 if(warnings)msg+=L"\n\n診断内容:\n"+format_diagnostics_since(diagnostic_start);
 show_export_result(owner,msg,warnings>0);
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module_handle=instance;initialize_importer(instance);}return TRUE;}
extern "C" __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(){static COMMON_PLUGIN_TABLE t{L"SRTrack",L"SRT 字幕の出力と同名 SRT の取込を支援します"};return &t;}
extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host){INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_TAB_CLASSES};InitCommonControlsEx(&controls);load_settings();export_edit_handle=host->create_edit_handle();host->register_export_menu(L"SRTrack Exporter",export_subtitles);register_importer(host);}
