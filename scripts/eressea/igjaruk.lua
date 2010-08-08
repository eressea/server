function teleport_all(map, grave)
  print("- teleporting all quest members to the grave")
  local index
  local r
  for index, r in pairs(map) do
    local u
    for u in r.units do
      u.region = grave
      print ("  .teleported " .. u.name)
      grave:add_notice("Ein Portal öffnet sich, und " .. u.name .. " erscheint in " .. grave.name)
    end
  end
end

function wyrm()
 print("- running the wyrm quest")
 local grave = get_region(-9995,4)
 local plane = get_plane_id("arena")
 local map = {}
 local mapsize = 0
 local r

 for r in regions() do
   if r.plane_id==plane then
     mapsize=mapsize+1
     map[mapsize] = r
   end
 end

 local u
 for u in grave.units do
   if u.faction.id~=atoi36("rr") then
     teleport_all(map, grave)
     break
   end
 end

 local index
 local r
 for index, r in pairs(map) do
  if r~=grave then
   if (math.mod(r.x,2)==math.mod(get_turn(),2)) then
     r:add_notice("Eine Botschaft von Igjarjuk, Herr der Wyrme: 'Die Zeit des Wartens ist beinahe vorrüber. Euer Fürst kehrt aus dem Grabe zurück.'")
   else
     r:add_notice("Eine Botschaft von Gwaewar, Herr der Greife: 'Das Ende naht. Igjarjuk ist aus seinem Grab auferstanden. Eilt, noch ist die Welt zu retten!'")
   end
  end
 end

 local gryph=get_unit(atoi36("gfd4"))
 local igjar=get_unit(atoi36("igjr"))
 if grave~=nil and gryph~=nil and igjar~=nil then
  gryph.region=grave
  igjar.region=grave
  grave:add_notice("Eine Botschaft von Gwaewar, Herr der Greife: 'Ihr, die Ihr die Strapazen der letzten Jahre überstanden habt: Lasst nicht zu, dass Igjarjuk wieder in die Welt der Lebenden zurückkehrt. Vernichtet das Auge - jetzt und hier!'")
  grave:add_notice("Eine Botschaft von Igjarjuk, Herr der Wyrme: 'Gwaewar, Du wirst dereinst an Deinem Glauben an das Gute in den Sterblichen verrecken... So wie ich es einst tat. Der Krieg ist die einzige Sprache die sie verstehen, und derjenige, der mir hilft, wird ihn gewinnen.'")
 end

end
