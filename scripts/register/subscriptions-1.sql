select users.id, left(concat(firstname,' ',lastname, ' <',email,'>'),43) Name, subscriptions.faction Partei, races.name Rasse
       from users, games, subscriptions, races           
       where subscriptions.user=users.id 
       	     and games.id=subscriptions.game 
	     and subscriptions.race=races.race
	     and games.id=1
       order by subscriptions.id;
