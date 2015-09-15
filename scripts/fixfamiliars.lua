require 'config'

function write_fam(file)
    for f in factions() do for u in f.units do if u.familiar then
        file:write(u.id .. " " .. u.familiar.id .. "\n")
    end end end
end

function read_fam(file)
    m, f = file:read("*n", "*n")
    while m and f do
        mag = get_unit(m)
        fam = get_unit(f)
        if mag and fam then
            mag.familiar = fam
        end
        m, f = file:read("*n", "*n")
    end
end

eressea.read_game(get_turn()..".dat")
file = io.open("familiars.txt", "r")
if file then
    read_fam(file)
    eressea.write_game(get_turn()..".fix")
else
    file = io.open("familiars.txt", "w")
    write_fam(file)
end
file:close()
