require 'bit32'

local function curse(file)
    for line in file:lines() do
        f = get_faction(line)
        if not f then
            print("no such faction: " .. line)
        elseif bit32.band(16, f.flags)==0 then
            print("cursing " .. tostring(f))
            f.flags = f.flags + 16
        else
            print("already cursed: " .. tostring(f))
        end
    end
end

local cursed = {}

function cursed.init() 
    print("curses!")
    local f = io.open("cursed.txt", "r")
    if f then
        print("found cursed.txt")
        curse(f)
    end
end

return cursed
