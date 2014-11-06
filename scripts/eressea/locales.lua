local function change_locales(localechange)
  for loc, flist in pairs(localechange) do
    for index, name in pairs(flist) do
      f = get_faction(atoi36(name))
      if f ~= nil and f.locale ~= loc then
        print("LOCALECHANGE ", f, f.locale, loc)
        f.locale = loc
      end
    end
  end
end

local pkg = {}

function pkg.update()
  local localechange = { de = { 'ii' } }
  change_locales(localechange)
end

return pkg
