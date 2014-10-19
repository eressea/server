dofile('config.lua')
eressea.read_game(get_turn() .. '.dat')
init_reports()
write_reports()
