-- SRTrack Importer handler for GCMZDrops2.
-- Uses only GCMZDrops2's documented handler API.  A matching video/SRT pair
-- is replaced with one standard AviUtl2 multi-object alias, which preserves
-- the pair's group relationship on import.

local M = {
  name = "SRTrack Importer",
  priority = 1000,
}

-- The initial target is the usual L-SMASH Works video container set plus AVI.
-- A candidate is also required to contain a video track before it is changed.
local video_extensions = {
  avi = true, mp4 = true, m4v = true, mov = true, qt = true,
  ['3gp'] = true, ['3g2'] = true, f4v = true, ismv = true, ism = true,
  mkv = true, webm = true, wmv = true, asf = true, flv = true,
  mpg = true, mpeg = true, m2ts = true, mts = true, ts = true,
}

local function extension(path)
  local ext = path:match("%.([^./\\]+)$")
  return ext and ext:lower() or nil
end

local function sibling_srt(path)
  return (path:gsub("%.[^./\\]+$", ".srt"))
end

local function file_exists(path)
  local file = io.open(path, "rb")
  if not file then return false end
  file:close()
  return true
end

local function escape_alias_value(value)
  -- AviUtl2 alias values are UTF-8 text. File paths must remain one value.
  return value:gsub("[\r\n]", "")
end

local function seconds(value)
  return string.format("%.3f", value)
end

local function write_grouped_alias(video_path, srt_path, source_duration, project)
  local path, err = gcmz.create_temp_file("srtrack.object")
  if not path then
    debug_print("[SRTrack] Object temporary file creation failed: " .. tostring(err))
    return nil
  end

  -- This is the multi-object alias layout emitted by AviUtl2 itself for a
  -- grouped video and SRTrack Object.  Both object headers use group=1.
  -- The video occupies the lower layer; SRTrack Object is directly above it.
  local frame_time = source_duration * project.rate / project.scale
  local video_frames = math.max(1, math.floor(frame_time + 1e-9))
  local video_last = video_frames - 1
  -- AviUtl2's saved grouped aliases use exactly the same timeline frame range
  -- for every member.  E/D retain the more precise media duration separately.
  local object_last = video_last
  local duration = seconds(source_duration)
  -- The media object's range must end on a project-frame boundary.  Keep the
  -- precise media duration for SRTrack's E/D, but use the frame-derived end
  -- value for the standard video object's playback range.
  local video_end = seconds(video_frames * project.scale / project.rate)
  local alias = table.concat({
    "[0]",
    "layer=2",
    "frame=0," .. tostring(object_last),
    "group=1",
    "[0.0]",
    "effect.name=SRTrack Object",
    "SRT ファイル=" .. escape_alias_value(srt_path),
    "再生位置=0.000",
    "再生範囲終端=" .. duration,
    "動画全長=" .. duration,
    "再生速度=100.00",
    "字幕プレビュー=1",
    "[0.1]",
    "effect.name=標準描画",
    "X=0.00", "Y=0.00", "Z=0.00", "Group=1",
    "中心X=0.00", "中心Y=0.00", "中心Z=0.00", "Group3=1",
    "X軸回転=0.00", "Y軸回転=0.00", "Z軸回転=0.00", "Group2=1",
    "拡大率=100.000", "縦横比=0.000", "透明度=0.00", "合成モード=通常",
    "[1]",
    "layer=1",
    "frame=0," .. tostring(video_last),
    "group=1",
    "[1.0]",
    "effect.name=動画ファイル",
    "再生位置=0.000," .. video_end .. ",再生範囲,0",
    "再生速度=100.00",
    "ファイル=" .. escape_alias_value(video_path),
    "トラック=0", "ループ再生=0", "音声付き=1", "YUV=", "fps調整=0",
    "[1.1]",
    "effect.name=映像再生",
    "X=0.00", "Y=0.00", "Z=0.00", "Group=1",
    "中心X=0.00", "中心Y=0.00", "中心Z=0.00", "Group3=1",
    "X軸回転=0.00", "Y軸回転=0.00", "Z軸回転=0.00", "Group2=1",
    "拡大率=100.000", "縦横比=0.000", "透明度=0.00", "合成モード=通常", "音量=100.00", "左右=0.00",
    "",
  }, "\r\n")

  local file, open_err = io.open(path, "wb")
  if not file then
    debug_print("[SRTrack] Object temporary file write failed: " .. tostring(open_err))
    return nil
  end
  file:write(alias)
  file:close()
  return path
end

function M.drag_enter(files, state)
  for _, file in ipairs(files) do
    local ext = extension(file.filepath)
    if ext and video_extensions[ext] then return true end
  end
  return false
end

function M.drop(files, state)
  local project = gcmz.get_project_data()
  if not project or not project.rate or not project.scale or project.rate <= 0 or project.scale <= 0 then
    return true
  end

  -- Appending generated .object files is the GCMZDrops2 path already used by
  -- the initial importer.  Remove handled videos explicitly instead of
  -- assigning files[index], so the host cannot retain the original media item.
  local aliases = {}
  for index = #files, 1, -1 do
    local file = files[index]
    local ext = extension(file.filepath)
    if ext and video_extensions[ext] then
      local srt_path = sibling_srt(file.filepath)
      if file_exists(srt_path) then
        local info, err = gcmz.get_media_info(file.filepath)
        if info and info.video_track_num > 0 and info.total_time and info.total_time > 0 then
          local object_path = write_grouped_alias(file.filepath, srt_path, info.total_time, project)
          if object_path then
            table.remove(files, index)
            table.insert(aliases, 1, {
              filepath = object_path,
              -- Leave MIME type unspecified so AviUtl2 dispatches the generated
              -- .object file through its ordinary extension-based loader.
              mimetype = "",
              temporary = true,
            })
          end
        else
          debug_print("[SRTrack] Video information was unavailable: " .. tostring(err or file.filepath))
        end
      end
    end
  end

  for _, alias_file in ipairs(aliases) do
    table.insert(files, alias_file)
  end

  return true
end

return M
