
function give_herbbag(unit)
  unit:add_item("h1",50);
  unit:add_item("h2",50);
  unit:add_item("h3",50);
  unit:add_item("h4",50);
  unit:add_item("h5",50);
  unit:add_item("h6",50);
  unit:add_item("h7",50);
  unit:add_item("h8",50);
  unit:add_item("h9",50);
  unit:add_item("h10",50);
  unit:add_item("h11",50);
  unit:add_item("h12",50);
  unit:add_item("h13",50);
  unit:add_item("h14",50);
  unit:add_item("h15",50);
  unit:add_item("h16",50);
  unit:add_item("h17",50);
  unit:add_item("h18",50);
  unit:add_item("h19",50);
  unit:add_item("h20",50);
end

function give_winner1()
  winner1 = get_unit(atoi36("3jic"))
  -- Horn, Statue, Akademie
  winner1:add_item("hornofdancing", 1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum Gewinn des Wettstreit der Künste, " .. winner1.name .. "."
  winner1.region:add_notice(s)
end

function give_winner2()
  winner2 = get_unit(atoi36("9d01"))
  -- Horn, Statue, Akademie
  winner2:add_item("instantartsculpture", 1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum zweiten Platz im Wettstreit der Künste, " .. winner2.name .. "."
  winner2.region:add_notice(s)
end

function give_winner3()
  winner3 = get_unit(atoi36("72a"))
  -- Horn, Statue, Akademie
  winner3:add_item("instantartacademy", 1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum dritten Platz im Wettstreit der Künste, " .. winner3.name .. "."
  winner3.region:add_notice(s)
end

function give_loser()
  loser = get_unit(atoi36("otna"))
  loser:add_item("bagpipeoffear", 1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum Gewinn des Nachwuchspreises im Wettstreit der Künste, " .. loser.name .. "."
  loser.region:add_notice(s)
end

function give_visitor1()
  visitor1 = get_unit(atoi36("zpok"))
  -- Kräutersammlung
  give_herbbag(visitor1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum Gewinn eines Besucherpreises im Wettstreit der Künste, " .. visitor1.name .. "."
  visitor1.region:add_notice(s)
end

function give_visitor2()
  visitor2 = get_unit(atoi36("aex"))
  -- Windgeist
  visitor2:add_item("trappedairelemental", 1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum Gewinn eines Besucherpreises im Wettstreit der Künste, " .. visitor2.name .. "."
  visitor2.region:add_notice(s)
end

function give_visitor3()
  visitor3 = get_unit(atoi36("visg"))
  -- Windgeist
  visitor3:add_item("trappedairelemental", 1)
  s = "von Bote des Rats von Andune (bote): Glückwunsch zum Gewinn eines Besucherpreises im Wettstreit der Künste, " .. visitor3.name .. "."
  visitor3.region:add_notice(s)
end

function give_rewards()
  print("- giving out rewards of the art contest")
  give_winner1()
  give_winner2()
  give_winner3()
  give_loser()
  give_visitor1()
  give_visitor2()
  give_visitor3()
end

give_rewards()

