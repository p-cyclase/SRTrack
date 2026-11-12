-- SRTrack Importer handler for GCMZDrops2.
-- Uses only GCMZDrops2's documented handler API.  The video file remains in
-- the drop list, so its normal reader (normally L-SMASH Works) still creates
-- the video object.

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

local function write_object_file(srt_path, source_duration, frame_length)
  local path, err = gcmz.create_temp_file("srtrack.object")
  if not path then
    debug_print("[SRTrack] Object temporary file creation failed: " .. tostring(err))
    return nil
  end

  -- A multi-object alias preserves the companion's one-layer offset and
  -- matching duration when AviUtl2 places the incoming drop set.
  local alias = table.concat({
    "[0]",
    "layer=1",
    "frame=0," .. tostring(frame_length - 1),
    "[0.0]",
    "effect.name=SRTrack Object",
    "SRT ファイル=" .. escape_alias_value(srt_path),
    "再生位置=0",
    "再生範囲終端=" .. tostring(source_duration),
    "動画全長=" .. tostring(source_duration),
    "再生速度=100",
    "字幕プレビュー=1",
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

function M.drop(files)
  local project = gcmz.get_project_data()
  if not project or not project.rate or not project.scale or project.rate <= 0 or project.scale <= 0 then
    return true
  end

  local additions = {}
  for _, file in ipairs(files) do
    local ext = extension(file.filepath)
    if ext and video_extensions[ext] then
      local srt_path = sibling_srt(file.filepath)
      if file_exists(srt_path) then
        local info, err = gcmz.get_media_info(file.filepath)
        if info and info.video_track_num > 0 and info.total_time and info.total_time > 0 then
          local frame_length = math.max(1, math.ceil(info.total_time * project.rate / project.scale - 1e-9))
          local object_path = write_object_file(srt_path, info.total_time, frame_length)
          if object_path then
            table.insert(additions, {
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

  for _, object_file in ipairs(additions) do
    table.insert(files, object_file)
  end
  return true
end

return M
