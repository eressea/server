import MySQLdb;
db=MySQLdb.connect(db='vinyambar');     
c=db.cursor()
users=c.execute('select id from users')
while users>0:
    users=users-1
    c2=db.cursor()
    user=c.fetchone()[0]
    a=c2.execute('select id from transactions where user='+str(int(user)))
    if a>0:
	c2.execute("update users set status='PAYING' where id="+str(int(user)))
    else:
	c2.execute("update users set status='CONFIRMED' where id="+str(int(user)))

