-- SRTrack Importer handler for GCMZDrops2.
-- Uses only GCMZDrops2's documented handler API.  A matching video/SRT pair
-- is replaced with one standard AviUtl2 multi-object alias, which preserves
-- the pair's group relationship on import.

local M = {
  name = "SRTrack Importer",
  priority = 1000,
}

local function extension(path)
  local ext = path:match("%.([^./\\]+)$")
  return ext and ext:lower() or nil
end

local function sibling_srt(path)
  return (path:gsub("%.[^./\\]+$", ".srt"))
end

local function filename(path)
  return path:match("([^/\\]+)$") or path
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

local function append_standard_drawing(lines, effect_name, audio)
  table.insert(lines, effect_name)
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

local function write_grouped_alias(pairs, project)
  local path, err = gcmz.create_temp_file("srtrack.object")
  if not path then
    debug_print("[SRTrack] Object temporary file creation failed: " .. tostring(err))
    return nil
  end

  -- A single multi-object alias prevents AviUtl2 from allocating a new layer
  -- pair for every source file.  Each pair has a distinct group value, while
  -- all pairs use the same video/Object layer pair and are laid out in time.
  local lines = {}
  local next_frame = 0
  local object_index = 0
  for group, pair in ipairs(pairs) do
    local frame_time = pair.source_duration * project.rate / project.scale
    local frame_count = math.max(1, math.floor(frame_time + 1e-9))
    local last_frame = next_frame + frame_count - 1
    local duration = seconds(frame_count * project.scale / project.rate)

    table.insert(lines, "[" .. tostring(object_index) .. "]")
    table.insert(lines, "layer=2")
    table.insert(lines, "frame=" .. tostring(next_frame) .. "," .. tostring(last_frame))
    table.insert(lines, "group=" .. tostring(group))
    table.insert(lines, "[" .. tostring(object_index) .. ".0]")
    table.insert(lines, "effect.name=SRTrack Object")
    table.insert(lines, "SRT ファイル=" .. escape_alias_value(pair.srt_path))
    table.insert(lines, "再生位置=0.000")
    table.insert(lines, "再生範囲終端=" .. duration)
    table.insert(lines, "動画全長=" .. duration)
    table.insert(lines, "再生速度=100.00")
    table.insert(lines, "ループ再生=0")
    table.insert(lines, "字幕プレビュー=1")
    table.insert(lines, "[" .. tostring(object_index) .. ".1]")
    append_standard_drawing(lines, "effect.name=標準描画", false)
    object_index = object_index + 1

    table.insert(lines, "[" .. tostring(object_index) .. "]")
    table.insert(lines, "layer=1")
    table.insert(lines, "frame=" .. tostring(next_frame) .. "," .. tostring(last_frame))
    table.insert(lines, "group=" .. tostring(group))
    table.insert(lines, "[" .. tostring(object_index) .. ".0]")
    table.insert(lines, "effect.name=動画ファイル")
    table.insert(lines, "再生位置=0.000," .. duration .. ",再生範囲,0")
    table.insert(lines, "再生速度=100.00")
    table.insert(lines, "ファイル=" .. escape_alias_value(pair.video_path))
    table.insert(lines, "トラック=0")
    table.insert(lines, "ループ再生=0")
    table.insert(lines, "音声付き=1")
    table.insert(lines, "YUV=")
    table.insert(lines, "fps調整=0")
    table.insert(lines, "[" .. tostring(object_index) .. ".1]")
    append_standard_drawing(lines, "effect.name=映像再生", true)
    object_index = object_index + 1
    next_frame = last_frame + 1
  end
  table.insert(lines, "")
  local alias = table.concat(lines, "\r\n")

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
    -- Do not maintain a second extension list.  When a same-name SRT exists,
    -- defer format support to the installed L-SMASH Works reader (or AVI
    -- reader) and verify the actual video track in drop().
    if ext and ext ~= "srt" and file_exists(sibling_srt(file.filepath)) then
      return true
    end
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
  local pairs = {}
  local handled_indices = {}
  for index = #files, 1, -1 do
    local file = files[index]
    local ext = extension(file.filepath)
    if ext and ext ~= "srt" then
      local srt_path = sibling_srt(file.filepath)
      if file_exists(srt_path) then
        local info, err = gcmz.get_media_info(file.filepath)
        if info and info.video_track_num > 0 and info.total_time and info.total_time > 0 then
          table.insert(pairs, 1, {
            video_path = file.filepath,
            srt_path = srt_path,
            source_duration = info.total_time,
            filename = filename(file.filepath),
            input_index = index,
          })
          table.insert(handled_indices, index)
        else
          debug_print("[SRTrack] Video information was unavailable: " .. tostring(err or file.filepath))
        end
      end
    end
  end

  if #pairs > 0 then
    table.sort(pairs, function(left, right)
      local left_name = left.filename:lower()
      local right_name = right.filename:lower()
      if left_name == right_name then
        return left.input_index < right.input_index
      end
      return left_name < right_name
    end)
    local object_path = write_grouped_alias(pairs, project)
    if object_path then
      for _, index in ipairs(handled_indices) do
        table.remove(files, index)
      end
      table.insert(files, {
        filepath = object_path,
        -- Leave MIME type unspecified so AviUtl2 dispatches the generated
        -- .object file through its ordinary extension-based loader.
        mimetype = "",
        temporary = true,
      })
    end
  end

  return true
end

return M
