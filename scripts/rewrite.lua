dofile('config.lua')
eressea.read_game(get_turn() .. '-orig.dat')
eressea.write_game(get_turn() .. '-new.dat')
