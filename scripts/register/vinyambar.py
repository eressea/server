#!/usr/bin/env python
import sys
import curses
import MySQLdb

Footer        = "Vinyambar Informationssystem"
locale        = "de"
dbname        = sys.argv[1]
db            = MySQLdb.connect(db=dbname)
customers     = {}
custid        = 0
stdscr        = curses.initscr()
height, width = stdscr.getmaxyx()
custinfo      = stdscr.subwin(10, width,  0, 0)
custdetail    = stdscr.subwin(10, width, 10, 0)

def refresh_customers():
    cursor=db.cursor()
    count=cursor.execute('SELECT id, firstname, lastname FROM users')
    while count>0:
	count=count-1
	cid, firstname, lastname = cursor.fetchone()
	customers[cid] = (firstname, lastname)

def show_detail():
    line = 1
    custdetail.erase()
    custdetail.border()
    custdetail.addstr(0, 2, "[ Kontoinformationen ]", curses.A_BOLD)
    cursor=db.cursor()
    count = cursor.execute('SELECT games.name, subscriptions.status, races.name FROM games, subscriptions, races WHERE subscriptions.user='+str(custid)+' and subscriptions.race=races.race and races.locale="'+locale+'" and subscriptions.game=games.id')
    while count>0:
	count = count-1
	game, status, race = cursor.fetchone()
	custdetail.addstr(line, 2, game+' - '+race+' - '+status)
	line=line+1
    count = cursor.execute('SELECT balance, description, date FROM transactions WHERE user='+str(custid))
    line=line+1
    while count>0:
	count = count-1
	balance, description, date = cursor.fetchone()
	custdetail.addstr(line, 2, str(date)[0:10]+' - '+description+' - '+str(balance)+' EUR')
	line=line+1
    custdetail.refresh()

def show_customer():
    cursor=db.cursor()
    custinfo.erase()
    custinfo.border()
    custinfo.addstr(0, 2, "[ Kundendaten ]", curses.A_BOLD)
    custinfo.addstr(1, 2, 'Kundennummer: '+str(custid))
    show_detail()
    count = cursor.execute('SELECT firstname, lastname, email, address, city, countries.name, phone, info, password, status FROM users, countries WHERE countries.id=users.country AND users.id='+str(custid))
    if (count!=0):
	firstname, lastname, email, addr, city, ccode, phone, info, passwd, status = cursor.fetchone()
	custinfo.addstr(2, 2, 'Name:         '+firstname+' '+lastname)
	custinfo.addstr(3, 2, 'Address:      '+addr)
	custinfo.addstr(4, 2, '              '+city+', '+ccode)
	custinfo.addstr(5, 2, 'Phone:        '+phone)
	custinfo.addstr(6, 2, 'Password:     '+passwd)
	if info!=None: custinfo.addstr(8, 2, str(info))
    custinfo.refresh()

def search():
    global Footer, custid
    stdscr.addstr(height-1, 0, '/')
    stdscr.clrtoeol()
    curses.echo()
    s = stdscr.getstr()
    curses.noecho()
    refresh_customers()
    try:
	custid = int(s)
    except:
	curses.beep()
	Footer='Customer #'+s+' was not found'
	
    if customers.has_key(custid):
	show_customer()
    else:
	curses.beep()
	Footer='Customer #'+s+' was not found'

	
def next_customer():
    global custid
    custid=custid+1
    show_customer()
    
def prev_customer():
    global custid
    if custid>0:
	custid=custid-1
    show_customer()
    
def main():
    global Footer
    branch = { 'q' : None, '/' : search, '+' : next_customer, '-' : prev_customer }
    while 1:
	stdscr.addstr(height-1, 0, Footer)
	stdscr.clrtoeol()
	stdscr.refresh()
	key=stdscr.getch()
	Footer=':'
	if (key<256) & (key>=0):
	    c = chr(key)
	    if branch.has_key(c):
		fun=branch[c]
		if (fun==None):
		    break
		else:
		    fun()
	    else:
		Footer='Unknown keycode '+curses.keyname(key)
		curses.beep()

stdscr.keypad(1)
curses.noecho()
curses.cbreak()

try:
    show_customer()
    main()
finally:
    stdscr.keypad(0)
    curses.echo()
    curses.nocbreak()
    curses.endwin()
