
function init_sphinxhints()

  for faction in factions() do

    for i=0,14,1 do
      hints[i] = 0
    end
    for i=0,4,1 do
        if faction.objects:get("sphinxhint"..tostring(i)) == nil then
          repeat
           hint = math.random(0,14)
          until hints[hint] = 0
          hints[hint] = 1
          faction.objects:set("sphinxhint"..tostring(i), tostring(hint))
      end
    end
  end
end

init_sphinxhints()

