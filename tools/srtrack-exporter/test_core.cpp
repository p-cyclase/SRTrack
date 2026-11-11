#include "core.h"
#include <cassert>
int main() {
  auto timeline=build_timeline({{1,0,300,"章"},{2,60,90,"こんにちは"},{3,150,180,"次"}},30,1,0,1000);
  assert(timeline.size()==3 && timeline[0].text=="章" && timeline[0].end==2000);
  assert(timeline[1].start==2000 && timeline[1].end==5000 && timeline[2].end==6000);
  auto simultaneous=build_timeline({{4,0,90,"B"},{2,0,150,"A"},{1,90,120,"C"}},30,1,0,1000);
  assert(simultaneous.size()==2 && simultaneous[0].text=="A\nB" && simultaneous[0].end==3000);
  auto chain=build_timeline({{0,0,1,"A"},{0,3,4,"B"},{0,6,9,"C"},{0,60,61,"D"}},30,1,200,1000);
  assert(chain.size()==2 && chain[0].text=="A\nB\nC" && chain[0].end==2000);
  assert(chain[1].start==2000 && chain[1].end==3000);
  size_t skip=0;auto shifted=shift_timeline(chain,-2500,&skip);
  assert(skip==1 && shifted.size()==1 && shifted[0].start==0 && shifted[0].end==500);
  assert(render_srt(shifted).find("00:00:00,000 --> 00:00:00,500")!=std::string::npos);
  int64_t offset=0;
  assert(parse_offset(L"5.250",offset) && offset==5250);
  assert(parse_offset(L" -0.5 ",offset) && offset==-500);
  assert(parse_offset(L"0",offset) && offset==0);
  assert(!parse_offset(L"1.2345",offset) && !parse_offset(L"1,2",offset));
  assert(!parse_offset(L"1000000",offset) && !parse_offset(L"-",offset));
  auto b = merge_captions({{2,30,120,"A"},{4,90,150,"B"}});
  assert(b.size()==3 && b[0].start==30 && b[0].end==90 && b[0].text=="A");
  assert(b[1].start==90 && b[1].end==120 && b[1].text=="A\nB");
  assert(b[2].start==120 && b[2].end==150 && b[2].text=="B");
  assert(timestamp(107892,30000,1001)=="00:59:59,996");
  auto c=merge_captions({{1,0,1,"日本語"},{1,1,2,"日本語"}});
  assert(c.size()==1 && c[0].end==2);
  assert(srt(c,30,1).find("00:00:00,000 --> 00:00:00,067")!=std::string::npos);
  size_t written=0, omitted=0;
  auto delayed=srt(b,30,1,5250,&written,&omitted);
  assert(written==3 && omitted==0);
  assert(delayed.find("00:00:06,250 --> 00:00:08,250")!=std::string::npos);
  auto advanced=srt(b,30,1,-3500,&written,&omitted);
  assert(written==2 && omitted==1);
  assert(advanced.find("1\r\n00:00:00,000 --> 00:00:00,500")!=std::string::npos);
  assert(srt(b,30,1,-6000,&written,&omitted).empty() && written==0 && omitted==3);
}
