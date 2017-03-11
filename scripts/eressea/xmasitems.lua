local function get_direction(locale, token)
    local dir = eressea.locale.direction(locale, token)
    if dir and dir>=0 then
        return dir
    end
    return nil
end

local function error_message(msg, u, ord)
    local msg = message.create(msg)
    msg:set_unit("unit", u)
    msg:set_region("region", u.region)
    msg:set_order("command", ord)
    msg:send_faction(u.faction)
    return -1
end


local function usepotion_message(u, type)
  msg = message.create("usepotion")
  msg:set_unit("unit", u)
  msg:set_resource("potion", type)
  return msg
end

function use_stardust(u, amount)
  local p = u.region:get_resource("peasant")
  assert(p>0)
  p = math.ceil(1.5 * p)
  u.region:set_resource("peasant", p)
  local msg = usepotion_message(u, "stardust")
  msg:send_region(u.region)
  return 1
end

function use_snowglobe(u, amount, token, ord)
    local transform = {
        ocean = "glacier",
        firewall = "volcano",
        volcano = "mountain",
        desert = "plain"
    }
    local direction = get_direction(u.faction.locale, token)
    if direction then
        local r = u.region:next(direction)
        if r.units() then
            return error_message('target_region_not_empty', u, ord)
        end
        if r then 
            local trans = transform[r.terrain]
            if trans then
                r.terrain = trans
                return 1
            else
                return error_message('target_region_invalid', u, ord)
            end
        else
            return error_message('target_region_invalid', u, ord)
        end
    end
    return error_message('missing_direction', u, ord)
end

function use_snowman(u, amount)
    if amount>0 and u.region.terrain == "glacier" then
        local man = unit.create(u.faction, u.region, amount, "snowman")
        return amount
    end
    return -4
end

function use_xmastree(u, amount)
    if u.region.herb~=nil then
        -- TODO: else?
        local trees = u.region:get_resource("tree")
        u.region:set_key("xm06", true)
        u.region:set_resource("tree", 10+trees)
        local msg = usepotion_message(u, "xmastree")
        msg:send_region(u.region)
        return amount
    end
    return 0
end

local self = {}

function self.update()
    local turn = get_turn()
    local season = get_season(turn)
    if season == "calendar::winter" then
        eressea.log.debug("it is " .. season .. ", the christmas trees do their magic")
        local msg = message.create("xmastree_effect")
        for r in regions() do
            if r:get_key("xm06") then
                trees = r:get_resource("tree")
                if trees*0.1>=1 then
                    r:set_resource("tree", trees * 1.1)
                    msg:send_region(r)
                end
                if clear then
                end
            end
        end
    else
        local prevseason = get_season(turn-1)
        if prevseason == "calendar::winter" then
            -- we celebrate knut and kick out the trees.
            for r in regions() do
                if r:get_key("xm06") then
                    r:set_key("xm06", false)
                end
            end
        end
    end
end

return self
