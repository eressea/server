select distinct u.id ID, left(concat(firstname,' ',lastname, ' <',email,'>'),43) Name, sum(t.balance) Kontostand
       from users u, transactions t, subscriptions s
       where u.id=t.user and u.id=s.user
	   		 and u.status!='CONFIRMED' 
			 and s.status='ACTIVE'
	   GROUP BY u.id
	   HAVING SUM(t.balance)<2.5;
