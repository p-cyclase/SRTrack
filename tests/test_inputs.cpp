#include "../src/aux2/exporter.cpp"
#include <cassert>
#include <cstddef>
int main() {
  auto names=split_names(L"みもり,セリフ\r\nナレーション 字幕\r\n 名前 \r\n\r\n");
  assert(names.size()==3 && names[0]==L"みもり,セリフ" && names[1]==L"ナレーション 字幕" && names[2]==L" 名前 ");
  std::set<int> layers;
  assert(parse_layers(L"3, 5, 8-10",layers) && layers.size()==5 && layers.count(2) && layers.count(9));
  layers.clear();assert(parse_layers(L" ",layers)&&layers.empty());
  assert(!parse_layers(L"3,",layers) && !parse_layers(L"5-3",layers));
  assert(offsetof(EDIT_SECTION,get_layer_name)==27*sizeof(void*));
  HWND window=CreateDialogParamW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(101),nullptr,dialog,0);
  assert(window);
  HWND names_input=exporter_control(window,1012);
  assert(names_input && (GetWindowLongPtrW(names_input,GWL_STYLE)&ES_WANTRETURN));
  assert(exporter_control(window,1026) && exporter_control(window,1027));
  assert(!IsWindowEnabled(exporter_control(window,1027)));
  SendMessageW(exporter_control(window,1026),BM_SETCHECK,BST_CHECKED,0);
  dialog(window,WM_COMMAND,MAKEWPARAM(1026,BN_CLICKED),0);
  assert(IsWindowEnabled(exporter_control(window,1027)));
  HWND next=GetDlgItem(window,1020);
  assert(next && !GetDlgItem(window,IDOK));
  assert(dialog(window,WM_COMMAND,MAKEWPARAM(IDOK,0),0)==TRUE && IsWindow(window));
  MSG enter{};enter.hwnd=next;enter.message=WM_KEYDOWN;enter.wParam=VK_RETURN;
  assert(SendMessageW(next,WM_GETDLGCODE,VK_RETURN,reinterpret_cast<LPARAM>(&enter)) & DLGC_WANTMESSAGE);
  assert(SendMessageW(next,WM_KEYDOWN,VK_RETURN,0)==0 && IsWindow(window));
  DestroyWindow(window);
}
