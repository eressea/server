function unitid(u)
  return u.name .. " (" .. itoa36(u.id) .. ")"
end

function teleport_all(map, grave)
  print("- teleporting all quest members to the grave")
  local index
  for index in map do
    local r = map[index]
	local u
	for u in r.units do
	  u.region = grave
	  print ("  .teleported " .. unitid(u))
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
   mapsize=mapsize+1
   map[mapsize] = r
 end

 local u
 for u in grave.units do
   if u.faction.id~=atoi36("rr") then
     teleport_all(map, grave)
	 break
   end
 end

 local index
 for index in map do
  local r = map[index]
  if r.x~=grave.x or r.y~=grave.y then
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

function write_emails()
  local locales = { "de", "en" }
  local files = {}
  local key
  for key in locales do
    local locale = locales[key]
	files[locale] = io.open(basepath .. "/emails." .. locale, "w")
  end

  local faction
  for faction in factions() do
    -- print(faction.id .. " - " .. faction.locale)
  	files[faction.locale]:write(faction.email .. "\n")
  end

  for key in files do
    files[key]:close()
  end
end

function process(orders)
  file = "" .. get_turn()
  if read_game(file)~=0 then 
    print("could not read game")
    return -1
  end

  -- initialize starting equipment for new players
  -- probably not necessary, since mapper sets new players, not server
  add_equipment("conquesttoken", 1);
  add_equipment("wood", 30);
  add_equipment("stone", 30);
  add_equipment("money", 2000 + get_turn() * 10);

  -- run the turn:
  read_orders(orders)
  process_orders()
  
  -- igjarjuk special
  wyrm()

  write_passwords()
  write_reports()

  write_emails()

  file = "" .. get_turn()
  if write_game(file)~=0 then 
    print("could not write game")
    return -1
  end
  
end


--
-- main body of script
--

-- orderfile: contains the name of the orders.
if orderfile==nil then
  print "you must specify an orderfile"
else
  process(orderfile)
end

