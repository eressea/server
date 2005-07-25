local function kill_multis()
  local multis = { "ogq", "idan", "e8L1" }
  for k, v in multis do
    print(v)
    local f = get_faction(atoi36(v))
    if f~=nil then
      print("Marking " .. tostring(f) .. " as a multi-player.")
      f.email="doppelspieler@eressea.de"
      f.password=""
    else
      print("- could not find faction " .. v)
    end
  end
end

print("killing multi-players")
kill_multis()
