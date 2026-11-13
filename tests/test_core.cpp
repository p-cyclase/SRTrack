#include "../src/aux2/core.h"
#include <cassert>

int main() {
  int64_t offset = 0;
  assert(parse_offset(L"5.250", offset) && offset == 5250);
  assert(parse_offset(L" -0.5 ", offset) && offset == -500);
  assert(!parse_offset(L"1.2345", offset));

  size_t rejected = 0;
  auto source = parse_srt(
      "\xEF\xBB\xBF" "1\r\n00:00:01,000 --> 00:00:03,000\r\n第一行\r\n第二行\r\n\r\n"
      "2\r\n00:00:05,000 --> 00:00:06,000\r\n次\r\n", &rejected);
  assert(rejected == 0 && source.size() == 2 && source[0].text == "第一行\n第二行");
  assert(parse_srt("1\n00:00:00,000 --> 00:00:00,000\n不正\n", &rejected).empty() && rejected == 1);

  auto mapped = map_srt_to_scene(source, 10000, 14000, 1000, 2.0);
  assert(mapped.size() == 2 && mapped[0].start == 10000 && mapped[0].end == 11000);
  assert(mapped[1].start == 12000 && mapped[1].end == 12500);

  auto video_bound = map_video_bound_srt_segment({{5000, 7000, "途中", 0}}, 5000, 8000,
                                                  0, 0, 1.0, false, 0);
  assert(video_bound.size() == 1 && video_bound[0].start == 5000 && video_bound[0].end == 7000);
  auto looped = map_looped_srt_to_scene({{1000, 2000, "A", 0}, {4000, 5000, "B", 1}},
                                        10000, 22000, 4000, 5000, 1.0);
  assert(looped.size() == 5 && looped[0].text == "B" && looped[4].end == 21000);

  // A caption's own end creates a gap; it is never extended merely because a
  // later caption exists.
  auto interrupted = integrate_timeline({
      {0, 5000, "A", 0, 0}, {2000, 3000, "X", 1, 0}, {5000, 8000, "B", 0, 1}});
  assert(interrupted.size() == 3);
  assert(interrupted[0].start == 0 && interrupted[0].end == 2000 && interrupted[0].text == "A");
  assert(interrupted[1].start == 2000 && interrupted[1].end == 3000 && interrupted[1].text == "X");
  assert(interrupted[2].start == 5000 && interrupted[2].end == 8000 && interrupted[2].text == "B");

  // Equal-start events are processed from Layer 1 downward and all zero-length
  // intermediate candidates remain until the short-caption pass.
  auto simultaneous = integrate_timeline({
      {0, 1000, "d", 3, 0}, {0, 1000, "b", 1, 0},
      {0, 1000, "a", 0, 0}, {0, 1000, "c", 2, 0}});
  assert(simultaneous.size() == 4);
  assert(simultaneous[0].end == 0 && simultaneous[0].text == "a");
  assert(simultaneous[1].end == 0 && simultaneous[1].text == "b");
  assert(simultaneous[2].end == 0 && simultaneous[2].text == "c");
  assert(simultaneous[3].end == 1000 && simultaneous[3].text == "d");
  auto joined = merge_timed_blocks(simultaneous, 1, 1000);
  assert(joined.size() == 1 && joined[0].start == 0 && joined[0].end == 1000 && joined[0].text == "abcd");

  auto ordinary_short = merge_timed_blocks({{0, 100, "A"}, {300, 1000, "B"}}, 200, 1000);
  assert(ordinary_short.size() == 1 && ordinary_short[0].start == 0 && ordinary_short[0].end == 1000 && ordinary_short[0].text == "AB");
  auto final_short = merge_timed_blocks({{0, 50, "A"}}, 200, 1000);
  assert(final_short.size() == 1 && final_short[0].end == 1000);

  auto clipped = clip_and_rebase_timeline({{1000, 1000, "Z"}, {1000, 4000, "A"}, {5000, 7000, "B"}},
                                           1000, 6000);
  assert(clipped.size() == 3 && clipped[0].start == 0 && clipped[0].end == 0);
  assert(clipped[1].start == 0 && clipped[1].end == 3000);
  assert(clipped[2].start == 4000 && clipped[2].end == 5000);

  size_t omitted = 0;
  auto shifted = shift_timeline({{0, 1000, "A"}}, -500, &omitted);
  assert(omitted == 0 && shifted.size() == 1 && shifted[0].start == 0 && shifted[0].end == 500);
  assert(render_srt(shifted).find("00:00:00,000 --> 00:00:00,500") != std::string::npos);
}
