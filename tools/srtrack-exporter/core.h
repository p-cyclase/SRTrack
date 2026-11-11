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
                                               int64_t threshold_ms, int64_t final_min_ms) {
    std::stable_sort(captions.begin(), captions.end(), [](const Caption& a, const Caption& b) {
        return a.start != b.start ? a.start < b.start : a.layer < b.layer;
    });
    std::vector<TimedBlock> raw, out;
    for (size_t i=0; i<captions.size(); ++i) {
        int end = i+1 < captions.size() ? captions[i+1].start : captions[i].end;
        raw.push_back({frame_milliseconds(captions[i].start,rate,scale),
                       frame_milliseconds(end,rate,scale),captions[i].text});
    }
    for (size_t i=0; i<raw.size();) {
        TimedBlock block=raw[i++];
        while (block.end-block.start <= threshold_ms && i < raw.size()) {
            block.text += "\n" + raw[i].text;
            block.end = raw[i++].end;
        }
        if (i==raw.size() && block.end-block.start <= threshold_ms)
            block.end=std::max(block.end,block.start+final_min_ms);
        out.push_back(block);
    }
    return out;
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
