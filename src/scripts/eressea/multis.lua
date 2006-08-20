local function kill_multis()
  local multis = { 
--    ["Luna"]="Wird wegen Missbrauch von Sonnensegeln geloescht",
--    ["amam"]="Wird wegen Missbrauch von Sonnensegeln geloescht",
--    ["jr81"]="Wird wegen Missbrauch von Sonnensegeln geloescht"
  }
  for k, v in multis do
    local f = get_faction(atoi36(k))
    if f~=nil then
      print("- marking " .. tostring(f) .. " as a multi-player.")
      f.email="doppelspieler@eressea.de"
      f.password=""
      f.info=v
    else
      print("- could not find faction " .. k)
    end
  end
end

print("killing multi-players")
kill_multis()
