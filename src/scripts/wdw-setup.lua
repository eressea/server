positions = {}

function init_positions()
  -- init starting positions for the alliances here.
  positions = { 
    [11] = get_region(1,-12),
    [12] = get_region(10,-11),
    [13] = get_region(-7,-8),
    [14] = get_region(13,1),
    [15] = get_region(5,10),
    [17] = get_region(-6,14),
    [18] = get_region(-15,12),
    [19] = get_region(-15,6)
  }  
end

function get_position(aid)
-- return the position at which alliance 'aid' will start.
  print("Alliance " .. aid)
  local pos = positions[aid]
  
  -- hack, because i have no coordinates yet:
  if pos ~= nil and pos.terrain ~= "ocean" then
    return pos
  else
    -- find a region. let's use the region number 'aid' in the list, 
    -- so everyone gets their own
    -- print("cannot place alliance " .. aid .. " at " .. pos.x .. ", " .. pos.y)
    for pos in regions() do
      if pos.terrain ~= "ocean" then
        if aid==0 then
          return pos
        else
          aid = aid-1
        end
      end
    end
  end
end

gems = { "opal", "diamond", "zaphire", "topaz", "beryl", "agate", "garnet", "emerald" }

function get_gem(id)
  return gems[math.mod(id, 8)+1]
end

ano = 0 -- counting active alliance
numalliances = 8 -- number of alliances
lastalliance = nil

function make_faction(position, alliance, number, email, race)
  local skillno = 25 -- es gibt 25 skills in der liste
  local units = (1+skillno)*6 / number -- jede allianz kriegt 168 leute
  local money = units * 5 * 10 -- jede allianz kriegt 8400 silber
  
  local f = add_faction(email, race, "de")
  if f == nil then
    print("could not create " .. email .. " " .. race)
    return
  end
  
  
  print("\n" .. email .. " (" .. itoa36(f.id) .. ")")
  f.alliance = alliance
  local u = add_unit(f, position)
  -- erster ist der, der die extras kriegt:
  u.number = 1
  local units = units - 1
  u:add_item("money", money)
  if lastalliance==nil or alliance.id ~= lastalliance.id then
    ano=ano+1
    lastalliance = alliance
    u:add_item(get_gem(ano), numalliances-1)
    u:add_item(get_gem(ano+1), 2)
    u:add_item(get_gem(ano+2), 2)
    u:add_item("log", 50)
  end
  
  local sk
  for sk in skills do
    u = add_unit(f, position)

    -- anzahl personen berechnen
    local number = math.floor(units / skillno)
    units = units - number
    skillno = skillno - 1
    u.number = number

    local skill = skills[sk]
    u:set_skill(skill, 3)

    print("- " .. number .. " x " .. skill)
  end
end

-- skills that will be given to new units
skills = {
  "roadwork",
  "crossbow",
  "mining",
  "bow",
  "building",
  "trade",
  "forestry",
  "catapult",
  "herbalism",
  "training",
  "riding",
  "armorer",
  "shipcraft",
  "melee",
  "sailing",
  "polearm",
  "espionage",
  "quarrying",
  "stealth",
  "entertainment",
  "weaponsmithing",
  "cartmaking",
  "perception",
  "taxation",
  "stamina"
}

function wdw_setup()
  -- initialize starting equipment for new players
  -- equipment_setitem("new_faction", "magicskillboost", "1")

  init_positions()

  -- Initialize Wächter des Phoenix
  alliance = add_alliance(11, "Wächter des Phoenix")
  position = get_position(11)
  faction = make_faction(position, alliance, 4, "durgan@web.de", "Elfen")
  faction = make_faction(position, alliance, 4, "Daniel@gedankenwelt.net", "Trolle")
  faction = make_faction(position, alliance, 4, "rostnicht@web.de", "Dämonen")
  faction = make_faction(position, alliance, 4, "16419@uni-lueneburg.de", "Insekten")

  -- Initialize Refinius
  alliance = add_alliance(12, "Allianz 12")
  position = get_position(12)
  faction = make_faction(position, alliance, 5, "wanderameisen@dunklerpfad.de", "Meermenschen")
  faction = make_faction(position, alliance, 5, "ElunasErben@dunklerpfad.de", "Elfen")
  faction = make_faction(position, alliance, 5, "miles.tegson@gmx.net", "Zwerge")
  faction = make_faction(position, alliance, 5, "shannera@shannera.de", "Dämonen")
  faction = make_faction(position, alliance, 5, "langhaarigerBombenleger@firemail.de", "Trolle")

  -- Initialize Stählerner Bund von Parinor
  alliance = add_alliance(13, "Stählerner Bund von Parinor")
  position = get_position(13)
  faction = make_faction(position, alliance, 5, "ta@sts-gbr.de", "Dämonen")
  faction = make_faction(position, alliance, 5, "klausnet@gmx.de", "Elfen")
  faction = make_faction(position, alliance, 5, "r.jerger@gmx.de", "Trolle")
  faction = make_faction(position, alliance, 5, "christianemmler@t-online.de", "Insekten")
  faction = make_faction(position, alliance, 5, "DieCherusker@gmx.de", "Orks")

  -- Initialize Ethâra´s Zirkel des Chaos
  alliance = add_alliance(14, "Ethâra´s Zirkel des Chaos")
  position = get_position(14)
  faction = make_faction(position, alliance, 4, "craban@web.de", "Elfen")
  faction = make_faction(position, alliance, 4, "saressa@celtic-visions.net", "Zwerge")
  faction = make_faction(position, alliance, 4, "krachon@gmx.de", "Orks")
  faction = make_faction(position, alliance, 4, "andreas.westhoff@physik3.gwdg.de", "Meermenschen")

  -- Initialize Janustempler
  alliance = add_alliance(15, "Janustempler")
  position = get_position(15)
  faction = make_faction(position, alliance, 4, "alex.goelkel@t-online.de", "Meermenschen")
  faction = make_faction(position, alliance, 4, "matthias.lendholt@gmx.de", "Elfen")
  faction = make_faction(position, alliance, 4, "Uwe.Kopf@onlinehome.de", "Orks")
  faction = make_faction(position, alliance, 4, "Jan.Thaler@gmx.de", "Dämonen")

  -- Initialize Kiddies
  alliance = add_alliance(17, "Kiddies")
  position = get_position(17)
  faction = make_faction(position, alliance, 5, "henning.ewald@t-online.de", "Orks")
  faction = make_faction(position, alliance, 5, "red@gmx.de", "Meermenschen")
  faction = make_faction(position, alliance, 5, "Morgon@Morgon.de", "Zwerge")
  faction = make_faction(position, alliance, 5, "hadmar.lang@wu-wien.ac.at", "Elfen")
  faction = make_faction(position, alliance, 5, "vinyambar@burningchaos.org", "Dämonen")

  -- Initialize Airbe Druad
  alliance = add_alliance(18, "Airbe Druad")
  position = get_position(18)
  faction = make_faction(position, alliance, 6, "t.lam@gmx.at", "Elfen")
  faction = make_faction(position, alliance, 6, "Post_der_Trolle@gmx.de", "Dämonen")
  faction = make_faction(position, alliance, 6, "Mann.Martin@t-online.de", "Meermenschen")
  faction = make_faction(position, alliance, 6, "thaeberli@freesurf.ch", "Orks")
  faction = make_faction(position, alliance, 6, "sonja@tzi.de", "Trolle")
  faction = make_faction(position, alliance, 6, "lothar.juerss@gmx.at", "Zwerge")

  -- Initialize Matula
  alliance = add_alliance(19, "Allianz 19")
  position = get_position(19)
  faction = make_faction(position, alliance, 4, "mserrano@tiscali.de", "Meermenschen")
  faction = make_faction(position, alliance, 4, "kerki@aol.com", "Elfen")
  faction = make_faction(position, alliance, 4, "Roddiwi@aol.com", "Zwerge")
  faction = make_faction(position, alliance, 4, "bauschan@aol.com", "Trolle")
  
end


--
-- main body of the script
wdw_setup()
