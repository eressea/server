local function create_ents(r, number)
    local f = get_faction(666)
    if f~=nil and number>0 then
        u = unit.create(f, r, number, "ent")
        u.name = "Rasende Ents"
        u:set_skill("perception", 2)
        
        msg = message.create("entrise")
        msg:set_region("region", r)
        msg:send_region(r)
        return u
    end
    return nil
end

local ents = {}

function ents.update()
    local r
    for r in regions() do
        if r:get_flag(0) then -- RF_CHAOTIC
            if r.terrain == "plain" and r:get_resource("tree")==0 then
                if math.random(3)==1 then
                    u = create_ents(r, math.random(30))
                    if u ~= nil then
                        r:set_resource("tree", u.number)
                    end
                end
            end
        end
    end
end

return ents
