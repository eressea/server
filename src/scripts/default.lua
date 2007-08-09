function write_emails(locales)
  local files = {}
  local key
  local locale
  local file
  for key, locale in pairs(locales) do
    files[locale] = io.open(basepath .. "/emails." .. locale, "w")
  end

  local faction
  for faction in factions() do
    -- print(faction.id .. " - " .. faction.locale)
    files[faction.locale]:write(faction.email .. "\n")
  end

  for key, file in pairs(files) do
    file:close()
  end
end

function write_addresses()
  local file
  local faction

  file = io.open(basepath .. "/adressen", "w")
  for faction in factions() do
    -- print(faction.id .. " - " .. faction.locale)
    file:write(tostring(faction) .. ":" .. faction.email .. ":" .. faction.banner .. "\n")
  end

  file:close()
end

function write_aliases()
  local file
  local faction

  file = io.open(basepath .. "/aliases." .. locale, "w")
  for faction in factions() do
    local unit
    file:write("partei-" .. itoa36(faction.id) .. ": " .. faction.email .. "\n")
    for unit in f.units do
      file:write("einheit-" .. itoa36(unit.id) .. ": " .. faction.email .. "\n")
    end
  end
 
  file:close()
end

function write_files(locales)
  write_passwords()
  write_reports()
  write_emails(locales)
  write_aliases()
  write_summary()
  write_addresses()
end
