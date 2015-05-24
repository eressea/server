local pkg = {}

print("loading jsreport module")

function pkg.init()
    eressea.settings.set("feature.jsreport.enable", "1")
end

function pkg.update()
    local factions = { '777', '1wpy', 'd08a', 'hani', 'scaL' }
    for _, id in ipairs(factions) do
        local f = faction.get(id)
        if f then
            local o = f.options
            local bit = (math.floor(o / 8) % 2)
            if bit==0 then
                eressea.log.info("enable JSON report for " .. tostring(f))
                f.options = o + 8
            end
        end
    end
end

return pkg
