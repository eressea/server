local pkg = {}

function pkg.update()
    local factions = { '777', '1wpy', 'd08a', 'hani', 'scaL' }
    for id in ipairs(factions) do
        local f = faction.get(id)
        if f then
            local o = f.options
            local bit = (math.floor(o / 8) % 2)
            if bit==0 then
                eressea.log.debug("enable JSON report for " .. tostring(f))
                f.options = o + 8
            end
        end
    end
end

return pkg
