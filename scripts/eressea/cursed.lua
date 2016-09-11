local function bitset(flags, bit)
    -- TODO: use bit32 when we no longer have to consider lua 5.1 compatibility
    local x = flags % (bit*2)
    return x >= bit
end

local function curse(file)
    for line in file:lines() do
        f = get_faction(line)
        if not f then
            print("no such faction: " .. line)
        elseif not bitset(f.flags, 16) then
            print("cursing " .. tostring(f))
            f.flags = f.flags + 16
        end
    end
end

local cursed = {}

function cursed.init() 
    local f = io.open("cursed.txt", "r")
    if f then
        print("found cursed.txt")
        curse(f)
        f:close()
    end
    f:close()
end

return cursed
