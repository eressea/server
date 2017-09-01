if not config.ponnuki or config.ponnuki==0 then return nil end
local ponnuki = {}

local directions = { "NW", "NO", "O", "SO", "SW", "W" }
local jokes = {
    "Ein Bummerang ist, wenn man ihn wegwirft und er kommt nicht wieder, dann war's keiner.",
    "Merke: Mit Schwabenwitzen soll man ganz sparsam sein.",
    "Was bekommt man, wenn man Katzen und Elfen kreuzt? Elfen ohne Rheuma.",
    "Was bekommt man, wenn man Insekten und Katzen kreuzt? Tiger, die Crisan benutzen."
}

local function ponnuki_brain(u)
    u:clear_orders()
    local i = math.random(#jokes)
    u:add_order("BOTSCHAFT REGION \"" .. jokes[i] .. "\"")
    eressea.log.info("Ponnuki is telling jokes in " .. tostring(u.region))
    local d = math.random(6)
    local r = u.region:next(d-1)
    if r.terrain == 'glacier' then
        u:add_order("NACH " .. directions[d])
        eressea.log.info("Ponnuki is walking to " .. tostring(r))
    end
end

function ponnuki.init()
    -- initialize other scripts
    local f = get_faction(666)
    local u = get_unit(atoi36("ponn"))
    if not u then
        eressea.log.error("Ponnuki is missing, will re-create")
        local home = get_region(-67, -5)
        if home and f then
            if home.terrain~="glacier" then
                home.terrain="glacier"
                home.name = 'Magrathea'
            end
            u = unit.create(f, home, 1, "template")
            if u then
                u.id = atoi36("ponn")
                u.name = "Ponnuki"
                u.info = "Go, Ponnuki, Go!"
                u.race_name = "Ritter von Go"
                u.status = 5 -- FLIEHE
                print(u:show())
            end
        else
            eressea.log.error("Ponnuki cannot find Magrathea")
        end
    elseif u.faction==f then
        eressea.log.info("Ponnuki is in " .. tostring(u.region))
        u.status = 5 -- FLIEHE
    end
    ponnuki_brain(u)
end

return ponnuki
