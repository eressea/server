select count(users.status) Anzahl, subscriptions.status Status, games.name Spiel
       from users, games, subscriptions
       where games.id = subscriptions.game
       	     and users.id = subscriptions.user
       group by games.name, subscriptions.status
	   order by subscriptions.game;
