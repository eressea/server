select u.id ID, left(concat(firstname,' ',lastname, ' <',email,'>'),43) Name, sum(t.balance) Kontostand
       from users u, transactions t
       where u.id=t.user
	   		 and u.status!='CONFIRMED'
	   GROUP BY u.id
	   HAVING SUM(t.balance)<2.5;

select count(users.status) Anzahl, users.status Status, games.name Spiel
       from users, games, subscriptions
       where games.id = subscriptions.game
       	     and users.id = subscriptions.user
       group by games.name, users.status
	   order by subscriptions.game;
