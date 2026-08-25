require 'config'

region.create(1, 0, 'desert').name = 'Armouth'
region.create(-1, 0, 'ocean')
region.create(-1, 1, 'ocean')
region.create(0, 1, 'ocean')
region.create(0, -1, 'ocean')
region.create(1, -1, 'hellfire').name = 'Dis'

r = region.create(0, 0, 'plain')
r.name = 'Gont'
f = faction.create('orc', 'ged@example.com', 'de')
f.name = 'Wizards of Earthsea'
ship.create(r, 'boat').name = 'Shadow'
b = building.create(r, 'lighthouse')
b.name = 'Gont Port'
b = building.create(r, 'castle')
b.name = 'The Tombs of Atuan'
u = unit.create(f, r)
equip_unit(u, "seed_faction")
equip_unit(u, "seed_unit")
equip_unit(u, "seed_orc", 7)
u.name = 'Sparrowhawk'
u.ship = ship.create(r, 'caravel')
u.ship.name = 'Lookfar'
print(u)
print(r, r.terrain)
init_reports()
write_reports()
eressea.write_game(get_turn() .. '.dat')
