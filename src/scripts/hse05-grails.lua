function write_grails(filename)
  grails = {}
  for f in factions() do
    if f.id ~= 0 then
      for u in f.units do
        if u:get_item("grail") > 0 then
          if grails[f] == nil then
            grails[f] = u:get_item("grail")
          else
            grails[f] = grails[f] + u:get_item("grail")
	  end
        end
      end
    end
  end

  local file = io.open(reportpath .. "/" .. filename, "w")
  file:write("Parteien mit mehr oder weniger als einem Gral:\n")
  for k, v in grails do
    if v ~= 1 then
      file:write("- " .. v .. " Grale: " .. k.name .. " (" .. itoa36(k.id) .. ")\n")
    end
  end
  file:close()
end
