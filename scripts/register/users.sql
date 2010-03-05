select users.id, users.email, users.firstname, users.lastname, users.email, games.name, races.name
       from users, games, subscriptions, races           
       where subscriptions.user=users.id 
       	     and games.id=subscriptions.game 
	     and subscriptions.race=races.race
       order by games.id;
