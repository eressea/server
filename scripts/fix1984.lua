function recover(turn)
    eressea.free_game()
    eressea.read_game(turn .. ".dat")
    ships = {}
    bldgs = {}
    for r in regions() do
        for b in r.buildings do
            if b.info~=nil and (string.len(b.info)>120) then
                bldgs[b.id] = b.info
            end
        end
        for b in r.ships do
            if b.info~=nil and (string.len(b.info)>120) then
                ships[b.id] = b.info
            end
        end
    end
    eressea.free_game()
    eressea.read_game((turn+1) .. ".dat")
    for k, v in pairs(bldgs) do
        b = get_building(k)
        if b~=nil then
            b.info = v
        end
    end
    for k, v in pairs(ships) do
        b = get_ship(k)
        if b~=nil then
            b.info = v
        end
    end
    eressea.write_game((turn+1) .. ".fixed")
end
