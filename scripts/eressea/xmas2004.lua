function use_snowman(u, amount)
    if amount>0 and u.region.terrain == "glacier" then
        local man = unit.create(u.faction, u.region)
        man.race = "snowman"
        man.number = amount
        return amount
    end
    return -4
end

local self = {}

function self.update()
    if not get_key("xm04") then
        eressea.log.debug("Es weihnachtet sehr (2004)")
        set_key("xm04", true)
        for f in factions() do
            f:add_item("speedsail", 1)
            f:add_notice("santa2004")
        end
    end
end

return self
