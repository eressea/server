if not config.xmas or config.xmas==0 then return nil end

local gifts = {
    e2 = {
        { year = 2015, turn = 959, item = 'snowglobe', msg='santa_f' },
        { year = 2009, turn = 624, item = 'xmastree' },
        { year = 2006, turn = 468, key = 'xm06', item = 'snowman' },
        { year = 2005, turn = 416, key = 'xm05', item = 'stardust' },
        { year = 2004, turn = 364, key = 'xm04', item = 'speedsail' }
    },
    e3 = {
        -- { year = 2015, turn = 338, item = 'snowglobe' },
        { year = 2009, turn = 26, key = 'xm09', item = 'xmastree' }
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
        for _, gift in ipairs(tbl) do
            if turn == gift.turn then
                give_gifts(gift)
            elseif gift.key and not get_key(gift.key) then
                give_gifts(gift)
                set_key(gift.key)
            end
        end
    end
end

return self
