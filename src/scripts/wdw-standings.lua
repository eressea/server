conditions = { pyramid="Pyramide", gems="Handel", phoenix="Phönix" }

function log(file, line)
  print(line)
  file:write(line .. "\n")
end

function write_standings()
  print(reportpath .. "/victory.txt")
  local file = io.open(reportpath .. "/victory.txt", "w")
  
  log(file, "** Allianzen **")
  local alliance
  for alliance in alliances() do
    local faction
    log(file, alliance.id .. ": " .. alliance.name)
    for faction in alliance.factions do
      log(file, "- " .. faction.name .." (" .. itoa36(faction.id) .. ")")
    end
    log (file, "")
  end

  log(file, "** Erfüllte Siegbedingungen **")
  local condition
  for condition in conditions do
    local none = true
    log(file, conditions[condition])
    for alliance in alliances() do
      if victorycondition(alliance, condition)==1 then
        log(file, "  - " .. alliance.name .. " (" .. alliance.id .. ")")
        none = false
      end
    end
    if none then
      log(file, " - Niemand")
    end
  end

  file:close()
end

-- main body of script
write_standings()
