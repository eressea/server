select distinct u.email, s.race, u.locale from users u, subscriptions s left
join userips i on u.id=i.user left join bannedips b on i.ip=b.ip where
s.user=u.id and b.ip is NULL and u.status='CONFIRMED' order by u.id;
