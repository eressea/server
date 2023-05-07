require 'config'

eressea.read_game(get_turn() .. ".dat")

local top = { 
    skill = '',
    unit = nil,
    value = 0
}

for r in regions() do
    for u in r.units do
        local sk = u.skills
        if sk then
            for k, v in pairs(sk) do
                local val = u:eff_skill(k)
                if val >= top.value then
                    top.value = val
                    top.unit = u
                    top.skill = k
                end
            end
        end
    end
end

print(top.unit, top.unit.faction, top.skill, top.value)
