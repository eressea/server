require 'config'

eressea.read_game(get_turn() .. ".dat")
u = get_unit('maat')
print(u.ship.owner)
u.ship.owner = u
u.region:reorder_units()
print(u.ship.owner)
eressea.write_game(get_turn() .. ".dat")
