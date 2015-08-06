function use_xmastree(u, amount)
    if u.region.herb~=nil then
        -- TODO: else?
        local trees = u.region:get_resource("tree")
        u.region:set_resource("tree", 10+trees)
        local msg = message.create("usepotion")
        msg:set_unit("unit", u)
        msg:set_resource("potion", "xmastree")
        msg:send_region(u.region)
        return amount
    end
    return 0
end

local xmas = {}

function xmas.update()
    if not get_key("xm09") then
        print("Es weihnachtet sehr (2009)")
        set_key("xm09", true)
        for f in factions() do
            f:add_item("xmastree", 1)
            local msg = message.create("msg_event")
            msg:set_string("string", translate("santa2006"))
            msg:send_faction(f)
        end
    end
end

return xmas
