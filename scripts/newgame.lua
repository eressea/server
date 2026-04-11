require 'config'

region.create(0, 0, 'plain')
region.create(1, 0, 'ocean')
region.create(-1, 0, 'ocean')
region.create(-1, 1, 'ocean')
region.create(0, 1, 'ocean')
region.create(0, -1, 'ocean')
region.create(1, -1, 'ocean')

eressea.write_game(get_turn() .. '.dat')
