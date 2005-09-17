local function kill_multis()
  local multis = { "aLuk", "3", "rd2r", "aaa", "no3i", "1Lru", "heLd" }
  for k, v in multis do
    local f = get_faction(atoi36(v))
    if f~=nil then
      print("- marking " .. tostring(f) .. " as a multi-player.")
      f.email="doppelspieler@eressea.de"
      f.password=""
    else
      print("- could not find faction " .. v)
    end
  end
end

print("killing multi-players")
kill_multis()
