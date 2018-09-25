function nmr_check(maxnmrs)
  local nmrs = get_nmrs(1)
  if nmrs > maxnmrs then
    eressea.log.error("Shit. More than " .. maxnmrs .. " factions with 1 NMR (" .. nmrs .. ")")
    write_summary()
    return -1
  end
  print (nmrs .. " Factions with 1 NMR")
  return 0
end

function open_game(turn)
  file = "" .. get_turn()
  return eressea.read_game(file .. ".dat")
end

function callbacks(rules, name, ...)
	for k, v in pairs(rules) do
		if 'table' == type(v) then
			cb = v[name]
			if 'function' == type(cb) then
				cb(...)
			end
		end
	end
end

local function write_emails(locales)
  local files = {}
  local key
  local locale
  local file
  for key, locale in pairs(locales) do
    files[locale] = io.open(config.basepath .. "/emails." .. locale, "w")
  end

  local faction
  for faction in factions() do
    if faction.email~="" then
      files[faction.locale]:write(faction.email .. "\n")
    end
  end

  for key, file in pairs(files) do
    file:close()
  end
end

local function join_path(a, b)
    if a then return a .. '/' .. b end
    return b
end

local function write_addresses()
  local file
  local faction

  file = io.open(join_path(config.basepath, "adressen"), "w")
  for faction in factions() do
    -- print(faction.id .. " - " .. faction.locale)
    file:write(tostring(faction) .. ":" .. faction.email .. ":" .. faction.info .. "\n")
  end

  file:close()
end

local function write_aliases()
  local file
  local faction

  file = io.open(join_path(config.basepath, "aliases"), "w")
  for faction in factions() do
    local unit
    if faction.email ~= "" then
      file:write("partei-" .. itoa36(faction.id) .. ": " .. faction.email .. "\n")
      for unit in faction.units do
        file:write("einheit-" .. itoa36(unit.id) .. ": " .. faction.email .. "\n")
      end
    end
  end
 
  file:close()
end

local function write_htpasswd()
    local out = io.open(join_path(config.basepath, "htpasswd"), "w")
    if out then
        for f in factions() do
            if f.password then
                out:write(itoa36(f.id) .. ":" .. f.password .. "\n")
            end
        end
        out:close()
    end
end

local function write_files(locales)
    write_database()
    write_passwords()
    -- write_htpasswd()
    write_reports()
    write_summary()
end

local function write_scores()
  local scores = {}
  for r in regions() do
    f = r.owner
    if f~=nil then
      value = scores[f.id]
      if value==nil then value=0 end
      value = value + r:get_resource("money")/100
      scores[f.id] = value
    end
  end
  for f in factions() do
    score=scores[f.id]
    if score==nil then score=0 end
    print(math.floor(score)..":"..f.name..":"..itoa36(f.id))
  end
end

function process(rules, orders)
    local confirmed_multis = { }
    local suspected_multis = { }

    if open_game(get_turn())~=0 then
        eressea.log.error("could not read game")
        return -1
    end

    turn_begin()

    -- run the turn:
    if eressea.read_orders(orders) ~= 0 then
        print("could not read " .. orders)
        return -1
    end

    if nmr_check(config.maxnmrs or 80)~=0 then
        return -1
    end
    plan_monsters()
    callbacks(rules, 'init')
    turn_process()
    callbacks(rules, 'update')
    turn_end() -- ageing, etc.

    write_files(config.locales)
    update_scores()

    file = '' .. get_turn() .. '.dat'
    if eressea.write_game(file)~=0 then
        eressea.log.error("could not write game")
        return -1
    end
    return 0
end

function run_turn(rules)
    local turn = get_turn()
    if turn<0 then
        turn = read_turn()
        set_turn(turn)
    end

    orderfile = orderfile or config.basepath .. '/orders.' .. turn
    eressea.log.debug("executing turn " .. get_turn() .. " with " .. orderfile .. " with rules=" .. config.rules)
    local result = process(rules, orderfile)
    return result
end

function file_exists(name)
    local f=io.open(name,"r")
    if not f then
        return false
    end
    io.close(f)
    return true
end

if file_exists('execute.lock') then
    eressea.log.error("Lockfile exists, aborting.")
    assert(false)
end

math.randomseed(rng.random())

local path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'
require 'eressea'
require 'eressea.xmlconf' -- read xml data

local rules = {}
if config.rules then
    rules = require('eressea.' .. config.rules)
    eressea.log.info('loaded ' .. #rules .. ' modules for ' .. config.rules)
else
    eressea.log.warning('no rule modules loaded, specify a game in eressea.ini or with -r')
end
run_turn(rules)
