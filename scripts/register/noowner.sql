select s.id ID, s.faction Partei, s.race Rasse, s.status Status, g.name Spiel
       from subscriptions s, games g
       where s.user=0 and g.id=s.game;
