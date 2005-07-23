function kill_multis()
  local multis = [ "ogq", "idan", "e8L1" ]
  for key in multis do
    local f = get_faction(atoi36(multis[key]))
    if f~=nil then
      f.email="doppelspieler@eressea.de"
      f.password=""
    end
  end
end
