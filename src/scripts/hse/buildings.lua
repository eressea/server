function write_buildings(file)
  types = { "sawmill", "quarry", "mine", "lighthouse", "castle", "monument" }
  local index
  local tname
  for index, tname in pairs(types) do
    count = 0
    best = nil
    for r in regions() do
      for b in r.buildings do
        if b.type == tname then
          count = count + 1
          if best == nil then
            best = b
          else
            if best.size<b.size then
              best = b
            end
          end
        end
      end
    end
    if best ~= nil then
      file:write("\n" .. count .. " x " .. get_string("de", best.type) .. "\n")
      file:write("- " .. fname(best) .. ", Groesse " .. best.size .. "\n")
    end
  end
end
