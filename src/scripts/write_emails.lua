
function write_emails()
  local locales = { "de", "en" }
  local files = {}
  local key
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

