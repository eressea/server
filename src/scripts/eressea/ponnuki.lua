function ponnuki_brain(u)
  jokes = {
    "Ein Bummerang ist, wenn man ihn wegwirft und er kommt nicht wieder, dann war's keiner.",

    "Merke: Mit Schwabenwitzen soll man ganz sparsam sein.",

    "Was bekommt man, wenn man Katzen und Elfen kreuzt? Elfen ohne Rheuma.",

    "Was bekommt man, wenn man Insekten und Katzen kreuzt? Tiger, die Crisan benutzen."
  }
  local i = math.random(table.getn(jokes))
  u.region:add_notice(jokes[i])
  local d = math.random(6)
  r = u.region:next(d-1)
  u:clear_orders()
  directions = { "NW", "NO", "O", "SO", "SW", "W" }
  u:add_order("NACH " .. directions[d])
end

local function init_ponnuki(home)
  local f = get_faction(0)
  local u = get_unit(atoi36("ponn"))
  if u == nil then
    u = add_unit(f, home)
    u.id = atoi36("ponn")
    u.name = "Ponnuki"
    u.info = "Go, Ponnuki, Go!"
    u.race = "illusion"
    u:set_racename("Ritter von Go")
  end
  if u.faction==f then
    set_unit_brain(u, ponnuki_brain)
  end
end

-- initialize other scripts
local magrathea = get_region(-67, -5)
if magrathea~=nil and init_ponnuki~=nil then
  init_ponnuki(magrathea)
  return
end

