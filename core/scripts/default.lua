function change_locales(localechange)
  for loc, flist in pairs(localechange) do
    for index, name in pairs(flist) do
      f = get_faction(atoi36(name))
      if f ~= nil then
        f.locale = loc
        print("LOCALECHANGE ", f, loc)
      end
    end
  end
end

function dbupdate()
  update_scores()
  edb = db.open(config.basepath.."/eressea.db")
  if edb~=nil then
    edb:update_factions()
    edb:update_scores()
  else
    print("could not open "..config.basepath.."/eressea.db")
  end
end

function nmr_check(maxnmrs)
  local nmrs = get_nmrs(1)
  if nmrs >= maxnmrs then
    print("Shit. More than " .. maxnmrs .. " factions with 1 NMR (" .. nmrs .. ")")
    write_summary()
    eressea.write_game("aborted.dat")
    return -1
  end
  print (nmrs .. " Factions with 1 NMR")
  return 0
end

function open_game(turn)
  file = "" .. get_turn()
  return eressea.read_game(file .. ".dat")
end

function write_emails(locales)
  local files = {}
  local key
  local locale
  local file
  for key, locale in pairs(locales) do
    files[locale] = io.open(config.basepath .. "/emails." .. locale, "w")
  end

  local faction
  for faction in factions() do
    if faction.email~="" then
      files[faction.locale]:write(faction.email .. "\n")
    end
  end

  for key, file in pairs(files) do
    file:close()
  end
end

function write_addresses()
  local file
  local faction

  file = io.open(config.basepath .. "/adressen", "w")
  for faction in factions() do
    -- print(faction.id .. " - " .. faction.locale)
    file:write(tostring(faction) .. ":" .. faction.email .. ":" .. faction.info .. "\n")
  end

  file:close()
end

function write_aliases()
  local file
  local faction

  file = io.open(config.basepath .. "/aliases", "w")
  for faction in factions() do
    local unit
    if faction.email ~= "" then
      file:write("partei-" .. itoa36(faction.id) .. ": " .. faction.email .. "\n")
      for unit in faction.units do
        file:write("einheit-" .. itoa36(unit.id) .. ": " .. faction.email .. "\n")
      end
    end
  end
 
  file:close()
end

function write_files(locales)
  write_passwords()
  write_reports()
  write_summary()
  -- write_emails(locales)
  -- write_aliases()
  -- write_addresses()
end

function write_scores()
  scores = {}
  for r in regions() do
    f = r.owner
    if f~=nil then
      value = scores[f.id]
      if value==nil then value=0 end
      value = value + r:get_resource("money")/100
      scores[f.id] = value
    end
  end
  for f in factions() do
    score=scores[f.id]
    if score==nil then score=0 end
    print(math.floor(score)..":"..f.name..":"..itoa36(f.id))
  end
end
