select distinct u.id ID, s.faction Partei, left(concat(firstname,' ',lastname, ' <',email,'>'),43) Name, sum(t.balance) Kontostand
       from users u, transactions t, subscriptions s
       where u.id=t.user and u.id=s.user
			 and s.status='ACTIVE'
	   GROUP BY s.faction
	   HAVING Kontostand<2.5;
