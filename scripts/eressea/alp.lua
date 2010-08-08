require "callbacks"
require "dumptable"

local function trigger_alp_destroyed(alp, event)
    m = message.create("alp_destroyed")
    m:set_region("region", alp.region)
    m:send_faction(alp.faction)
end

local function trigger_alp_dissolve(u, event, attr)
    local alp = attr.alp
    attr.alp.number = 0 -- kills the alp
end

local function init_alp(attr)
    -- dumptable(attr)
    eventbus.register(attr.alp, "destroy", trigger_alp_destroyed)
    eventbus.register(attr.mage, "destroy", trigger_alp_dissolve, attr)
    eventbus.register(attr.target, "destroy", trigger_alp_dissolve, attr)
end

callbacks["init_alp"] = init_alp

-- Spell: summon alp
function summon_alp(r, mage, level, force, params)
    local alp = unit.create(mage.faction, r, 1, "alp")
    local target = params[1]
    alp:set_skill("stealth", 7)
    alp.status = 5 -- FLEE
    attr = attrib.create(alp, { ['name'] = 'alp', ['target'] = target, ['alp'] = alp, ['mage'] = mage })
    init_alp(attr)
    msg = message.create("summon_alp_effect")
    m:set_unit("mage", mage)
    m:set_unit("alp", alp)
    m:set_unit("target", target)
    m:send_faction(mage.faction)
end
