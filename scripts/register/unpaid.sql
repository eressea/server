select u.id ID, left(concat(firstname,' ',lastname, ' <',email,'>'),43) Name, sum(t.balance) Kontostand
       from users u, transactions t
       where u.id=t.user
	   		 and u.status!='CONFIRMED'
	   GROUP BY u.id
	   HAVING SUM(t.balance)<2.5;
