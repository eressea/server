require 'config'
eressea.read_game(get_turn() .. '.dat')
r = get_region(-55, -22)
print(r)
roads = r.roads
for d = 1, 6 do
    print(roads[d])
end
for b in r.buildings do
    print(b)
end
