function write_grails(file)
  grails = {}
  for f in factions() do
    for u in f.units do
      if u:get_item("grail") > 0 then
        if grails[f.id] == nil then
          grails[f.id] = u:get_item("grail")
        else
          grails[f.id] = grails[f.id] + u:get_item("grail")
        end
      end
    end
  end

  file:write("Parteien mit mehr oder weniger als einem Gral:\n")

  for k in factions() do
    v = 0
    if grails[k.id] ~= nil then
      v = grails[k.id]
    end
    if v~= 1 then
      file:write("- " .. v .. " Grale: " .. k.name .. " (" .. itoa36(k.id) .. ")\n")
    end
  end
end
