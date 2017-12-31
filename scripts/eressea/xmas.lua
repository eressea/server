if not config.xmas or config.xmas==0 then return nil end

local gifts = {
    e2 = {
        [1057] = { year = 2017, item = 'snowglobe', msg='santa_f' },
        [959] = { year = 2015, item = 'snowglobe', msg='santa_f' },
        [624] = { year = 2009, item = 'xmastree', msg='santa_m' },
        [468] = { year = 2006, item = 'snowman', msg='santa_m'  },
        [416] = { year = 2005, item = 'stardust' },
        [364] = { year = 2004, item = 'speedsail' }
    },
    e3 = {
        [26] = { year = 2009, item = 'xmastree' }
    }
}

local function give_gifts(gift)
    eressea.log.info("Es weihnachtet sehr (" .. gift.year .. ")")
    local msg = nil
    if gift.msg then
        msg = message.create(gift.msg)
        msg:set_resource("item", gift.item)
    end
    if gift.item then
        for f in factions() do
            f:add_item(gift.item, 1)
            if msg then
                msg:send_faction(f)
            end
        end
    end
end

local self = {}

function self.init()
    local turn = get_turn()
    local tbl = gifts[config.rules]
    if tbl then
        gift = tbl[turn]
        if gift then 
            give_gifts(gift)
        end
    end
end

return self
