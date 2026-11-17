-- SRTrack Importer handler for GCMZDrops2.
--
-- This initial implementation has one responsibility only: when a supported
-- media file and a same-name SRT file are dropped together as a pair, replace
-- the media file with a two-object alias (video below, SRTrack Object above)
-- whose two objects share a group value.

local M = {
  name = "SRTrack Importer",
  priority = 1000,
}

-- AviUtl2's standard AVI reader plus the extensions registered by the
-- L-SMASH Works reader used by SRTrack's supported environment.  The actual
-- video-track check is still performed in drop(), because some extensions
-- also represent audio-only media.
local supported_extensions = {
  avi = true,
  mp4 = true, m4v = true, m4a = true, mov = true, qt = true,
  ["3gp"] = true, ["3g2"] = true, f4v = true, ismv = true, isma = true,
  mpg = true, mpeg = true, m1v = true, mts = true, m2t = true, m2ts = true,
  ts = true, vob = true, dv = true, flv = true, mkv = true, webm = true,
  wmv = true, asf = true, lwi = true, avs = true, vpy = true,
}

local function extension(path)
  local ext = path:match("%.([^./\\]+)$")
  return ext and ext:lower() or nil
end

local function sibling_srt(path)
  return path:gsub("%.[^./\\]+$", ".srt")
end

local function file_exists(path)
  local file = io.open(path, "rb")
  if not file then return false end
  file:close()
  return true
end

local function filename(path)
  return path:match("([^/\\]+)$") or path
end

local function escape_alias_value(value)
  return value:gsub("[\r\n]", "")
end

local function append_standard_drawing(lines, effect_name, audio)
  table.insert(lines, "effect.name=" .. effect_name)
  table.insert(lines, "X=0.00")
  table.insert(lines, "Y=0.00")
  table.insert(lines, "Z=0.00")
  table.insert(lines, "Group=1")
  table.insert(lines, "中心X=0.00")
  table.insert(lines, "中心Y=0.00")
  table.insert(lines, "中心Z=0.00")
  table.insert(lines, "Group3=1")
  table.insert(lines, "X軸回転=0.00")
  table.insert(lines, "Y軸回転=0.00")
  table.insert(lines, "Z軸回転=0.00")
  table.insert(lines, "Group2=1")
  table.insert(lines, "拡大率=100.000")
  table.insert(lines, "縦横比=0.000")
  table.insert(lines, "透明度=0.00")
  table.insert(lines, "合成モード=通常")
  if audio then
    table.insert(lines, "音量=100.00")
    table.insert(lines, "左右=0.00")
  end
end

local function write_alias(pairs, project)
  local path = gcmz.create_temp_file("srtrack.object")
  if not path then return nil end

  local lines = {}
  local next_frame = 0
  local object_index = 0
  for group, pair in ipairs(pairs) do
    local frame_count = math.max(1, math.floor(pair.duration * project.rate / project.scale + 1e-9))
    local last_frame = next_frame + frame_count - 1
    local duration = string.format("%.3f", frame_count * project.scale / project.rate)

    -- The upper subtitle Object.
    table.insert(lines, "[" .. object_index .. "]")
    table.insert(lines, "layer=2")
    table.insert(lines, "frame=" .. next_frame .. "," .. last_frame)
    table.insert(lines, "group=" .. group)
    table.insert(lines, "[" .. object_index .. ".0]")
    table.insert(lines, "effect.name=SRTrackObject")
    table.insert(lines, "SRT ファイル=" .. escape_alias_value(pair.srt))
    table.insert(lines, "全長=" .. duration)
    table.insert(lines, "字幕プレビュー=1")
    table.insert(lines, "[" .. object_index .. ".1]")
    append_standard_drawing(lines, "標準描画", false)
    object_index = object_index + 1

    -- The lower standard video Object.
    table.insert(lines, "[" .. object_index .. "]")
    table.insert(lines, "layer=1")
    table.insert(lines, "frame=" .. next_frame .. "," .. last_frame)
    table.insert(lines, "group=" .. group)
    table.insert(lines, "[" .. object_index .. ".0]")
    table.insert(lines, "effect.name=動画ファイル")
    table.insert(lines, "再生位置=0.000," .. duration .. ",再生範囲,0")
    table.insert(lines, "再生速度=100.00")
    table.insert(lines, "ファイル=" .. escape_alias_value(pair.video))
    table.insert(lines, "トラック=0")
    table.insert(lines, "ループ再生=0")
    table.insert(lines, "音声付き=1")
    table.insert(lines, "YUV=")
    table.insert(lines, "fps調整=0")
    table.insert(lines, "[" .. object_index .. ".1]")
    append_standard_drawing(lines, "映像再生", true)
    object_index = object_index + 1
    next_frame = last_frame + 1
  end

  local file = io.open(path, "wb")
  if not file then return nil end
  file:write(table.concat(lines, "\r\n"), "\r\n")
  file:close()
  return path
end

function M.drag_enter(files, state)
  -- This hook may inspect metadata only.  Do not open files here.
  for _, file in ipairs(files) do
    local ext = extension(file.filepath)
    if ext and supported_extensions[ext] then return true end
  end
  return false
end

-- Do all work that may fail before modifying files.  Leaving files unchanged
-- is the documented way to hand them back to AviUtl2's regular input path.
local function prepare_pair_replacement(files)
  local project = gcmz.get_project_data()
  if not project or not project.rate or not project.scale or project.rate <= 0 or project.scale <= 0 then
    return nil
  end

  local pairs = {}
  for index, file in ipairs(files) do
    local ext = extension(file.filepath)
    if ext and supported_extensions[ext] then
      local srt = sibling_srt(file.filepath)
      if file_exists(srt) then
        local info = gcmz.get_media_info(file.filepath)
        if info and info.video_track_num > 0 and info.total_time and info.total_time > 0 then
          table.insert(pairs, {
            index = index,
            name = filename(file.filepath),
            video = file.filepath,
            srt = srt,
            duration = info.total_time,
          })
        end
      end
    end
  end

  if #pairs == 0 then return nil end
  table.sort(pairs, function(a, b) return a.name:lower() < b.name:lower() end)
  local alias = write_alias(pairs, project)
  if not alias then return nil end

  local handled = {}
  for _, pair in ipairs(pairs) do handled[#handled + 1] = pair.index end
  table.sort(handled, function(a, b) return a > b end)
  return { alias = alias, handled = handled }
end

function M.drop(files, state)
  -- gcmz APIs and filesystem operations may fail unexpectedly.  The protected
  -- phase does not mutate files, so a failure preserves every source video for
  -- AviUtl2 and its normal input plugins.
  local ok, replacement = pcall(prepare_pair_replacement, files)
  if not ok or not replacement then return end

  -- Mutation begins only after the complete replacement alias exists.
  for _, index in ipairs(replacement.handled) do
    table.remove(files, index)
  end
  table.insert(files, { filepath = replacement.alias, mimetype = "", temporary = true })
end

return M
