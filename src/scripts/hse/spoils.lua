function fname(f)
  return f.name .. " (" .. itoa36(f.id) .. ")"
end

function write_spoils(file)
  items = { "elfspoil", "demonspoil", "goblinspoil", "dwarfspoil", "halflingspoil", "humanspoil", "aquarianspoil", "insectspoil", "catspoil", "orcspoil", "trollspoil" }
  for index, iname in items do
    printed = false
    for f in factions() do
      trophies = 0
      for u in f.units do
        if u:get_item(iname) > 0 then
          trophies = trophies + u:get_item(iname)
        end
      end
      if trophies > 0 then
        if not printed then
	  file:write(get_string("de", iname .. "_p") .. "\n")
	  printed=true
	end
        if trophies == 1 then
          istr = get_string("de", iname)
        else
          istr = get_string("de", iname .. "_p")
        end
        file:write("- " .. trophies .. " " .. istr .. ": " .. fname(f) .. "\n")
      end
    end
  end
end
