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
  // Both video-bound and fix Objects use the same loop mapper.  The only
  // difference is the timeline start supplied to the transformation.
  auto fixed_looped = map_video_bound_srt_segment(
      {{1000, 2000, "A", 0}, {4000, 5000, "B", 1}}, 10000, 22000,
      10000, 4000, 1.0, true, 5000);
  assert(fixed_looped.size() == looped.size() && fixed_looped[0].text == "B" && fixed_looped[4].end == 21000);
  // A loop folds source time into [0, D): captions beyond D do not leak into
  // the next cycle, and captions crossing D end at the cycle boundary.
  auto clipped_loop = map_looped_srt_to_scene(
      {{0, 3000, "A", 0}, {3000, 6000, "B", 1}, {6000, 9000, "C", 2}},
      0, 15000, 0, 5000, 1.0);
  assert(clipped_loop.size() == 6 && clipped_loop[0].text == "A" && clipped_loop[1].text == "B");
  assert(clipped_loop[1].end == 5000 && clipped_loop[2].start == 5000 && clipped_loop[2].text == "A");
  assert(clipped_loop[5].end == 15000 && clipped_loop[5].text == "B");

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
  auto joined = merge_timed_blocks(simultaneous, 500);
  assert(joined.size() == 1 && joined[0].start == 0 && joined[0].end == 1000 && joined[0].text == "abcd");
  auto reverse_simultaneous = integrate_timeline({
      {0, 1000, "d", 3, 0}, {0, 1000, "b", 1, 0},
      {0, 1000, "a", 0, 0}, {0, 1000, "c", 2, 0}},
      SimultaneousOrder::lower_layer_first);
  auto reverse_joined = merge_timed_blocks(reverse_simultaneous, 500);
  assert(reverse_joined.size() == 1 && reverse_joined[0].text == "dcba");

  auto ordinary_short = merge_timed_blocks({{0, 100, "A"}, {300, 1000, "B"}}, 500);
  assert(ordinary_short.size() == 1 && ordinary_short[0].start == 0 && ordinary_short[0].end == 1000 && ordinary_short[0].text == "AB");
  auto independently_extended = merge_timed_blocks({{0, 100, "A"}, {500, 1000, "B"}}, 500);
  assert(independently_extended.size() == 2);
  assert(independently_extended[0].start == 0 && independently_extended[0].end == 500 && independently_extended[0].text == "A");
  assert(independently_extended[1].start == 500 && independently_extended[1].end == 1000 && independently_extended[1].text == "B");
  auto separated_short = merge_timed_blocks({{0, 100, "A"}, {700, 800, "B"}}, 500);
  assert(separated_short.size() == 2);
  assert(separated_short[0].start == 0 && separated_short[0].end == 500 && separated_short[0].text == "A");
  assert(separated_short[1].start == 700 && separated_short[1].end == 1200 && separated_short[1].text == "B");
  auto cascading_short = merge_timed_blocks({{0, 100, "A"}, {300, 350, "B"}, {450, 1000, "C"}}, 500);
  assert(cascading_short.size() == 1 && cascading_short[0].start == 0 && cascading_short[0].end == 1000 && cascading_short[0].text == "ABC");
  auto final_short = merge_timed_blocks({{0, 50, "A"}}, 500);
  assert(final_short.size() == 1 && final_short[0].end == 500);

  auto clipped = clip_and_rebase_timeline({{1000, 1000, "Z"}, {1000, 4000, "A"}, {5000, 7000, "B"}},
                                           1000, 6000);
  assert(clipped.size() == 3 && clipped[0].start == 0 && clipped[0].end == 0);
  assert(clipped[1].start == 0 && clipped[1].end == 3000);
  assert(clipped[2].start == 4000 && clipped[2].end == 5000);

  size_t omitted = 0;
  auto shifted = shift_timeline({{0, 1000, "A"}}, -500, &omitted);
  assert(omitted == 0 && shifted.size() == 1 && shifted[0].start == 0 && shifted[0].end == 500);
  assert(render_srt(shifted).find("00:00:00,000 --> 00:00:00,500") != std::string::npos);

  assert(sanitize_aviutl_text("A<#ffffff><s32,Meiryo,3,2><@Meiryo,3><p20,40><gw8><tw0.8>B", true, true) == "AB");
  assert(sanitize_aviutl_text("</>漢字<!>かんじ</> <&拍手>", true, true) == "漢字（かんじ） （拍手）");
  assert(sanitize_aviutl_text("</>漢字<!>かんじ</> <&拍手>", false, false) == "漢字 ");
  assert(sanitize_aviutl_text("A<r10>B", true, true) == "AB");
  assert(sanitize_aviutl_text("A<w0.5>B", true, true) == "AB");
  assert(sanitize_aviutl_text("A<c>B", true, true) == "AB");
  assert(sanitize_aviutl_text("A<//note//>B", true, true) == "AB");
  assert(sanitize_aviutl_text("A<?return 'x'?>B", true, true) == "AB");
  assert(sanitize_aviutl_text("A<r10><w0.5><c>B<//note//><?return 'x'?>C", true, true) == "ABC");
  assert(sanitize_aviutl_text("A<not-a-control>B", true, true) == "A<not-a-control>B");

  // Timing controls advance one shared cursor but never create a cue per
  // displayed character.  Wait keeps the already visible body on screen;
  // clear creates a new subtitle boundary.
  auto timed = build_timing_controlled_text("<r10>AB<c0.5>CD", 0, 10000, 0, true, true);
  assert(timed.size() == 2 && timed[0].start == 0 && timed[0].end == 700 && timed[0].text == "AB");
  assert(timed[1].start == 700 && timed[1].end == 10000 && timed[1].text == "CD");
  auto multiplied_clear = build_timing_controlled_text("<r10>AB<c*0.2>CD", 0, 10000, 0, true, true);
  assert(multiplied_clear.size() == 2 && multiplied_clear[0].end == 600 && multiplied_clear[1].start == 600);
  auto waited = build_timing_controlled_text("<r10>A<w1>B", 0, 10000, 0, true, true);
  assert(waited.size() == 2 && waited[0].end == 1100 && waited[0].text == "A");
  assert(waited[1].start == 1100 && waited[1].text == "AB");
  auto clear_to_empty = build_timing_controlled_text("<r10>AB<c>", 0, 10000, 0, true, true);
  assert(clear_to_empty.size() == 1 && clear_to_empty[0].end == 200);
  auto ignored_newline = build_timing_controlled_text("<r10>AB<c>\r\nCD", 0, 10000, 0, true, true);
  assert(ignored_newline.size() == 2 && ignored_newline[1].start == 200 && ignored_newline[1].text == "CD");
}
