#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cwctype>
#include <set>
#include <sstream>
#include <string>
#include <vector>

struct Caption { int layer; int start; int end; std::string text; }; // end exclusive
struct Block { int start; int end; std::string text; };
struct TimedBlock { int64_t start, end; std::string text; };
struct SrtCaption { int64_t start, end; std::string text; size_t order; };
enum class SimultaneousOrder { upper_layer_first, lower_layer_first };
struct SourceTimedCaption {
    int64_t start, end;
    std::string text;
    // Smaller layer numbers are visually higher in AviUtl2.  Equal-start
    // events are processed from Layer 1 downward; this is not winner-takes-all.
    int layer = 0;
    size_t order = 0;
};

inline bool parse_srt_timestamp(const std::string& value, int64_t& result) {
    size_t p = 0;
    while (p < value.size() && (value[p] == ' ' || value[p] == '\t')) ++p;
    auto number = [&](size_t digits, int64_t& out) {
        if (p + digits > value.size()) return false;
        out = 0;
        for (size_t i = 0; i < digits; ++i) {
            if (value[p + i] < '0' || value[p + i] > '9') return false;
            out = out * 10 + value[p + i] - '0';
        }
        p += digits;
        return true;
    };
    int64_t h = 0, m = 0, s = 0, ms = 0;
    size_t hour_start = p;
    while (p < value.size() && value[p] >= '0' && value[p] <= '9') ++p;
    if (p == hour_start || p >= value.size() || value[p++] != ':') return false;
    for (size_t i = hour_start; i < p - 1; ++i) h = h * 10 + value[i] - '0';
    if (!number(2, m) || p >= value.size() || value[p++] != ':' || !number(2, s) || p >= value.size() || (value[p] != ',' && value[p] != '.')) return false;
    ++p;
    if (!number(3, ms) || m > 59 || s > 59) return false;
    while (p < value.size() && (value[p] == ' ' || value[p] == '\t')) ++p;
    if (p != value.size()) return false;
    result = ((h * 60 + m) * 60 + s) * 1000 + ms;
    return true;
}

inline std::vector<SrtCaption> parse_srt(const std::string& input, size_t* rejected = nullptr) {
    std::string data = input;
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xef && static_cast<unsigned char>(data[1]) == 0xbb && static_cast<unsigned char>(data[2]) == 0xbf) data.erase(0, 3);
    std::string normalized;
    normalized.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i] == '\r') { if (i + 1 < data.size() && data[i + 1] == '\n') ++i; normalized += '\n'; }
        else normalized += data[i];
    }
    std::vector<SrtCaption> out;
    size_t bad = 0, block_start = 0;
    auto parse_block = [&](const std::string& block) {
        if (block.empty()) return;
        std::vector<std::string> lines;
        size_t start = 0;
        while (start <= block.size()) {
            size_t end = block.find('\n', start);
            lines.push_back(block.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (end == std::string::npos) break;
            start = end + 1;
        }
        size_t time_line = !lines.empty() && lines[0].find_first_not_of("0123456789 \t") == std::string::npos ? 1 : 0;
        if (time_line >= lines.size()) { ++bad; return; }
        size_t arrow = lines[time_line].find("-->");
        if (arrow == std::string::npos) { ++bad; return; }
        int64_t begin = 0, end = 0;
        if (!parse_srt_timestamp(lines[time_line].substr(0, arrow), begin) || !parse_srt_timestamp(lines[time_line].substr(arrow + 3), end) || end <= begin) { ++bad; return; }
        std::string text;
        for (size_t i = time_line + 1; i < lines.size(); ++i) { if (i > time_line + 1) text += '\n'; text += lines[i]; }
        if (text.empty()) { ++bad; return; }
        out.push_back({begin, end, text, out.size()});
    };
    while (block_start < normalized.size()) {
        size_t block_end = normalized.find("\n\n", block_start);
        if (block_end == std::string::npos) { parse_block(normalized.substr(block_start)); break; }
        parse_block(normalized.substr(block_start, block_end - block_start));
        block_start = block_end + 2;
        while (block_start < normalized.size() && normalized[block_start] == '\n') ++block_start;
    }
    if (rejected) *rejected = bad;
    return out;
}

inline std::vector<TimedBlock> map_srt_to_scene(const std::vector<SrtCaption>& captions, int64_t object_start,
                                                 int64_t object_end, int64_t source_start, double speed) {
    std::vector<TimedBlock> out;
    if (object_end <= object_start || speed <= 0) return out;
    const double source_end = source_start + speed * (object_end - object_start);
    for (const auto& caption : captions) {
        double left = std::max<double>(caption.start, source_start);
        double right = std::min<double>(caption.end, source_end);
        if (right <= left) continue;
        int64_t start = object_start + static_cast<int64_t>((left - source_start) / speed + 0.5);
        int64_t end = object_start + static_cast<int64_t>((right - source_start) / speed + 0.5);
        start = std::max(start, object_start); end = std::min(end, object_end);
        if (end > start) out.push_back({start, end, caption.text});
    }
    return out;
}

// Map a looping source whose original video duration is the repeat period.
// The source starts at source_start, advances at speed, then wraps to zero
// whenever it reaches source_duration.  The current non-looping range end is
// intentionally not used: D is the loop period retained by SRTrack Object.
inline std::vector<TimedBlock> map_looped_srt_to_scene(const std::vector<SrtCaption>& captions,
                                                        int64_t object_start, int64_t object_end,
                                                        int64_t source_start, int64_t source_duration,
                                                        double speed) {
    std::vector<TimedBlock> out;
    if (object_end <= object_start || source_duration <= 0 || speed <= 0) return out;
    const int64_t source_end = source_start + static_cast<int64_t>(std::llround(speed * (object_end - object_start)));
    for (const auto& caption : captions) {
        int64_t cycle = (source_start - caption.end) / source_duration;
        if (source_start - caption.end < 0 && (source_start - caption.end) % source_duration) --cycle;
        for (;; ++cycle) {
            const int64_t left_source = caption.start + cycle * source_duration;
            const int64_t right_source = caption.end + cycle * source_duration;
            if (left_source >= source_end) break;
            const int64_t left = std::max(left_source, source_start);
            const int64_t right = std::min(right_source, source_end);
            if (right <= left) continue;
            int64_t start = object_start + static_cast<int64_t>(std::llround((left - source_start) / speed));
            int64_t end = object_start + static_cast<int64_t>(std::llround((right - source_start) / speed));
            start = std::max(start, object_start);
            end = std::min(end, object_end);
            if (end > start) out.push_back({start, end, caption.text});
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const TimedBlock& left, const TimedBlock& right) {
        return left.start < right.start;
    });
    return out;
}

// Map one intersection of an SRTrack Object and the video directly above it.
// The source time is deliberately based on the video's timeline start, not
// the SRTrack Object's start, matching the Preview calculation.
inline std::vector<TimedBlock> map_video_bound_srt_segment(
    const std::vector<SrtCaption>& captions, int64_t scene_start, int64_t scene_end,
    int64_t video_start, int64_t playback_start,
    double speed, bool loop, int64_t source_duration) {
    if (scene_end <= scene_start || speed <= 0) return {};
    const int64_t source_start = playback_start +
        static_cast<int64_t>(std::llround(speed * (scene_start - video_start)));
    if (loop) {
        return map_looped_srt_to_scene(captions, scene_start, scene_end,
                                       source_start, source_duration, speed);
    }
    return map_srt_to_scene(captions, scene_start, scene_end, source_start, speed);
}

inline std::vector<TimedBlock> integrate_timeline(
    std::vector<SourceTimedCaption> captions,
    SimultaneousOrder simultaneous_order = SimultaneousOrder::upper_layer_first) {
    std::stable_sort(captions.begin(), captions.end(), [simultaneous_order](const SourceTimedCaption& a, const SourceTimedCaption& b) {
        if (a.start != b.start) return a.start < b.start;
        if (a.layer != b.layer) return simultaneous_order == SimultaneousOrder::upper_layer_first
            ? a.layer < b.layer : a.layer > b.layer;
        return a.order < b.order;
    });
    std::vector<TimedBlock> out;
    bool active = false;
    SourceTimedCaption current{};
    for (const auto& next : captions) {
        if (next.end <= next.start || next.text.empty()) continue;
        if (active) {
            // A later start interrupts the active caption, while the active
            // caption's own end still creates a blank interval when earlier.
            const int64_t end = std::min(current.end, next.start);
            // Equal-start captions are intentionally retained as zero-length
            // blocks.  The short-caption pass decides how to combine them.
            if (end >= current.start) out.push_back({current.start, end, current.text});
        }
        current = next;
        active = true;
    }
    if (active && current.end > current.start) out.push_back({current.start, current.end, current.text});
    return out;
}

inline bool is_control_number(const std::string& value, bool allow_multiplier = false) {
    if (value.empty()) return true;
    size_t index = 0;
    if (allow_multiplier && value[index] == '*') ++index;
    if (index < value.size() && (value[index] == '+' || value[index] == '-')) ++index;
    bool digit = false;
    bool decimal = false;
    for (; index < value.size(); ++index) {
        const char c = value[index];
        if (c >= '0' && c <= '9') { digit = true; continue; }
        if (c == '.' && !decimal) { decimal = true; continue; }
        return false;
    }
    return digit;
}

inline bool is_control_numbers(const std::string& value) {
    if (value.empty()) return true;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t end = value.find(',', start);
        if (!is_control_number(value.substr(start, end == std::string::npos ? std::string::npos : end - start), true)) return false;
        if (end == std::string::npos) return true;
        start = end + 1;
    }
    return false;
}

inline bool is_aviutl_text_control(const std::string& token) {
    if (token == "/") return true; // Text block separator.
    if (token.empty()) return false;
    const std::string payload = token.substr(1);
    switch (token[0]) {
    case '#': return true;
    case '@': case '$': return true;
    case 's': {
        const size_t comma = payload.find(',');
        return is_control_number(payload.substr(0, comma), true);
    }
    case 'r': case 'w': case 'c': return is_control_number(payload, true);
    case 'p': return is_control_numbers(payload);
    case 'g': return token.size() >= 3 && (token[1] == 'w' || token[1] == 'h') && is_control_number(token.substr(2), true);
    case 't': return token.size() >= 3 && (token[1] == 'w' || token[1] == 'h' || token[1] == 'r') && is_control_number(token.substr(2), true);
    default: return false;
    }
}

// Converts only documented AviUtl2 text controls.  It deliberately leaves
// unknown angle-bracketed text intact so ordinary prose is not lost.
inline std::string sanitize_aviutl_text(const std::string& input, bool keep_ruby, bool keep_emoji) {
    std::string output;
    output.reserve(input.size());
    for (size_t index = 0; index < input.size();) {
        if (input.compare(index, 3, "<//") == 0) {
            const size_t end = input.find("//>", index + 3);
            if (end != std::string::npos) { index = end + 3; continue; }
        }
        if (input.compare(index, 2, "<?") == 0) {
            const size_t end = input.find("?>", index + 2);
            if (end != std::string::npos) { index = end + 2; continue; }
        }
        if (input.compare(index, 3, "</>") == 0) {
            const size_t ruby_marker = input.find("<!", index + 3);
            if (ruby_marker != std::string::npos) {
                const size_t ruby_header_end = input.find('>', ruby_marker + 2);
                if (ruby_header_end != std::string::npos) {
                    const size_t ruby_end = input.find("</>", ruby_header_end + 1);
                    if (ruby_end != std::string::npos) {
                        const std::string base = sanitize_aviutl_text(input.substr(index + 3, ruby_marker - (index + 3)), keep_ruby, keep_emoji);
                        const std::string reading = sanitize_aviutl_text(input.substr(ruby_header_end + 1, ruby_end - (ruby_header_end + 1)), keep_ruby, keep_emoji);
                        output += base;
                        if (keep_ruby && !reading.empty()) output += "（" + reading + "）";
                        index = ruby_end + 3;
                        continue;
                    }
                }
            }
            index += 3;
            continue;
        }
        if (input[index] == '<') {
            const size_t end = input.find('>', index + 1);
            if (end != std::string::npos) {
                const std::string token = input.substr(index + 1, end - (index + 1));
                if (!token.empty() && token[0] == '&') {
                    if (keep_emoji && token.size() > 1) output += "（" + token.substr(1) + "）";
                    index = end + 1;
                    continue;
                }
                if (is_aviutl_text_control(token)) {
                    index = end + 1;
                    continue;
                }
            }
        }
        output += input[index++];
    }
    return output;
}

inline std::vector<TimedBlock> clip_and_rebase_timeline(
    const std::vector<TimedBlock>& blocks, int64_t range_start, int64_t range_end) {
    std::vector<TimedBlock> out;
    if (range_end <= range_start) return out;
    for (const auto& block : blocks) {
        if (block.end == block.start) {
            if (block.start >= range_start && block.start < range_end)
                out.push_back({block.start - range_start, block.end - range_start, block.text});
            continue;
        }
        const int64_t start = std::max(block.start, range_start);
        const int64_t end = std::min(block.end, range_end);
        if (end > start) out.push_back({start - range_start, end - range_start, block.text});
    }
    return out;
}

inline std::vector<TimedBlock> merge_timed_blocks(const std::vector<TimedBlock>& raw,
                                                   int64_t minimum_duration_ms) {
    std::vector<TimedBlock> out;
    for (size_t i = 0; i < raw.size();) {
        TimedBlock block = raw[i++];
        while (block.end - block.start < minimum_duration_ms && i < raw.size()) {
            // A short caption becomes the leading text of the immediately
            // following caption.  No separator is inserted in the initial UI.
            block.text += raw[i].text;
            block.end = raw[i++].end;
        }
        if (i == raw.size() && block.end - block.start < minimum_duration_ms)
            block.end = std::max(block.end, block.start + minimum_duration_ms);
        if (block.end > block.start) out.push_back(std::move(block));
    }
    return out;
}

inline bool parse_offset(const std::wstring& input, int64_t& result) {
    size_t i = 0;
    while (i < input.size() && iswspace(input[i])) ++i;
    bool negative = false;
    if (i < input.size() && (input[i] == L'+' || input[i] == L'-')) negative = input[i++] == L'-';
    int64_t whole = 0, fraction = 0;
    int digits = 0, decimal_digits = 0;
    while (i < input.size() && input[i] >= L'0' && input[i] <= L'9') {
        whole = whole * 10 + (input[i++] - L'0'); ++digits;
        if (whole > 999999) return false;
    }
    if (i < input.size() && input[i] == L'.') {
        ++i;
        while (i < input.size() && input[i] >= L'0' && input[i] <= L'9') {
            if (++decimal_digits > 3) return false;
            fraction = fraction * 10 + (input[i++] - L'0');
        }
        if (!decimal_digits) return false;
    }
    if (!digits && !decimal_digits) return false;
    while (i < input.size() && iswspace(input[i])) ++i;
    if (i != input.size()) return false;
    for (int n = decimal_digits; n < 3; ++n) fraction *= 10;
    result = (whole * 1000 + fraction) * (negative ? -1 : 1);
    return true;
}

inline std::vector<Block> merge_captions(std::vector<Caption> captions) {
    std::stable_sort(captions.begin(), captions.end(), [](const Caption& a, const Caption& b) {
        return a.layer != b.layer ? a.layer < b.layer : a.start < b.start;
    });
    std::set<int> boundaries;
    for (const auto& c : captions) { boundaries.insert(c.start); boundaries.insert(c.end); }
    std::vector<Block> result;
    if (boundaries.size() < 2) return result;
    for (auto it = boundaries.begin(), next = std::next(it); next != boundaries.end(); ++it, ++next) {
        std::string text;
        for (const auto& c : captions) if (c.start <= *it && c.end >= *next) {
            if (!text.empty()) text += '\n';
            text += c.text;
        }
        if (text.empty()) continue;
        if (!result.empty() && result.back().end == *it && result.back().text == text) result.back().end = *next;
        else result.push_back({*it, *next, text});
    }
    return result;
}

inline int64_t frame_milliseconds(int frame, int rate, int scale) {
    int64_t n = int64_t(frame) * scale * 1000;
    return (n + rate / 2) / rate;
}

inline std::vector<TimedBlock> build_timeline(std::vector<Caption> captions, int rate, int scale,
                                               int64_t minimum_duration_ms) {
    std::stable_sort(captions.begin(), captions.end(), [](const Caption& a, const Caption& b) {
        return a.start != b.start ? a.start < b.start : a.layer < b.layer;
    });
    std::vector<TimedBlock> raw;
    for (size_t i=0; i<captions.size(); ++i) {
        int end = i+1 < captions.size() ? captions[i+1].start : captions[i].end;
        raw.push_back({frame_milliseconds(captions[i].start,rate,scale),
                       frame_milliseconds(end,rate,scale),captions[i].text});
    }
    return merge_timed_blocks(raw, minimum_duration_ms);
}

inline std::vector<TimedBlock> shift_timeline(const std::vector<TimedBlock>& blocks,
                                               int64_t offset, size_t* omitted=nullptr) {
    std::vector<TimedBlock> out; size_t n=0;
    for (auto block:blocks) {
        block.start+=offset; block.end+=offset;
        if(block.end<=0 || block.end<=block.start) { ++n; continue; }
        block.start=std::max<int64_t>(0,block.start);
        out.push_back(block);
    }
    if(omitted) *omitted=n;
    return out;
}

inline std::string format_milliseconds(int64_t ms) {
    char s[48];
    std::snprintf(s, sizeof(s), "%02lld:%02lld:%02lld,%03lld", ms/3600000, ms/60000%60, ms/1000%60, ms%1000);
    return s;
}

inline std::string timestamp(int frame, int rate, int scale) {
    return format_milliseconds(frame_milliseconds(frame, rate, scale));
}

inline std::string render_srt(const std::vector<TimedBlock>& blocks) {
    std::string out;
    for(size_t i=0;i<blocks.size();++i) {
        out+=std::to_string(i+1)+"\r\n"+format_milliseconds(blocks[i].start)+" --> "+format_milliseconds(blocks[i].end)+"\r\n";
        for(char c:blocks[i].text) { if(c=='\n') out+="\r\n"; else if(c!='\r') out+=c; }
        out+="\r\n\r\n";
    }
    return out;
}

inline std::string srt(const std::vector<Block>& blocks, int rate, int scale,
                       int64_t offset_ms = 0, size_t* written = nullptr, size_t* omitted = nullptr) {
    std::string out;
    size_t count = 0, skipped = 0;
    for (size_t i = 0; i < blocks.size(); ++i) {
        int64_t start = frame_milliseconds(blocks[i].start, rate, scale) + offset_ms;
        int64_t end = frame_milliseconds(blocks[i].end, rate, scale) + offset_ms;
        if (end <= 0 || end <= start) { ++skipped; continue; }
        start = std::max<int64_t>(0, start);
        out += std::to_string(++count) + "\r\n" + format_milliseconds(start) + " --> " + format_milliseconds(end) + "\r\n";
        for (char c : blocks[i].text) { if (c == '\n') out += "\r\n"; else if (c != '\r') out += c; }
        out += "\r\n\r\n";
    }
    if (written) *written = count;
    if (omitted) *omitted = skipped;
    return out;
}
