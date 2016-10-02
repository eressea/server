local ponnuki = {}

local directions = { "NW", "NO", "O", "SO", "SW", "W" }
local jokes = {
    "Ein Bummerang ist, wenn man ihn wegwirft und er kommt nicht wieder, dann war's keiner.",
    "Merke: Mit Schwabenwitzen soll man ganz sparsam sein.",
    "Was bekommt man, wenn man Katzen und Elfen kreuzt? Elfen ohne Rheuma.",
    "Was bekommt man, wenn man Insekten und Katzen kreuzt? Tiger, die Crisan benutzen."
}

local function ponnuki_brain(u)
  local i = math.random(#jokes)
  u:add_notice("Eine Botschaft von " .. tostring(u) .. ": " ..jokes[i])
  local d = math.random(6)
  local r = u.region:next(d-1)
  u:clear_orders()
  u:add_order("NACH " .. directions[d])
end

function ponnuki.init()
    -- initialize other scripts
    local u = get_unit(atoi36("ponn"))
    if not u then
        eressea.log.error("Ponnuki is missing, will re-create")
        local home = get_region(-67, -5)
        local f = get_faction(666)
        if home and f then
            u = unit.create(f, home, 1, "illusion")
            u.id = atoi36("ponn")
            u.name = "Ponnuki"
            u.info = "Go, Ponnuki, Go!"
            u:set_racename("Ritter von Go")
        else
            eressea.log.error("Ponnuki cannot find Magrathea")
        end
    end
    if u then
        ponnuki_brain(u)
    end
end

return ponnuki
