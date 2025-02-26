if not config.ponnuki or config.ponnuki==0 then return nil end
local ponnuki = {}

local directions = { "NW", "NO", "O", "SO", "SW", "W" }
local jokes = {
    "Ein Bummerang ist, wenn man ihn wegwirft und er kommt nicht wieder, dann war's keiner.",
    "Merke: Mit Schwabenwitzen soll man ganz sparsam sein.",
    "Was bekommt man, wenn man Katzen und Elfen kreuzt? Elfen ohne Rheuma.",
    "Was bekommt man, wenn man Insekten und Katzen kreuzt? Tiger, die Crisan benutzen.",
    'Ein Skelett kommt in eine Kneipe und sagt: "Ein Bier und einen Lappen!"',
    'Der Tod ist ein bleibender Schaden.',
    'Was macht ein Insekt auf einer Eisscholle? - Frieren.'
}

local function ponnuki_brain(u)
    u:clear_orders()
    local i = math.random(#jokes)
    u:add_order("BOTSCHAFT REGION \"" .. jokes[i] .. "\"")
    eressea.log.info("Ponnuki is telling jokes in " .. tostring(u.region))
    local glaciers = {}
    for i = 0, 5 do
        local r = u.region:next(i)
        if r.terrain == 'glacier' then
            table.insert(glaciers, i + 1)
        end
    end
    if #glaciers > 0 then
        local i = math.random(1, #glaciers)
        local d = glaciers[i]
        u:add_order("NACH " .. directions[d])
        local r = u.region:next(d - 1)
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
            end
            home.name = 'Magrathea'
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
    if u then
        ponnuki_brain(u)
    end
end

return ponnuki
