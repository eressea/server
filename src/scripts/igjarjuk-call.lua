function teleport(u, r)
  u.region:add_notice(tostring(u) .. " wird durchsichtig und verschwindet.")
  u.region = r
  u.region:add_notice(tostring(u) .. " erscheint.")
end

function call_igjarjuk()
  u = get_unit(atoi36("50ki"))
  if (u==nil) then
    return
  end
  fog = u.region
  
  laoris = get_unit(atoi36("cshL"))
  wyrm = get_unit(atoi36("igjr"))
  if (laoris==nil or wyrm==nil or laoris.region==fog) then
    return
  end
  
  -- make him a special kind of wyrm
  wyrm.race = "skeletal_wyrm"
  wyrm.hp = 10000
  wyrm.status = 1
  wyrm.magic = "nomagic"
  wyrm:set_skill("sk_magic", 20)
  wyrm.aura = 1000
  wyrm.faction:set_policy(laoris.faction, "fight", true)
  wyrm.faction:set_policy(get_faction(atoi36("dpen")), "fight", true)

  laoris:add_item("wand_of_tears", -6)
  -- inform the folks in the grave about what's up
  grave = wyrm.region
  fog:add_notice(tostring(laoris) .. " schwingt die Schwinge des Greifen. Das Auge des Dämons glüht in hellem weißen Licht, Blitze springen auf " .. tostring(wyrm) .. " über.")
  fog:add_notice(tostring(wyrm) .. " wird durchsichtig und verschwindet.")
  fog:add_notice(tostring(laoris) .. " wird durchsichtig und verschwindet.")
  
  -- andere Pentagramm-Meister
  teleport(get_unit(atoi36("q7qf")), fog)
  teleport(get_unit(atoi36("abqp")), fog)
  
  -- Wyrm und Laoris
  wyrm.region = fog
  laoris.region = fog
  
  fog:add_notice("von " .. tostring(wyrm) .. ": 'DER TAG MEINER RÜCKKEHR IST GEKOMMEN. MIT DEM BLUT DER STERBLICHEN WILL ICH AUF DIESEN TAG ANSTOSSEN.'")
  fog:add_notice(tostring(laoris) .. " erscheint. Auf seinem Rücken sind die Schwingen eines mächtigen Greifen zu sehen, in seinen Händen hält er beschwörend einen hell leuchtenden Kristall, das Auge des Dämon.")
  fog:add_notice(tostring(wyrm) .. " erscheint.")
  fog:add_notice("von " .. tostring(wyrm) .. ": 'WAS IST DAS? DIES IST NICHT DIE WELT DER STERBLICHEN. LAORIS! WOHIN HAST DU MICH GEBRACHT?'")
end
