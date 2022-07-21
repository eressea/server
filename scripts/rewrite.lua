require 'eressea.path'
require 'eressea'
require 'eressea.xmlconf' -- read xml data
require 'eressea.e2'

eressea.read_game(get_turn() .. '-orig.dat')
eressea.read_orders("C:\\Users\\enno\\source\\repos\\eressea\\orders.txt")
process_orders()
init_reports()
f = get_faction("enno")
write_report(f)
-- eressea.write_game(get_turn() .. '-new.dat')
