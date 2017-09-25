dofile('config.lua')
eressea.read_game(get_turn() .. '.dat')
init_reports()
write_reports()
write_map(config.reportpath .. '/' .. get_turn() .. 'map.cr')

