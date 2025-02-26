local function callbacks(rules, name, ...)
  for k, v in pairs(rules) do
    if 'table' == type(v) then
      cb = v[name]
      if 'function' == type(cb) then
        cb(...)
      end
    end
  end
end

require 'eressea'
require 'eressea.xmlconf' -- read xml data

local rules = {}
if config.rules then
  rules = require('eressea.' .. config.rules)
  eressea.log.info('loaded ' .. #rules .. ' modules for ' .. config.rules)
else
  eressea.log.warning('no rule modules loaded, specify a game in eressea.ini or with -r')
end

local turn = get_turn()
local crfile = "D:/Eressea/archive/" .. turn .. "-enno.cr"
eressea.import_cr(crfile)
eressea.fixup_import()
local orders = "D:/Eressea/" .. turn .. "-enno.txt"

turn_begin()
plan_monsters()
if eressea.read_orders(orders) ~= 0 then
    print("could not read " .. orders)
    return -1
end

callbacks(rules, 'init')
  
-- run the turn:
turn_process()
callbacks(rules, 'update')
turn_end() -- ageing, etc.
eressea.write_game("import.dat")
write_reports()

gmtool.editor()
