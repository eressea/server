function main()
    for f in factions() do
        if f.race=="demon" then
            f.flags = 2147484672
            for u in f.units do
                u.building.size = 2
                u.building.name = u.region.name .. " Keep"
                u.name = "Lord " .. u.region.name
                u:add_item("money", 1000-u:get_item("money"))
            end
        else
            u = f.units()
            u:add_item("money", 1000-u:get_item("money"))
            u:add_item("adamantium", 1-u:get_item("adamantium"))
        end
    end
    for r in regions() do for u in r.units do
        print(u)
        things = ""
        comma = ""
        for i in u.items do
            things = things .. comma .. u:get_item(i) .. " " .. i
            comma = ", "
        end
        print(' - ' .. things)
    end end
end

if eressea==nil then
    print("this script is part of eressea")
else
    read_xml()
    eressea.read_game('0.dat')
    main()
    eressea.write_game('0.dat')
    print('done')
end
