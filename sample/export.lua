require 'config'

function export(filename)
    local cr = io.open(filename, 'w')
    cr:write('VERSION 69\n"UTF-8";charset\n"de";locale\n')
    for f in factions() do
        cr:write('PARTEI ' .. f.id)
        cr:write(string.format('"%s";Parteiname\n', f.name))
        cr:write(string.format('"%s";email\n', f.email))        
    end
    for r in regions() do
		if r.plane then
			local x, y = r.plane:normalize(r.x, r.y)
			cr:write(string.format('REGION %d %d %d\n', x, y, r.plane.id))
		else
			cr:write(string.format('REGION %d %d\n', r.x, r.y))
		end
		cr:write(string.format('%d;id\n', r.id))
        cr:write(string.format('"%s";Terrain\n', translate(r.terrain, 'de')))
		local s = r:get_resource('money')
		if s > 0 then cr:write(string.format('%d;Silber\n', s)) end
		local p = r.peasants
		if p > 0 then
			cr:write(string.format('%d;Bauern\n', p))
			cr:write(string.format('%d;Unterh\n', math.floor(p / 20)))
		end
        for x in r.buildings do
            cr:write(string.format('BURG %d\n', x.id))
            cr:write(string.format('"%s";Name\n', x.name))
			local typename = x:get_typename() or x.type
            cr:write(string.format('"%s";Typ\n', translate(typename, 'de')))
			cr:write(string.format('%d;Groesse\n', x.size))
        end
        for x in r.ships do
            cr:write(string.format('SCHIFF %d\n', x.id))
            cr:write(string.format('"%s";Name\n', x.name))
            cr:write(string.format('"%s";Typ\n', translate(x.type, 'de')))
            cr:write(string.format('%d;Groesse\n', x.size))
			local cap = x.units()
			if cap then
				cr:write(string.format('%d;Kapitaen\n', cap.id))
			end
        end
        for u in r.units do
            cr:write(string.format('EINHEIT %d\n', u.id))
            cr:write(string.format('"%s";Typ\n', translate('race::' .. u.race, 'de')))
            cr:write(string.format('"%s";Name\n', u.name))
            cr:write(string.format('%d;Partei\n', u.faction.id))
            cr:write(string.format('%d;Anzahl\n', u.number))
            if u.building then
                cr:write(string.format('%d;Burg\n', u.building.id))
            end
            if u.ship then
                cr:write(string.format('%d;Schiff\n', u.ship.id))
            end
        end
    end
end

eressea.read_game(get_turn() .. '.dat')
export("export.cr")
