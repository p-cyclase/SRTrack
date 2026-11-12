#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <set>
#include <string>
#include <vector>
#include "core.h"
#include "../../third_party/aviutl2-sdk/plugin2.h"
#include "importer.h"
#include "monitor.h"

static HINSTANCE module_handle;
struct Settings {
 bool all=true, text=true, dialogue=true, srtrack=true;
 std::wstring numbers=L"", names=L"", offset=L"0", threshold=L"0", final_min=L"1";
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
 settings.text=read_string(k,L"text",L"1")==L"1";
 settings.dialogue=read_string(k,L"dialogue",L"1")==L"1";
 settings.srtrack=read_string(k,L"srtrack",L"1")==L"1";
 settings.numbers=read_string(k,L"numbers",L"");settings.names=read_string(k,L"names",L"");
 settings.offset=read_string(k,L"offset",L"0");settings.threshold=read_string(k,L"threshold",L"0");settings.final_min=read_string(k,L"final_min",L"1");
 RegCloseKey(k);
}
static bool save_settings() {
 HKEY k; if(RegCreateKeyExW(HKEY_CURRENT_USER,registry_key,0,nullptr,0,KEY_SET_VALUE,nullptr,&k,nullptr)!=ERROR_SUCCESS)return false;
 bool ok=true;
 auto put=[&](const wchar_t* n,const std::wstring& v){if(RegSetValueExW(k,n,0,REG_SZ,(const BYTE*)v.c_str(),DWORD((v.size()+1)*sizeof(wchar_t)))!=ERROR_SUCCESS)ok=false;};
 put(L"scope",settings.all?L"all":L"selected");put(L"text",settings.text?L"1":L"0");put(L"dialogue",settings.dialogue?L"1":L"0");put(L"srtrack",settings.srtrack?L"1":L"0");
 put(L"numbers",settings.numbers);put(L"names",settings.names);put(L"offset",settings.offset);put(L"threshold",settings.threshold);put(L"final_min",settings.final_min);
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
static std::wstring get_text(HWND h,int id) {
 HWND c=GetDlgItem(h,id);int n=GetWindowTextLengthW(c);std::vector<wchar_t> b(size_t(n)+1);GetWindowTextW(c,b.data(),n+1);return b.data();
}
static void enable_scope(HWND h){BOOL enabled=IsDlgButtonChecked(h,1011)==BST_CHECKED;EnableWindow(GetDlgItem(h,1001),enabled);EnableWindow(GetDlgItem(h,1012),enabled);}
static WNDPROC original_next_proc=nullptr;
static LRESULT CALLBACK next_button_proc(HWND button,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_GETDLGCODE && l && reinterpret_cast<MSG*>(l)->message==WM_KEYDOWN && w==VK_RETURN)
  return CallWindowProcW(original_next_proc,button,msg,w,l)|DLGC_WANTMESSAGE;
 if((msg==WM_KEYDOWN||msg==WM_KEYUP||msg==WM_CHAR)&&w==VK_RETURN)return 0;
 return CallWindowProcW(original_next_proc,button,msg,w,l);
}
static INT_PTR CALLBACK dialog(HWND h,UINT msg,WPARAM w,LPARAM) {
 if(msg==WM_INITDIALOG){CheckRadioButton(h,1010,1011,settings.all?1010:1011);CheckDlgButton(h,1013,settings.text?BST_CHECKED:BST_UNCHECKED);CheckDlgButton(h,1014,settings.dialogue?BST_CHECKED:BST_UNCHECKED);CheckDlgButton(h,1017,settings.srtrack?BST_CHECKED:BST_UNCHECKED);
  SetDlgItemTextW(h,1001,settings.numbers.c_str());SetDlgItemTextW(h,1012,settings.names.c_str());SetDlgItemTextW(h,1002,settings.offset.c_str());SetDlgItemTextW(h,1015,settings.threshold.c_str());SetDlgItemTextW(h,1016,settings.final_min.c_str());
  original_next_proc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(GetDlgItem(h,1020),GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(next_button_proc)));enable_scope(h);return TRUE;}
 if(msg==WM_COMMAND&&(LOWORD(w)==1010||LOWORD(w)==1011)){enable_scope(h);return TRUE;}
 if(msg==WM_COMMAND&&LOWORD(w)==IDOK)return TRUE;
 if(msg==WM_COMMAND&&LOWORD(w)==1020&&HIWORD(w)==BN_CLICKED){Settings next=settings;next.all=IsDlgButtonChecked(h,1010)==BST_CHECKED;next.text=IsDlgButtonChecked(h,1013)==BST_CHECKED;next.dialogue=IsDlgButtonChecked(h,1014)==BST_CHECKED;
  next.numbers=get_text(h,1001);next.names=get_text(h,1012);next.offset=get_text(h,1002);next.threshold=get_text(h,1015);next.final_min=get_text(h,1016);next.srtrack=IsDlgButtonChecked(h,1017)==BST_CHECKED;
  std::set<int> nums;int64_t off,threshold,minimum;
  if(!next.text&&!next.dialogue&&!next.srtrack){MessageBoxW(h,L"対象オブジェクトを1つ以上選んでください。",L"入力エラー",MB_ICONWARNING);return TRUE;}
  if(!next.all&&(!parse_layers(next.numbers,nums)||(nums.empty()&&split_names(next.names).empty()))){MessageBoxW(h,L"レイヤー番号または名前を指定してください。番号の例: 3, 5, 8-10",L"入力エラー",MB_ICONWARNING);return TRUE;}
  if(!parse_offset(next.offset,off)||!parse_offset(next.threshold,threshold)||threshold<0||!parse_offset(next.final_min,minimum)||minimum<=0){MessageBoxW(h,L"秒数は小数点以下3桁までです。結合閾値は0以上、末尾の最小表示時間は0より大きくしてください。",L"入力エラー",MB_ICONWARNING);return TRUE;}
  settings=next;if(!save_settings())MessageBoxW(h,L"設定を保存できませんでした。今回の出力は続行できます。",L"設定保存",MB_ICONWARNING);EndDialog(h,IDOK);return TRUE;}
 if((msg==WM_COMMAND&&LOWORD(w)==IDCANCEL)||msg==WM_CLOSE){EndDialog(h,IDCANCEL);return TRUE;}return FALSE;
}
static std::string decode_value(const char* raw){std::string out;for(size_t i=0;raw[i];++i){if(raw[i]=='\\'&&raw[i+1]){char n=raw[i+1];if(n=='n'){out+='\n';++i;continue;}if(n=='r'){++i;continue;}if(n=='\\'){out+='\\';++i;continue;}}out+=raw[i];}return out;}
static bool empty_text(const std::string& s){return s.find_first_not_of(" \t\r\n")==std::string::npos;}
static bool write_file(const wchar_t* path,const std::string& bytes){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;DWORD n=0;bool ok=bytes.size()<=MAXDWORD&&WriteFile(f,bytes.data(),DWORD(bytes.size()),&n,nullptr)&&n==bytes.size();if(!CloseHandle(f))ok=false;return ok;}
static bool read_file(const wchar_t* path,std::string& bytes){HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER size{};bool ok=GetFileSizeEx(f,&size)&&size.QuadPart>=0&&size.QuadPart<=16*1024*1024; if(ok){bytes.resize(size_t(size.QuadPart));DWORD n=0;ok=bytes.empty()|| (ReadFile(f,bytes.data(),DWORD(bytes.size()),&n,nullptr)&&n==bytes.size());}CloseHandle(f);return ok;}
static bool utf8_to_wide(const std::string& input,std::wstring& output){if(input.empty()){output.clear();return true;}int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),int(input.size()),nullptr,0);if(!n)return false;output.resize(n);return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),int(input.size()),output.data(),n)==n;}
static bool parse_number(const std::string& input,double& output){char* end=nullptr;output=std::strtod(input.c_str(),&end);return end&&*end=='\0'&&std::isfinite(output);}
static bool parse_check(const std::string& input,bool& output){if(input=="1"||input=="true"){output=true;return true;}if(input=="0"||input=="false"){output=false;return true;}return false;}
static void export_subtitles(EDIT_SECTION* edit){
 HWND owner=GetActiveWindow();if(!edit||!edit->info||edit->info->rate<=0||edit->info->scale<=0){MessageBoxW(owner,L"シーン情報を取得できません。",L"SRTrack Exporter",MB_ICONERROR);return;}
 if(DialogBoxParamW(module_handle,MAKEINTRESOURCEW(101),owner,dialog,0)!=IDOK)return;
 std::set<int> layers;std::vector<std::wstring> missing;
 if(settings.all){for(int i=0;i<=edit->info->layer_max;++i)layers.insert(i);}
 else {parse_layers(settings.numbers,layers);auto names=split_names(settings.names);std::vector<bool> found(names.size(),false);
  for(int i=0;i<=edit->info->layer_max;++i){LPCWSTR name=edit->get_layer_name(i);if(!name)continue;for(size_t j=0;j<names.size();++j)if(names[j]==name){layers.insert(i);found[j]=true;}}
  for(size_t j=0;j<names.size();++j)if(!found[j])missing.push_back(names[j]);}
 if(!missing.empty()){std::wstring message=L"次のレイヤー名が見つかりませんでした。\n";for(const auto& n:missing)message+=L"・"+n+L"\n";message+=L"\n見つかった対象で続行しますか？";if(MessageBoxW(owner,message.c_str(),L"SRTrack Exporter",MB_YESNO|MB_ICONWARNING)!=IDYES)return;}
 std::vector<Caption> captions;std::vector<TimedBlock> srt_blocks;int warnings=0;
 for(int layer:layers){int pos=0;while(pos<=edit->info->frame_max){auto obj=edit->find_object(layer,pos);if(!obj)break;auto loc=edit->get_object_layer_frame(obj);if(loc.layer!=layer||loc.end<pos||loc.end==INT_MAX){++warnings;break;}
  const wchar_t* effect=nullptr;bool isdialogue=edit->count_object_effect(obj,L"セリフ準備@PSDToolKit")>0;const wchar_t* srtrack_effect=edit->count_object_effect(obj,L"SRTrack")>0?L"SRTrack":(edit->count_object_effect(obj,L"SRTrack Object")>0?L"SRTrack Object":nullptr);bool issrtrack=srtrack_effect!=nullptr;
  if(isdialogue){if(settings.dialogue)effect=L"セリフ準備@PSDToolKit";}else if(settings.text&&edit->count_object_effect(obj,L"テキスト")>0)effect=L"テキスト";
  if(effect){const char* raw=edit->get_object_item_value(obj,effect,L"テキスト");if(!raw)++warnings;else{std::string text=decode_value(raw);if(text.find('<')!=std::string::npos||text.find("\n\n")!=std::string::npos)++warnings;if(!empty_text(text))captions.push_back({layer,loc.start,loc.end+1,text});}}
  if(issrtrack&&settings.srtrack){const char* file_raw=edit->get_object_item_value(obj,srtrack_effect,L"SRT ファイル");const char* position_raw=edit->get_object_item_value(obj,srtrack_effect,L"再生位置");const char* end_raw=edit->get_object_item_value(obj,srtrack_effect,L"再生範囲終端");const char* duration_raw=edit->get_object_item_value(obj,srtrack_effect,L"動画全長");const char* speed_raw=edit->get_object_item_value(obj,srtrack_effect,L"再生速度");const char* loop_raw=edit->get_object_item_value(obj,srtrack_effect,L"ループ再生");std::wstring file;double position=0,end_position=0,source_duration=0,speed_percent=0;bool loop=false;
   if(!file_raw||!position_raw||!end_raw||!duration_raw||!speed_raw||!loop_raw||!utf8_to_wide(decode_value(file_raw),file)||file.empty()||!parse_number(decode_value(position_raw),position)||!parse_number(decode_value(end_raw),end_position)||!parse_number(decode_value(duration_raw),source_duration)||!parse_number(decode_value(speed_raw),speed_percent)||!parse_check(decode_value(loop_raw),loop)||position<0||end_position<=position||speed_percent<=0||(loop&&source_duration<=0)){++warnings;}
   else {std::string bytes;if(!read_file(file.c_str(),bytes)){++warnings;}else{size_t rejected=0;auto parsed=parse_srt(bytes,&rejected);if(rejected)++warnings;const int64_t scene_start=frame_milliseconds(loc.start,edit->info->rate,edit->info->scale);const int64_t scene_end=frame_milliseconds(loc.end+1,edit->info->rate,edit->info->scale);std::vector<TimedBlock> mapped;if(loop)mapped=map_looped_srt_to_scene(parsed,scene_start,scene_end,int64_t(std::llround(position*1000)),int64_t(std::llround(source_duration*1000)),speed_percent/100);else{const int64_t range_end=scene_start+int64_t(std::llround((end_position-position)*1000/(speed_percent/100)));mapped=map_srt_to_scene(parsed,scene_start,std::min(scene_end,range_end),int64_t(std::llround(position*1000)),speed_percent/100);}srt_blocks.insert(srt_blocks.end(),mapped.begin(),mapped.end());}}}
  pos=loc.end+1;}}
 if(captions.empty()&&srt_blocks.empty()){MessageBoxW(owner,L"字幕を取得できませんでした。対象レイヤーとオブジェクトを確認してください。",L"SRTrack Exporter",MB_ICONWARNING);return;}
 int64_t offset=0,threshold=0,minimum=1000;parse_offset(settings.offset,offset);parse_offset(settings.threshold,threshold);parse_offset(settings.final_min,minimum);
 std::stable_sort(captions.begin(),captions.end(),[](const Caption&a,const Caption&b){return a.start!=b.start?a.start<b.start:a.layer<b.layer;});std::vector<SourceTimedCaption> sources;for(const auto& c:captions)sources.push_back({frame_milliseconds(c.start,edit->info->rate,edit->info->scale),frame_milliseconds(c.end,edit->info->rate,edit->info->scale),c.text,false});for(const auto& b:srt_blocks)sources.push_back({b.start,b.end,b.text,true});auto blocks=merge_timed_blocks(integrate_timeline(sources),threshold,minimum);size_t omitted=0;auto shifted=shift_timeline(blocks,offset,&omitted);auto output=render_srt(shifted);
 if(output.empty()){MessageBoxW(owner,L"オフセットを適用すると全ての字幕が動画開始前になるため、出力できません。",L"SRTrack Exporter",MB_ICONWARNING);return;}
 if(warnings&&MessageBoxW(owner,L"取得できない項目や制御文字等を検出しました。出力内容の確認が必要です。続けますか？",L"SRTrack Exporter",MB_YESNO|MB_ICONWARNING)!=IDYES)return;
 wchar_t path[32768]=L"subtitles.srt";OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=owner;ofn.lpstrFilter=L"SRT 字幕 (*.srt)\0*.srt\0\0";ofn.lpstrFile=path;ofn.nMaxFile=32768;ofn.lpstrDefExt=L"srt";ofn.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetSaveFileNameW(&ofn))return;if(!write_file(path,output)){MessageBoxW(owner,L"ファイルの書き込みに失敗しました。",L"SRTrack Exporter",MB_ICONERROR);return;}
 std::wstring msg=L"出力しました。\n対象: "+std::to_wstring(captions.size())+L" 件\n字幕: "+std::to_wstring(shifted.size())+L" 件\n開始前のため除外: "+std::to_wstring(omitted)+L" 件\n警告: "+std::to_wstring(warnings)+L" 件\n\n"+path;MessageBoxW(owner,msg.c_str(),L"SRTrack Exporter",MB_OK);
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module_handle=instance;initialize_importer(instance);initialize_monitor(instance);}return TRUE;}
extern "C" __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(){static COMMON_PLUGIN_TABLE t{L"SRTrack",L"SRT 字幕の出力と同名 SRT の取込を支援します"};return &t;}
extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host){load_settings();host->register_export_menu(L"SRTrack Exporter",export_subtitles);register_importer();register_monitor(host);}
