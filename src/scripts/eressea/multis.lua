local function email_multis()
  local multis = { 
    ["u9bx"]="Tachlaar@web.de",
    ["7Lwz"]="Tachlaar@web.de",
    ["ddr"]="Tachlaar@web.de",
    ["myrd"]="Tachlaar@web.de",
    ["2a4v"]="Samurai_krieger@web.de",
    ["7oiw"]="Samurai_krieger@web.de",
    ["brud"]="Samurai_krieger@web.de",
    ["bzcm"]="Samurai_krieger@web.de",
    ["crow"]="Samurai_krieger@web.de",
    ["dino"]="Samurai_krieger@web.de",
    ["fynd"]="Samurai_krieger@web.de",
    ["Leer"]="Samurai_krieger@web.de",
    ["moos"]="Samurai_krieger@web.de",
    ["ogcL"]="Samurai_krieger@web.de",
    ["paty"]="Samurai_krieger@web.de",
    ["rd"]="Samurai_krieger@web.de",
    ["seee"]="Samurai_krieger@web.de",
    ["szem"]="Samurai_krieger@web.de",
    ["uebL"]="Samurai_krieger@web.de",
    ["uvzp"]="Samurai_krieger@web.de",
    ["wzLp"]="Samurai_krieger@web.de",
    ["ziwe"]="Samurai_krieger@web.de"
  }
  local k
  local v
  for k, info in pairs(multis) do
    local f = get_faction(atoi36(k))
    if f~=nil then
      print("- marking " .. tostring(f) .. " as a multi-player.")
      f.email = v
      f.password = ""
      f.info = info
    else
      print("- could not find faction " .. k)
    end
  end
end

local function kill_multis()
  local multis = { 
    ["u9bx"]="Doppelspiel-Partei von Tachlaar@web.de",
    ["7Lwz"]="Doppelspiel-Partei von Tachlaar@web.de",
    ["ddr"]="Doppelspiel-Partei von Tachlaar@web.de",
    ["myrd"]="Doppelspiel-Partei von Tachlaar@web.de",
    ["2a4v"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["7oiw"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["brud"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["bzcm"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["crow"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["dino"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["fynd"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["Leer"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["moos"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["ogcL"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["paty"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["rd"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["seee"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["szem"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["uebL"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["uvzp"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["wzLp"]="Doppelspiel-Partei von Samurai_krieger@web.de",
    ["ziwe"]="Doppelspiel-Partei von Samurai_krieger@web.de"
  }
  local k
  local v
  for k, info in pairs(multis) do
    local f = get_faction(atoi36(k))
    if f~=nil then
      print("- marking " .. tostring(f) .. " as a multi-player.")
      f.email = "doppelspieler@eressea.de"
      f.password = ""
      f.info = info
    else
      print("- could not find faction " .. k)
    end
  end
end

print("killing multi-players")
email_multis()
