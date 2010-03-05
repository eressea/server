select races.name Rasse, count(*) Anmeldungen
       from races, subscriptions, users 
       where races.race=subscriptions.race and subscriptions.user=users.id
       group by races.race;

select games.name Spiel, count(*) Anmeldungen 
       from games, subscriptions, users
       where subscriptions.game=games.id and subscriptions.user=users.id
       group by game;

