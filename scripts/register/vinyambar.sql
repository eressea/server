# MySQL dump 8.13
#
# Host: localhost    Database: vinyambar
#--------------------------------------------------------
# Server version	3.23.36-log

#
# Table structure for table 'countries'
#

CREATE TABLE countries (
  id int(11) NOT NULL default '0',
  name varchar(32) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

#
# Dumping data for table 'countries'
#

INSERT INTO countries VALUES (1,'United States');
INSERT INTO countries VALUES (10,'Anguilla');
INSERT INTO countries VALUES (100,'Israel');
INSERT INTO countries VALUES (101,'Italy');
INSERT INTO countries VALUES (102,'Jamaica');
INSERT INTO countries VALUES (103,'Jan Mayen');
INSERT INTO countries VALUES (104,'Japan');
INSERT INTO countries VALUES (105,'Jersey');
INSERT INTO countries VALUES (106,'Jordan');
INSERT INTO countries VALUES (107,'Kazakhstan');
INSERT INTO countries VALUES (108,'Kenya Coast Republic');
INSERT INTO countries VALUES (109,'Kiribati');
INSERT INTO countries VALUES (11,'Antigua and Barbuda');
INSERT INTO countries VALUES (111,'Korea, South');
INSERT INTO countries VALUES (112,'Kuwait');
INSERT INTO countries VALUES (113,'Kyrgyzstan');
INSERT INTO countries VALUES (114,'Laos');
INSERT INTO countries VALUES (115,'Latvia');
INSERT INTO countries VALUES (116,'Lebanon');
INSERT INTO countries VALUES (117,'Lesotho');
INSERT INTO countries VALUES (118,'Liberia');
INSERT INTO countries VALUES (12,'Argentina');
INSERT INTO countries VALUES (120,'Liechtenstein');
INSERT INTO countries VALUES (121,'Lithuania');
INSERT INTO countries VALUES (122,'Luxembourg');
INSERT INTO countries VALUES (123,'Macau');
INSERT INTO countries VALUES (124,'Macedonia');
INSERT INTO countries VALUES (125,'Madagascar');
INSERT INTO countries VALUES (126,'Malawi');
INSERT INTO countries VALUES (127,'Malaysia');
INSERT INTO countries VALUES (128,'Maldives');
INSERT INTO countries VALUES (129,'Mali');
INSERT INTO countries VALUES (13,'Armenia');
INSERT INTO countries VALUES (130,'Malta');
INSERT INTO countries VALUES (131,'Marshall Islands');
INSERT INTO countries VALUES (132,'Martinique');
INSERT INTO countries VALUES (133,'Mauritania');
INSERT INTO countries VALUES (134,'Mauritius');
INSERT INTO countries VALUES (135,'Mayotte');
INSERT INTO countries VALUES (136,'Mexico');
INSERT INTO countries VALUES (137,'Moldova');
INSERT INTO countries VALUES (138,'Monaco');
INSERT INTO countries VALUES (139,'Mongolia');
INSERT INTO countries VALUES (14,'Aruba');
INSERT INTO countries VALUES (140,'Montserrat');
INSERT INTO countries VALUES (141,'Morocco');
INSERT INTO countries VALUES (142,'Mozambique');
INSERT INTO countries VALUES (143,'Namibia');
INSERT INTO countries VALUES (144,'Nauru');
INSERT INTO countries VALUES (145,'Nepal');
INSERT INTO countries VALUES (146,'Netherlands');
INSERT INTO countries VALUES (147,'Netherlands Antilles');
INSERT INTO countries VALUES (148,'New Caledonia');
INSERT INTO countries VALUES (149,'New Zealand');
INSERT INTO countries VALUES (15,'Australia');
INSERT INTO countries VALUES (150,'Nicaragua');
INSERT INTO countries VALUES (151,'Niger');
INSERT INTO countries VALUES (152,'Nigeria');
INSERT INTO countries VALUES (153,'Niue');
INSERT INTO countries VALUES (154,'Norway');
INSERT INTO countries VALUES (155,'Oman');
INSERT INTO countries VALUES (156,'Pakistan');
INSERT INTO countries VALUES (157,'Palau');
INSERT INTO countries VALUES (158,'Panama');
INSERT INTO countries VALUES (159,'Papua New Guinea');
INSERT INTO countries VALUES (16,'Austria');
INSERT INTO countries VALUES (160,'Paraguay');
INSERT INTO countries VALUES (161,'Peru');
INSERT INTO countries VALUES (162,'Philippines');
INSERT INTO countries VALUES (163,'Poland');
INSERT INTO countries VALUES (164,'Portugal');
INSERT INTO countries VALUES (165,'Puerto Rico');
INSERT INTO countries VALUES (166,'Qatar');
INSERT INTO countries VALUES (167,'Romania');
INSERT INTO countries VALUES (168,'Russian Federation');
INSERT INTO countries VALUES (169,'Rwanda');
INSERT INTO countries VALUES (17,'Azerbaijan Republic');
INSERT INTO countries VALUES (170,'Saint Helena');
INSERT INTO countries VALUES (171,'Saint Kitts-Nevis');
INSERT INTO countries VALUES (172,'Saint Lucia');
INSERT INTO countries VALUES (173,'Saint Pierre and Miquelon');
INSERT INTO countries VALUES (174,'Saint Vincent and the Grenadines');
INSERT INTO countries VALUES (175,'San Marino');
INSERT INTO countries VALUES (176,'Saudi Arabia');
INSERT INTO countries VALUES (177,'Senegal');
INSERT INTO countries VALUES (178,'Seychelles');
INSERT INTO countries VALUES (179,'Sierra Leone');
INSERT INTO countries VALUES (18,'Bahamas');
INSERT INTO countries VALUES (180,'Singapore');
INSERT INTO countries VALUES (181,'Slovakia');
INSERT INTO countries VALUES (182,'Slovenia');
INSERT INTO countries VALUES (183,'Solomon Islands');
INSERT INTO countries VALUES (184,'Somalia');
INSERT INTO countries VALUES (185,'South Africa');
INSERT INTO countries VALUES (186,'Spain');
INSERT INTO countries VALUES (187,'Sri Lanka');
INSERT INTO countries VALUES (188,'Sudan');
INSERT INTO countries VALUES (189,'Suriname');
INSERT INTO countries VALUES (19,'Bahrain');
INSERT INTO countries VALUES (190,'Svalbard');
INSERT INTO countries VALUES (191,'Swaziland');
INSERT INTO countries VALUES (192,'Sweden');
INSERT INTO countries VALUES (193,'Switzerland');
INSERT INTO countries VALUES (194,'Syria');
INSERT INTO countries VALUES (195,'Tahiti');
INSERT INTO countries VALUES (196,'Taiwan');
INSERT INTO countries VALUES (197,'Tajikistan');
INSERT INTO countries VALUES (198,'Tanzania');
INSERT INTO countries VALUES (199,'Thailand');
INSERT INTO countries VALUES (2,'Canada');
INSERT INTO countries VALUES (20,'Bangladesh');
INSERT INTO countries VALUES (200,'Togo');
INSERT INTO countries VALUES (201,'Tonga');
INSERT INTO countries VALUES (202,'Trinidad and Tobago');
INSERT INTO countries VALUES (203,'Tunisia');
INSERT INTO countries VALUES (204,'Turkey');
INSERT INTO countries VALUES (205,'Turkmenistan');
INSERT INTO countries VALUES (206,'Turks and Caicos Islands');
INSERT INTO countries VALUES (207,'Tuvalu');
INSERT INTO countries VALUES (208,'Uganda');
INSERT INTO countries VALUES (209,'Ukraine');
INSERT INTO countries VALUES (21,'Barbados');
INSERT INTO countries VALUES (210,'United Arab Emirates');
INSERT INTO countries VALUES (211,'Uruguay');
INSERT INTO countries VALUES (212,'Uzbekistan');
INSERT INTO countries VALUES (213,'Vanuatu');
INSERT INTO countries VALUES (214,'Vatican City State');
INSERT INTO countries VALUES (215,'Venezuela');
INSERT INTO countries VALUES (216,'Vietnam');
INSERT INTO countries VALUES (217,'Virgin Islands (U.S.)');
INSERT INTO countries VALUES (218,'Wallis and Futuna');
INSERT INTO countries VALUES (219,'Western Sahara');
INSERT INTO countries VALUES (22,'Belarus');
INSERT INTO countries VALUES (220,'Western Samoa');
INSERT INTO countries VALUES (221,'Yemen');
INSERT INTO countries VALUES (222,'Yugoslavia');
INSERT INTO countries VALUES (223,'Zambia');
INSERT INTO countries VALUES (224,'Zimbabwe</SELECT>');
INSERT INTO countries VALUES (225,'APO/FPO');
INSERT INTO countries VALUES (226,'Micronesia');
INSERT INTO countries VALUES (23,'Belgium');
INSERT INTO countries VALUES (24,'Belize');
INSERT INTO countries VALUES (25,'Benin');
INSERT INTO countries VALUES (26,'Bermuda');
INSERT INTO countries VALUES (27,'Bhutan');
INSERT INTO countries VALUES (28,'Bolivia');
INSERT INTO countries VALUES (29,'Bosnia and Herzegovina');
INSERT INTO countries VALUES (3,'United Kingdom');
INSERT INTO countries VALUES (30,'Botswana');
INSERT INTO countries VALUES (31,'Brazil');
INSERT INTO countries VALUES (32,'British Virgin Islands');
INSERT INTO countries VALUES (33,'Brunei Darussalam');
INSERT INTO countries VALUES (34,'Bulgaria');
INSERT INTO countries VALUES (35,'Burkina Faso');
INSERT INTO countries VALUES (36,'Burma');
INSERT INTO countries VALUES (37,'Burundi');
INSERT INTO countries VALUES (38,'Cambodia');
INSERT INTO countries VALUES (39,'Cameroon');
INSERT INTO countries VALUES (4,'Afghanistan');
INSERT INTO countries VALUES (40,'Cape Verde Islands');
INSERT INTO countries VALUES (41,'Cayman Islands');
INSERT INTO countries VALUES (42,'Central African Republic');
INSERT INTO countries VALUES (43,'Chad');
INSERT INTO countries VALUES (44,'Chile');
INSERT INTO countries VALUES (45,'China');
INSERT INTO countries VALUES (46,'Colombia');
INSERT INTO countries VALUES (47,'Comoros');
INSERT INTO countries VALUES (48,'Congo, Democratic Republic of th');
INSERT INTO countries VALUES (49,'Congo, Republic of the');
INSERT INTO countries VALUES (5,'Albania');
INSERT INTO countries VALUES (50,'Cook Islands');
INSERT INTO countries VALUES (51,'Costa Rica');
INSERT INTO countries VALUES (52,'Cote d Ivoire (Ivory Coast)');
INSERT INTO countries VALUES (53,'Croatia, Republic of');
INSERT INTO countries VALUES (55,'Cyprus');
INSERT INTO countries VALUES (56,'Czech Republic');
INSERT INTO countries VALUES (57,'Denmark');
INSERT INTO countries VALUES (58,'Djibouti');
INSERT INTO countries VALUES (59,'Dominica');
INSERT INTO countries VALUES (6,'Algeria');
INSERT INTO countries VALUES (60,'Dominican Republic');
INSERT INTO countries VALUES (61,'Ecuador');
INSERT INTO countries VALUES (62,'Egypt');
INSERT INTO countries VALUES (63,'El Salvador');
INSERT INTO countries VALUES (64,'Equatorial Guinea');
INSERT INTO countries VALUES (65,'Eritrea');
INSERT INTO countries VALUES (66,'Estonia');
INSERT INTO countries VALUES (67,'Ethiopia');
INSERT INTO countries VALUES (68,'Falkland Islands (Islas Malvinas');
INSERT INTO countries VALUES (69,'Fiji');
INSERT INTO countries VALUES (7,'American Samoa');
INSERT INTO countries VALUES (70,'Finland');
INSERT INTO countries VALUES (71,'France');
INSERT INTO countries VALUES (72,'French Guiana');
INSERT INTO countries VALUES (73,'French Polynesia');
INSERT INTO countries VALUES (74,'Gabon Republic');
INSERT INTO countries VALUES (75,'Gambia');
INSERT INTO countries VALUES (76,'Georgia');
INSERT INTO countries VALUES (77,'Germany');
INSERT INTO countries VALUES (78,'Ghana');
INSERT INTO countries VALUES (79,'Gibraltar');
INSERT INTO countries VALUES (8,'Andorra');
INSERT INTO countries VALUES (80,'Greece');
INSERT INTO countries VALUES (81,'Greenland');
INSERT INTO countries VALUES (82,'Grenada');
INSERT INTO countries VALUES (83,'Guadeloupe');
INSERT INTO countries VALUES (84,'Guam');
INSERT INTO countries VALUES (85,'Guatemala');
INSERT INTO countries VALUES (86,'Guernsey');
INSERT INTO countries VALUES (87,'Guinea');
INSERT INTO countries VALUES (88,'Guinea-Bissau');
INSERT INTO countries VALUES (89,'Guyana');
INSERT INTO countries VALUES (9,'Angola');
INSERT INTO countries VALUES (90,'Haiti');
INSERT INTO countries VALUES (91,'Honduras');
INSERT INTO countries VALUES (92,'Hong Kong');
INSERT INTO countries VALUES (93,'Hungary');
INSERT INTO countries VALUES (94,'Iceland');
INSERT INTO countries VALUES (95,'India');
INSERT INTO countries VALUES (96,'Indonesia');
INSERT INTO countries VALUES (99,'Ireland');

#
# Table structure for table 'dual'
#

CREATE TABLE dual (
  dual char(1) default NULL
) TYPE=MyISAM;

#
# Dumping data for table 'dual'
#

INSERT INTO dual VALUES ('0');

#
# Table structure for table 'factions'
#

CREATE TABLE factions (
  id varchar(6) NOT NULL default '',
  game int(11) NOT NULL default '0',
  email varchar(64) default NULL,
  banner text,
  vacation varchar(64) default NULL,
  password varchar(64) default NULL,
  name varchar(64) default NULL,
  user int(11) NOT NULL default '0',
  vacation_start date default NULL,
  race varchar(16) default NULL,
  locale varchar(10) default NULL,
  lastorders int(11) default NULL,
  PRIMARY KEY  (id,game)
) TYPE=MyISAM;

#
# Dumping data for table 'factions'
#


#
# Table structure for table 'games'
#

CREATE TABLE games (
  id int(11) NOT NULL auto_increment,
  name varchar(32) NOT NULL default '',
  info text,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

#
# Dumping data for table 'games'
#

INSERT INTO games VALUES (1,'Vinyambar I','Vinyambar nach alten Regeln');
INSERT INTO games VALUES (2,'Vinyambar II','Vinyambar nach neuen Regeln');
INSERT INTO games VALUES (3,'Warteliste','Interessenten für neue Regeln');

#
# Table structure for table 'races'
#

CREATE TABLE races (
  locale varchar(10) NOT NULL default '',
  race varchar(10) NOT NULL default '',
  name varchar(20) default NULL
) TYPE=MyISAM;

#
# Dumping data for table 'races'
#

INSERT INTO races VALUES ('de','GOBLIN','Goblins');
INSERT INTO races VALUES ('de','DWARF','Zwerge');
INSERT INTO races VALUES ('de','ELF','Elfen');
INSERT INTO races VALUES ('de','HALFLING','Halblinge');
INSERT INTO races VALUES ('de','INSECT','Insekten');
INSERT INTO races VALUES ('de','AQUARIAN','Meermenschen');
INSERT INTO races VALUES ('de','HUMAN','Menschen');
INSERT INTO races VALUES ('de','CAT','Katzen');
INSERT INTO races VALUES ('de','TROLL','Trolle');
INSERT INTO races VALUES ('de','ORC','Orks');
INSERT INTO races VALUES ('de','DEMON','Dämonen');

#
# Table structure for table 'subscriptions'
#

CREATE TABLE subscriptions (
  game int(11) NOT NULL default '0',
  user int(11) NOT NULL default '0',
  race varchar(10) default NULL,
  id int(10) NOT NULL auto_increment,
  status varchar(10) NOT NULL default 'NEW',
  updated timestamp(14) NOT NULL,
  credits int(11) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

#
# Dumping data for table 'subscriptions'
#

INSERT INTO subscriptions VALUES (1,4,'ELF',4,'CONFIRMED',20011106230004,0);
INSERT INTO subscriptions VALUES (1,2,'HUMAN',2,'CONFIRMED',20011106224055,0);
INSERT INTO subscriptions VALUES (1,3,'DWARF',3,'CONFIRMED',20011106224055,0);
INSERT INTO subscriptions VALUES (1,5,'HALFLING',5,'CONFIRMED',20011106230004,0);
INSERT INTO subscriptions VALUES (1,6,'DWARF',6,'CONFIRMED',20011106231004,0);
INSERT INTO subscriptions VALUES (1,7,'ELF',7,'CONFIRMED',20011106231504,0);
INSERT INTO subscriptions VALUES (1,8,'TROLL',8,'CONFIRMED',20011106232003,0);
INSERT INTO subscriptions VALUES (1,9,'DWARF',9,'CONFIRMED',20011106233004,0);
INSERT INTO subscriptions VALUES (1,10,'ELF',10,'CONFIRMED',20011106233005,0);
INSERT INTO subscriptions VALUES (1,11,'DWARF',11,'CONFIRMED',20011106233005,0);
INSERT INTO subscriptions VALUES (2,12,'GOBLIN',12,'CONFIRMED',20011106233505,0);
INSERT INTO subscriptions VALUES (1,68,'DWARF',78,'CONFIRMED',20011108215504,0);
INSERT INTO subscriptions VALUES (2,67,'TROLL',77,'CONFIRMED',20011108201004,0);
INSERT INTO subscriptions VALUES (1,14,'CAT',15,'CONFIRMED',20011107000503,0);
INSERT INTO subscriptions VALUES (1,15,'ELF',16,'CONFIRMED',20011107002509,0);
INSERT INTO subscriptions VALUES (2,15,'HALFLING',17,'CONFIRMED',20011107013618,0);
INSERT INTO subscriptions VALUES (2,18,'AQUARIAN',18,'CONFIRMED',20011107085504,0);
INSERT INTO subscriptions VALUES (1,19,'DEMON',19,'CONFIRMED',20011107085504,0);
INSERT INTO subscriptions VALUES (2,19,'AQUARIAN',20,'CONFIRMED',20011107085504,0);
INSERT INTO subscriptions VALUES (2,20,'ELF',21,'CONFIRMED',20011107085504,0);
INSERT INTO subscriptions VALUES (2,21,'CAT',22,'CONFIRMED',20011111220350,0);
INSERT INTO subscriptions VALUES (1,22,'HALFLING',23,'CONFIRMED',20011107095504,0);
INSERT INTO subscriptions VALUES (2,23,'DEMON',24,'CONFIRMED',20011107105009,0);
INSERT INTO subscriptions VALUES (2,24,'AQUARIAN',25,'CONFIRMED',20011107105504,0);
INSERT INTO subscriptions VALUES (2,25,'DWARF',26,'CONFIRMED',20011107110503,0);
INSERT INTO subscriptions VALUES (2,26,'AQUARIAN',27,'CONFIRMED',20011107114004,0);
INSERT INTO subscriptions VALUES (1,27,'ELF',28,'CONFIRMED',20011107120503,0);
INSERT INTO subscriptions VALUES (2,27,'CAT',29,'CONFIRMED',20011107120503,0);
INSERT INTO subscriptions VALUES (1,28,'HUMAN',30,'CONFIRMED',20011107121506,0);
INSERT INTO subscriptions VALUES (1,29,'AQUARIAN',31,'CONFIRMED',20011107122004,0);
INSERT INTO subscriptions VALUES (2,29,'AQUARIAN',32,'CONFIRMED',20011107122004,0);
INSERT INTO subscriptions VALUES (2,30,'HALFLING',33,'CONFIRMED',20011107123504,0);
INSERT INTO subscriptions VALUES (1,31,'TROLL',34,'CONFIRMED',20011107135004,0);
INSERT INTO subscriptions VALUES (2,32,'TROLL',35,'CONFIRMED',20011107143508,0);
INSERT INTO subscriptions VALUES (2,33,'DEMON',36,'CONFIRMED',20011107152006,0);
INSERT INTO subscriptions VALUES (2,34,'ELF',37,'CONFIRMED',20011107154504,0);
INSERT INTO subscriptions VALUES (2,35,'DWARF',38,'CONFIRMED',20011107154504,0);
INSERT INTO subscriptions VALUES (2,36,'AQUARIAN',39,'CONFIRMED',20011107160504,0);
INSERT INTO subscriptions VALUES (2,37,'GOBLIN',40,'CONFIRMED',20011107161008,0);
INSERT INTO subscriptions VALUES (2,38,'CAT',41,'CONFIRMED',20011107163023,0);
INSERT INTO subscriptions VALUES (2,39,'HUMAN',42,'CONFIRMED',20011107164505,0);
INSERT INTO subscriptions VALUES (2,40,'ORC',43,'CONFIRMED',20011107171004,0);
INSERT INTO subscriptions VALUES (2,41,'DWARF',44,'CONFIRMED',20011107173004,0);
INSERT INTO subscriptions VALUES (2,42,'DEMON',45,'CONFIRMED',20011107184004,0);
INSERT INTO subscriptions VALUES (2,43,'INSECT',46,'CONFIRMED',20011107190504,0);
INSERT INTO subscriptions VALUES (2,44,'AQUARIAN',47,'CONFIRMED',20011107200004,0);
INSERT INTO subscriptions VALUES (1,45,'DWARF',48,'CONFIRMED',20011107201504,0);
INSERT INTO subscriptions VALUES (2,45,'AQUARIAN',49,'CONFIRMED',20011107201504,0);
INSERT INTO subscriptions VALUES (2,46,'INSECT',50,'CONFIRMED',20011107202504,0);
INSERT INTO subscriptions VALUES (2,47,'CAT',51,'CONFIRMED',20011107203004,0);
INSERT INTO subscriptions VALUES (1,48,'TROLL',52,'CONFIRMED',20011107203508,0);
INSERT INTO subscriptions VALUES (2,48,'DWARF',53,'CONFIRMED',20011107203509,0);
INSERT INTO subscriptions VALUES (1,49,'ELF',54,'CONFIRMED',20011107213503,0);
INSERT INTO subscriptions VALUES (2,50,'HUMAN',55,'CONFIRMED',20011107222003,0);
INSERT INTO subscriptions VALUES (2,51,'AQUARIAN',56,'CONFIRMED',20011107223504,0);
INSERT INTO subscriptions VALUES (1,52,'TROLL',57,'CONFIRMED',20011107223504,0);
INSERT INTO subscriptions VALUES (2,52,'HUMAN',58,'CONFIRMED',20011107223504,0);
INSERT INTO subscriptions VALUES (1,53,'HUMAN',59,'CONFIRMED',20011107223504,0);
INSERT INTO subscriptions VALUES (2,53,'HUMAN',60,'CONFIRMED',20011107223505,0);
INSERT INTO subscriptions VALUES (1,54,'DWARF',61,'CONFIRMED',20011107234505,0);
INSERT INTO subscriptions VALUES (1,55,'HUMAN',62,'CONFIRMED',20011108001003,0);
INSERT INTO subscriptions VALUES (2,56,'TROLL',63,'CONFIRMED',20011108023507,0);
INSERT INTO subscriptions VALUES (1,57,'AQUARIAN',64,'CONFIRMED',20011108095504,0);
INSERT INTO subscriptions VALUES (2,58,'HUMAN',65,'CONFIRMED',20011108124503,0);
INSERT INTO subscriptions VALUES (2,59,'DWARF',66,'CONFIRMED',20011108153006,0);
INSERT INTO subscriptions VALUES (2,60,'DWARF',67,'CONFIRMED',20011108154504,0);
INSERT INTO subscriptions VALUES (2,61,'INSECT',68,'CONFIRMED',20011108165505,0);
INSERT INTO subscriptions VALUES (1,62,'HALFLING',69,'CONFIRMED',20011108183504,0);
INSERT INTO subscriptions VALUES (2,62,'ELF',70,'CONFIRMED',20011108183504,0);
INSERT INTO subscriptions VALUES (1,63,'DEMON',71,'CONFIRMED',20011108183504,0);
INSERT INTO subscriptions VALUES (2,63,'ELF',72,'CONFIRMED',20011108183504,0);
INSERT INTO subscriptions VALUES (1,64,'INSECT',73,'CONFIRMED',20011108185004,0);
INSERT INTO subscriptions VALUES (2,65,'DWARF',74,'CONFIRMED',20011108192503,0);
INSERT INTO subscriptions VALUES (1,66,'DWARF',75,'CONFIRMED',20011108195504,0);
INSERT INTO subscriptions VALUES (2,66,'ELF',76,'CONFIRMED',20011108195505,0);
INSERT INTO subscriptions VALUES (2,69,'INSECT',79,'CONFIRMED',20011108220003,0);
INSERT INTO subscriptions VALUES (2,70,'HALFLING',80,'CONFIRMED',20011108222503,0);
INSERT INTO subscriptions VALUES (1,71,'DWARF',81,'CONFIRMED',20011108224013,0);
INSERT INTO subscriptions VALUES (1,72,'GOBLIN',82,'CONFIRMED',20011108224503,0);
INSERT INTO subscriptions VALUES (2,72,'DWARF',83,'CONFIRMED',20011108224503,0);
INSERT INTO subscriptions VALUES (1,73,'TROLL',84,'CONFIRMED',20011108225004,0);
INSERT INTO subscriptions VALUES (1,74,'HUMAN',85,'CONFIRMED',20011109070003,0);
INSERT INTO subscriptions VALUES (2,74,'HUMAN',86,'CONFIRMED',20011109070004,0);
INSERT INTO subscriptions VALUES (2,75,'DWARF',87,'CONFIRMED',20011109094004,0);
INSERT INTO subscriptions VALUES (2,76,'ELF',88,'CONFIRMED',20011109094504,0);
INSERT INTO subscriptions VALUES (1,77,'ELF',89,'CONFIRMED',20011109094504,0);
INSERT INTO subscriptions VALUES (2,78,'DWARF',90,'CONFIRMED',20011109103504,0);
INSERT INTO subscriptions VALUES (2,79,'ORC',91,'CONFIRMED',20011109105004,0);
INSERT INTO subscriptions VALUES (2,80,'DEMON',92,'CONFIRMED',20011109121504,0);
INSERT INTO subscriptions VALUES (2,81,'INSECT',93,'CONFIRMED',20011109131003,0);
INSERT INTO subscriptions VALUES (2,82,'DEMON',94,'CONFIRMED',20011109144004,0);
INSERT INTO subscriptions VALUES (2,83,'CAT',95,'CONFIRMED',20011109145004,0);
INSERT INTO subscriptions VALUES (2,84,'AQUARIAN',96,'CONFIRMED',20011109190003,0);
INSERT INTO subscriptions VALUES (2,85,'GOBLIN',97,'CONFIRMED',20011109210506,0);
INSERT INTO subscriptions VALUES (1,86,'GOBLIN',98,'CONFIRMED',20011109215004,0);
INSERT INTO subscriptions VALUES (2,87,'ELF',99,'CONFIRMED',20011110115504,0);
INSERT INTO subscriptions VALUES (2,88,'AQUARIAN',100,'CONFIRMED',20011110121004,0);
INSERT INTO subscriptions VALUES (2,89,'TROLL',101,'CONFIRMED',20011110130504,0);
INSERT INTO subscriptions VALUES (1,90,'DWARF',102,'CONFIRMED',20011110142021,0);
INSERT INTO subscriptions VALUES (2,90,'DWARF',103,'CONFIRMED',20011110142021,0);
INSERT INTO subscriptions VALUES (2,91,'AQUARIAN',104,'CONFIRMED',20011110142504,0);
INSERT INTO subscriptions VALUES (1,93,'GOBLIN',106,'CONFIRMED',20011110152005,0);
INSERT INTO subscriptions VALUES (1,94,'HALFLING',107,'CONFIRMED',20011110152005,0);
INSERT INTO subscriptions VALUES (2,95,'HALFLING',108,'CONFIRMED',20011110155005,0);
INSERT INTO subscriptions VALUES (2,96,'ELF',109,'CONFIRMED',20011110160003,0);
INSERT INTO subscriptions VALUES (1,97,'DEMON',110,'CONFIRMED',20011110180504,0);
INSERT INTO subscriptions VALUES (2,97,'HALFLING',111,'CONFIRMED',20011110180504,0);
INSERT INTO subscriptions VALUES (1,98,'ORC',112,'CONFIRMED',20011110190508,0);
INSERT INTO subscriptions VALUES (2,99,'AQUARIAN',113,'CONFIRMED',20011110201003,0);
INSERT INTO subscriptions VALUES (1,100,'ELF',114,'CONFIRMED',20011110202005,0);
INSERT INTO subscriptions VALUES (2,101,'HUMAN',115,'CONFIRMED',20011110204505,0);
INSERT INTO subscriptions VALUES (2,102,'DEMON',116,'CONFIRMED',20011111111504,0);
INSERT INTO subscriptions VALUES (2,103,'DWARF',117,'CONFIRMED',20011111113004,0);
INSERT INTO subscriptions VALUES (1,104,'ELF',118,'CONFIRMED',20011111140003,0);
INSERT INTO subscriptions VALUES (1,105,'TROLL',119,'CONFIRMED',20011111141504,0);
INSERT INTO subscriptions VALUES (2,106,'DEMON',120,'CONFIRMED',20011111144505,0);
INSERT INTO subscriptions VALUES (2,107,'ELF',121,'CONFIRMED',20011111161507,0);
INSERT INTO subscriptions VALUES (2,108,'AQUARIAN',122,'CONFIRMED',20011111162004,0);
INSERT INTO subscriptions VALUES (2,109,'INSECT',123,'CONFIRMED',20011111163004,0);
INSERT INTO subscriptions VALUES (1,110,'INSECT',124,'CONFIRMED',20011111164510,0);
INSERT INTO subscriptions VALUES (1,111,'HALFLING',125,'CONFIRMED',20011111185004,0);
INSERT INTO subscriptions VALUES (2,111,'DEMON',126,'CONFIRMED',20011111185005,0);
INSERT INTO subscriptions VALUES (2,112,'AQUARIAN',127,'CONFIRMED',20011111195004,0);
INSERT INTO subscriptions VALUES (1,114,'ELF',129,'CONFIRMED',20011111202506,0);
INSERT INTO subscriptions VALUES (2,115,'DWARF',130,'CONFIRMED',20011111214505,0);
INSERT INTO subscriptions VALUES (1,116,'TROLL',131,'CONFIRMED',20011111225505,0);
INSERT INTO subscriptions VALUES (2,116,'HALFLING',132,'CONFIRMED',20011111225506,0);
INSERT INTO subscriptions VALUES (2,117,'DEMON',133,'CONFIRMED',20011112063020,0);
INSERT INTO subscriptions VALUES (2,118,'TROLL',134,'CONFIRMED',20011112082005,0);
INSERT INTO subscriptions VALUES (2,119,'HUMAN',135,'CONFIRMED',20011112101504,0);
INSERT INTO subscriptions VALUES (2,120,'HUMAN',136,'CONFIRMED',20011112103004,0);
INSERT INTO subscriptions VALUES (1,121,'AQUARIAN',137,'CONFIRMED',20011112103004,0);
INSERT INTO subscriptions VALUES (2,121,'GOBLIN',138,'CONFIRMED',20011114122718,0);
INSERT INTO subscriptions VALUES (2,122,'DWARF',139,'CONFIRMED',20011112103004,0);
INSERT INTO subscriptions VALUES (1,123,'DEMON',140,'CONFIRMED',20011112121504,0);
INSERT INTO subscriptions VALUES (2,124,'AQUARIAN',141,'CONFIRMED',20011112141504,0);
INSERT INTO subscriptions VALUES (2,125,'INSECT',142,'CONFIRMED',20011112143003,0);
INSERT INTO subscriptions VALUES (1,126,'INSECT',143,'CONFIRMED',20011112155004,0);
INSERT INTO subscriptions VALUES (1,127,'HUMAN',144,'CONFIRMED',20011112170004,0);
INSERT INTO subscriptions VALUES (2,128,'GOBLIN',145,'CONFIRMED',20011112172503,0);
INSERT INTO subscriptions VALUES (2,129,'INSECT',146,'CONFIRMED',20011112184504,0);
INSERT INTO subscriptions VALUES (1,130,'HALFLING',147,'CONFIRMED',20011112204505,0);
INSERT INTO subscriptions VALUES (2,130,'ELF',148,'CONFIRMED',20011112204505,0);
INSERT INTO subscriptions VALUES (2,131,'TROLL',149,'CONFIRMED',20011112225503,0);
INSERT INTO subscriptions VALUES (1,132,'ELF',150,'CONFIRMED',20011113010003,0);
INSERT INTO subscriptions VALUES (1,133,'ELF',151,'CONFIRMED',20011113013503,0);
INSERT INTO subscriptions VALUES (2,134,'CAT',152,'CONFIRMED',20011113085504,0);
INSERT INTO subscriptions VALUES (2,135,'ELF',153,'CONFIRMED',20011113145505,0);
INSERT INTO subscriptions VALUES (1,136,'ELF',154,'CONFIRMED',20011113151504,0);
INSERT INTO subscriptions VALUES (2,137,'GOBLIN',155,'CONFIRMED',20011113230507,0);
INSERT INTO subscriptions VALUES (2,138,'DWARF',156,'CONFIRMED',20011114002504,0);
INSERT INTO subscriptions VALUES (2,139,'GOBLIN',157,'CONFIRMED',20011114110503,0);
INSERT INTO subscriptions VALUES (2,140,'DEMON',158,'CONFIRMED',20011114112004,0);
INSERT INTO subscriptions VALUES (3,141,'DWARF',159,'WAITING',20011114162334,0);
INSERT INTO subscriptions VALUES (1,142,'INSECT',160,'CONFIRMED',20011116222017,0);
INSERT INTO subscriptions VALUES (3,143,'HALFLING',161,'WAITING',20011118204013,0);

#
# Table structure for table 'users'
#

CREATE TABLE users (
  id int(11) NOT NULL auto_increment,
  email varchar(64) default NULL,
  info text,
  address varchar(28) default NULL,
  city varchar(28) default NULL,
  country int(11) NOT NULL default '0',
  phone varchar(32) default NULL,
  firstname varchar(32) default NULL,
  lastname varchar(32) default NULL,
  password varchar(16) NOT NULL default '',
  updated timestamp(14) NOT NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

#
# Dumping data for table 'users'
#

INSERT INTO users VALUES (4,'stemu@netcologne.de',NULL,'Mendener Str. 9','51105 Kvln',77,'','Stephan','M|ller','MsR675tf',20011106225756);
INSERT INTO users VALUES (2,'christianemmler@t-online.de',NULL,'Delmestrasse 55','27777 Ganderkesee',77,'04222-7951073','Christian','Emmler','awfUaLOw',20011106223256);
INSERT INTO users VALUES (3,'R.Pusbatzkies@gmx.de',NULL,'Heideweg 6','03119 Welzow',77,'035751 12823','Rene','Pusbatzkies','4X2eRrsb',20011106224026);
INSERT INTO users VALUES (5,'meirose@studst.fh-muenster.de',NULL,'Lange Strasse 11','27777 Ganderkesee',77,'','Nils','Meirose','gPG1Simr',20011106233417);
INSERT INTO users VALUES (6,'alkas@t-online.de',NULL,'Emil-Barth-Str.99','Düsseldorf',77,'','Thomas','Volkmann','cwD7oD6H',20011106230847);
INSERT INTO users VALUES (7,'rosenhaeger@planet-interkom.de',NULL,'Mühlenbrink 18','Detmold',77,'05231-628338','Dirk','Rosenhäger','OwSKnl97',20011106231428);
INSERT INTO users VALUES (8,'sibbi@freenet.de',NULL,'Jägersberg 12','24103 Kiel',77,'0431552372','Christopher','Sievers','SachwIuS',20011106231808);
INSERT INTO users VALUES (9,'michael-steil@t-online.de',NULL,'Im Langgarten 14 A','66687 Wadern',77,'06874/182022','Michael','Steil','r3OHgxAp',20011106232959);
INSERT INTO users VALUES (10,'DMuenstermann@t-online.de',NULL,'Lärchenstr. 4','45892 Gelsenkirchen',77,'0209 799440','Denise','Münstermann','zcMn5FvU',20011106233000);
INSERT INTO users VALUES (11,'dvaergynlaender@gmx.de',NULL,'Lärchenstr. 4','45892 Gelsenkirchen',77,'0209 799440','Dirk','Marquardt','ylW7nwOm',20011106233000);
INSERT INTO users VALUES (12,'D.Axmacher@t-online.de',NULL,'Streuffstr. 46','Emmerich',77,'02828/92003','Daniel','Axmacher','wqpuZzMx',20011106233008);
INSERT INTO users VALUES (67,'bigkas@newsfactory.net',NULL,'Lechfeldstr. 23b','86316 Friedberg',77,'0821 229 29 12 (ges)','Klaus','Borchert','li1v7r2m',20011108200859);
INSERT INTO users VALUES (14,'cennaire@gmx.de',NULL,'Im Langgarten 14 A','66687 wadern',77,'','Sabine','Steil','MF6mH6ni',20011107000431);
INSERT INTO users VALUES (15,'Aerisprojekt@web.de',NULL,'Sundgauer Str. 105R','Berlin',77,'','Immanuel','Völker','ugTXl7Pn',20011107002356);
INSERT INTO users VALUES (20,'Schrat@t-online.de',NULL,'Wolfinstr. 10','77830 Bühlertal',77,'07223/991569','Jens','Schrader','o7uFqyKF',20011107085420);
INSERT INTO users VALUES (19,'saressa@celtic-visions.net',NULL,'Geismar Landstr. 9','Göttingen',77,'0551 / 49569266','Thomas','Schmeja','SiocxqZh',20011107085410);
INSERT INTO users VALUES (18,'Muescha@epost.de',NULL,'Saßnitzer Str. 4','Dresden',77,'','Michael','Sommer','0378RCOT',20011107085255);
INSERT INTO users VALUES (21,'red@gmx.de',NULL,'Selchower Strasse 28','12049 Berlin',77,'','Mareike','Paluk','IcC3kQ7P',20011107093654);
INSERT INTO users VALUES (22,'mirco-jabs@gmx.de',NULL,'Am Bollheister 54','47055 Duisburg',77,'','Mirco','Jabs','yREUOJd6',20011107095421);
INSERT INTO users VALUES (23,'egonaut@web.de',NULL,'Cammannstraße 4','38118 Braunschweig',77,'','Karsten','Schulz','ZSLPdqYW',20011107104844);
INSERT INTO users VALUES (24,'Santa_Cruz_@web.de',NULL,'Willy-Andreas-Allee 7','76131 Karlsruhe',77,'0178 - 4577630','Andreas','Kreuzer','xaC1vz69',20011107105426);
INSERT INTO users VALUES (25,'vic@tzi.de',NULL,'Hahnenstr. 21','28309 Bremen',77,'','Victor','Wundersee','Mv7l3PAz',20011107110426);
INSERT INTO users VALUES (26,'marcelkessels@web.de',NULL,'Bismarckstr. 51','41747 Viersen',77,'02162-574670','Marcel','Kessels','9XRr1bVY',20011107113517);
INSERT INTO users VALUES (27,'elfpunkt@yahoo.de',NULL,'Kirchheimer Str. 18','69214 Eppelheim',77,'','Dietmar','Fischer','ifEeJPKV',20011107120408);
INSERT INTO users VALUES (28,'ChiefMUC@gmx.net',NULL,'Josef-Frankl-Strasse 11B','80995 München',77,'','Oliver','Pappalardo','sQNeuVmz',20011107121008);
INSERT INTO users VALUES (29,'rolf.schmidt@nefkom.net',NULL,'Krugstr 22','90419 Nürnberg',77,'0049 172 8249600','Rolf','Schmidt','pYhJ45ya',20011107121817);
INSERT INTO users VALUES (30,'Gron-T.kar@gmx.de',NULL,'Hultroper Dorfstraße 19','59510 Lippetal-Hultrop',77,'02527/8362','Dominik','Gösken','6yMUcZE1',20011107123201);
INSERT INTO users VALUES (31,'roland.engels@web.de',NULL,'17 Lutton Place','Edinburgh EH8 9PD',3,'0044-131-6681134','Roland','Engels','bJLkxYpZ',20011107134618);
INSERT INTO users VALUES (32,'sahne@tzi.de',NULL,'Hinter dem Gartel 47','OHZ',77,'04791/899006','Daniel','Kühn','JVDBiaGt',20011107143430);
INSERT INTO users VALUES (33,'michael@kamenz.de',NULL,'Kirchweg 4','01920 Wendischbaselitz',77,'+49 3578 305068','Michael','Möller','jIqBsV1p',20011107151554);
INSERT INTO users VALUES (34,'wuestenkrieg@gmx.de',NULL,'Hauptstraße 9','02627 Breitendorf',77,'','Falk','Schneider','WJa1IAPy',20011107154137);
INSERT INTO users VALUES (35,'Sven.Pietschmann@t-online.de',NULL,'Löbauer Str. 64','Beiersdorf',77,'','Sven','Pietschmann','YnryGAD7',20011107154155);
INSERT INTO users VALUES (36,'520097125011-0001@t-online.de',NULL,'Nollinger Str. 42','79618 Rheinfelden',77,'0173/5215656','Uwe','Müller','AFPd86ap',20011107160339);
INSERT INTO users VALUES (37,'Schifferb@web.de',NULL,'Pappelstr. 71b','28199 Bremen',77,'0173-5779873','Bernd','Schiffer','uTkG6ild',20011107160859);
INSERT INTO users VALUES (38,'marten.kollakowski@t-online.de',NULL,'Carl-von-Ossietzky-Str. 3','29126 oldenburg',77,'0441/7779763','Marten','Kollakowski','ZYR9jY5e',20011107162558);
INSERT INTO users VALUES (39,'Markus.Klawitter@web.de',NULL,'Hoeltyweg 15','49082 Osnabrueck',77,'','Markus','Klawitter','YlB8yB6R',20011107164020);
INSERT INTO users VALUES (40,'cordesmartin@gmx.de',NULL,'Feldstrasse 79 a','Bremen',77,'0421 77412','Martin','Cordes','6tV8oqQf',20011107170659);
INSERT INTO users VALUES (41,'marco.vitali@gmx.ch',NULL,'Buechstrasse 18','5445 Eggenwil',193,'++41-(0)56-6316989','Marco','Vitali','T8HSsnWd',20011107172612);
INSERT INTO users VALUES (42,'moritzsalinger@web.de',NULL,'Obentrautstraße 64','10963 Berlin',77,'0173 97 95 701','Moritz','Salinger','GrfL4jon',20011107183554);
INSERT INTO users VALUES (43,'christian@decomain.de',NULL,'Spiegelsbergenweg 104A','Halberstadt',77,'+49 179 2155992','Christian','Decomain','s1Fx40y3',20011107190221);
INSERT INTO users VALUES (44,'ramona@schrepler.de',NULL,'Fritz-Frey-Str. 11','69121 Heidelberg',77,'06221-418010','Ramona','Schrepler','Lacn2Pyv',20011107195559);
INSERT INTO users VALUES (45,'ARose@nwn.de',NULL,'Walsroder Str.4','28215 Bremen',77,'','Arne','Rose','PrIw8UyY',20011107201347);
INSERT INTO users VALUES (46,'hbruhns@ix.urz.uni-heidelberg.de',NULL,'Fritz-Frey-Str. 11','69121 Heidelberg',77,'06221 418012','Hjalmar','Bruhns','zwpY7peL',20011107202024);
INSERT INTO users VALUES (47,'osprung@gmx.de',NULL,'Meißener Str.9','44139 Dortmund',77,'','Oliver','Sprung','xiXDEu9Y',20011107202741);
INSERT INTO users VALUES (48,'Fam.Spengler@t-online.de',NULL,'Irlenbornerstr. 14','53783 Eitorf',77,'02243 82178','Stephan','Spengler','okdq89vi',20011107203029);
INSERT INTO users VALUES (49,'gerhard.hecht@deutschlandweb.de',NULL,'Lerchenweg 16','86492 Egling a.d.Paar',77,'08206 / 903178','Gerhard','Hecht','s4ilkbQb',20011107213252);
INSERT INTO users VALUES (50,'Ralf.Hachmeister@t-online.de',NULL,'Georg-Viktor-Strasse 32','31812 Bad Pyrmont',77,'05281 960074','Ralf','Hachmeister','Rv5dJqqz',20011107221535);
INSERT INTO users VALUES (51,'gwaylare@gmx.net',NULL,'Leinestr. 2','Göttingen',77,'','Christoph','Albrecht','4Gp89cDU',20011107223151);
INSERT INTO users VALUES (52,'chennings@talknet.de',NULL,'Kämmerei 40','27749 Delmenhorst',77,'04221121222','Carsten','Hennings','HrPcHFQF',20011107223240);
INSERT INTO users VALUES (53,'thorsten.bahr@onlinehome.de',NULL,'Harthauser Straße 76','83043 Bad Aibling',77,'','Thorsten','Bahr','NYD9v561',20011107223249);
INSERT INTO users VALUES (54,'daniel@boiger.com',NULL,'Rechbergstraße 1','73240 Wendlingen',77,'','Daniel','Boiger','sj3ZmEZ6',20011107234225);
INSERT INTO users VALUES (55,'alerich2@gmx.net',NULL,'Geschwister-Scholl 6','91058 Erlangen',77,'09131/129670','Ulrich','Hofrichter','xntFLV17',20011108000555);
INSERT INTO users VALUES (56,'Seppel@prof-seppel.de',NULL,'Bayernallee 7','52066 Aachen',77,'','Sebastian','Oliva','fTDKrAhA',20011108023008);
INSERT INTO users VALUES (57,'helge.hennings@klinik.uni-regensburg.de',NULL,'Placidusstr. 8','93053 Regensburg',77,'0941/7081875','Helge','Hennings','0saimmtE',20011108095242);
INSERT INTO users VALUES (58,'andre.lerch@gmx.net',NULL,'Salgaer Str. 4','02694 Malschwitz',77,'','Andre','Lerch','npfnFzsF',20011108124045);
INSERT INTO users VALUES (59,'Volk-von-Condor@web.de',NULL,'Nadistr. 20','80809 München',77,'08151/4442450  oder  089/3573261','Ralf','Jung','o7tYHQtu',20011114122937);
INSERT INTO users VALUES (60,'raffa@tzi.de',NULL,'Bachstr. 81','28199 Bremen',77,'0421/393981','Raphael','Sturm','WfzzoBNb',20011108154052);
INSERT INTO users VALUES (61,'moekon@snafu.de',NULL,'JansaStr. 9','12045 Berlin',77,'030 / 62 72 76 84','Thomas','Konnerth','ufKYGIAu',20011108165022);
INSERT INTO users VALUES (62,'Morgon@Morgon.de',NULL,'Riegelstr. 58','63762 Großostheim',77,'06026/995153','Sebastian','Weigt','ehzZi6iz',20011108183027);
INSERT INTO users VALUES (63,'Rupalairpel@gmx.de',NULL,'Thüringer Straße 12','63811 Stockstadt',77,'','Andreas','Müller','rS9gevGv',20011108183035);
INSERT INTO users VALUES (64,'Oglbi@gmx.de',NULL,'Moritzstr.45','55130 Mainz',77,'','Alexander','Schoehl','6kv1s57y',20011108184527);
INSERT INTO users VALUES (65,'Xolgrim@gmx.de',NULL,'Hugo-Haelschner-Str.2','53129 Bonn',77,'0228/234588','Thomas','Straßberger','r1qBy4Ot',20011108192159);
INSERT INTO users VALUES (66,'paladin@bluemail.ch',NULL,'Alpenblickweg 17','3034 Uettligen',193,'','Matthias','Regli','KEbsf2EZ',20011108195229);
INSERT INTO users VALUES (68,'gzech@t-online.de',NULL,'brendelweg 42','27755 Delmenhorst',77,'0422124921','Guido','Zech','FchaKiVy',20011108215045);
INSERT INTO users VALUES (69,'stephan-heinrich@gmx.net',NULL,'Schnickenfeld 45a','25497 Prisdorf',77,'04101 782875','Stephan','Heinrich','CaYdwCs9',20011108215639);
INSERT INTO users VALUES (70,'r.m.glade@talknet.de',NULL,'Finsterwalderstraße 39','01239 Dresden',77,'0172/9147817','Matthias','Glade','a6PSYqFH',20011108222348);
INSERT INTO users VALUES (71,'vinyambar@waldgoettin.de',NULL,'Wehrweg 2','Kelkheim',77,'','Silvia','Tobies','VOgKOw9A',20011108223528);
INSERT INTO users VALUES (72,'peter.kraus@web.de',NULL,'Heideweg 94','50196 Kerpen',77,'0227369610','Peter','Kraus','nSK0W8q4',20011108224148);
INSERT INTO users VALUES (73,'cavendish@planet-interkom.de',NULL,'am gelskamp 16a','32758 detmold',77,'','michael','fisahn','2Z36XPjP',20011108224858);
INSERT INTO users VALUES (74,'ralphknoll@web.de',NULL,'Neutann 1','88364 Wolfegg',77,'','Ralph','Knoll','lC1dRs9P',20011109065601);
INSERT INTO users VALUES (75,'klaus@lottmann.de',NULL,'Neuenhainerstrasse 10','60326 Frankfurt',77,'01718596589','Klaus','Lottmann','EfW9d4rM',20011109093531);
INSERT INTO users VALUES (76,'pampala@gmx.de',NULL,'Grenadierweg 15','26129 Oldenburg',77,'0441/2179804','Pan','Pollack','e9aCJ47w',20011109094120);
INSERT INTO users VALUES (77,'p.biebow@web.de',NULL,'Friedrichstr. 72','68519 Viernheim',77,'0160 3241994','Peter','Biebow','6TIhSjM2',20011109094157);
INSERT INTO users VALUES (78,'Wilhelm.Dolle@brainmedia.de',NULL,'Cappeler Strasse 21','35039 Marburg',77,'','Wilhelm','Dolle','fufuxstL',20011109103349);
INSERT INTO users VALUES (79,'grrummpf@web.de',NULL,'Friedrichstr. 6','53757 Hangelar',77,'','Sebastian','Korte','xhBi2P64',20011109104641);
INSERT INTO users VALUES (80,'genua@snafu.de',NULL,'Winsstr. 22','10405 Berlin',77,'+49 172 3219138','Steffen','Schermaul','DxC9C8JW',20011109121424);
INSERT INTO users VALUES (81,'vinyambar@zigulle.de',NULL,'Sonnenstr. 232','44137 Dortmund',77,'','Daniel','Frickemeier','3GkZMurN',20011109130637);
INSERT INTO users VALUES (82,'alexander.metzner@informatik.uni-oldenburg.de',NULL,'Ahornweg 4','26919 Brake',77,'','Alexander','Metzner','F9GVOC2A',20011109143916);
INSERT INTO users VALUES (83,'Christian.Wachtendorf@Informatik.Uni-Oldenburg.DE',NULL,'Heiligengeistwall 10','26122 Oldenburg',77,'','Christian','Wachtendorf','Qs9xbr5P',20011109144906);
INSERT INTO users VALUES (84,'mallig@gmx.net',NULL,'Im Laimacker 32','79249 Merzhausen',77,'','Nicolai','Mallig','CFCsxTiL',20011109185952);
INSERT INTO users VALUES (85,'Carsten.Kaschube@web.de',NULL,'Sigmaringer Str. 52','72622 Nürtingen',77,'','Carsten','Kaschube','3IOR4tQx',20011109210108);
INSERT INTO users VALUES (86,'thomas-peter.klug@debitel.net',NULL,'Alter heerweg 35','53123 Bonn',77,'','Thomas-Peter','Klug','LYSt6qXS',20011109214720);
INSERT INTO users VALUES (87,'ahillenb@ix.urz.uni-heidelberg.de',NULL,'Albert-Überle-Str. 10','69120 Heidelberg',77,'06221 408995','Andreas','Hillenbach','LME6LSjq',20011110115209);
INSERT INTO users VALUES (88,'Xarkor@gmx.net',NULL,'Howaldtstr. 18','24118 Kiel',77,'0431-6409852','Michael','Jabs','jh5buGu3',20011110120628);
INSERT INTO users VALUES (89,'feibisch@estec.net',NULL,'Lutherstr. 84','07743 Jena',77,'03641/470096','Frank','Eibisch','KH1u5FEA',20011110130129);
INSERT INTO users VALUES (90,'ottstadt@sevcon.de',NULL,'Hermann-Löns-Weg 11','22848 Norderstedt',77,'040-512086-12','Willy','Ottstadt','NdYUBNQs',20011110141811);
INSERT INTO users VALUES (91,'Noilaht@web.de',NULL,'Ifteweg 6','58454 Witten',77,'','Thorsten','Engelbrecht','Hyyycvmt',20011110142138);
INSERT INTO users VALUES (93,'arthurrefinius@web.de',NULL,'Stolberger Str. 68','Aachen',77,'','Arthur','Refinius','MX2xIKNZ',20011110151659);
INSERT INTO users VALUES (94,'mina_murry@web.de',NULL,'Stolberger Str. 68','Aachen',77,'','Yvonne','Meis','lx5JAobE',20011110151718);
INSERT INTO users VALUES (95,'Frank-Michael.Zimmer@T-Online.de',NULL,'Ulmenweg 11','25451 Quickborn',77,'04106 / 66297','Frank-Michael','Zimmer','2RvsbXJB',20011110154508);
INSERT INTO users VALUES (96,'Holger.Gentemann@t-online.de',NULL,'Arminiusstr. 12','22525 Hamburg',77,'0408500103','Holger','Gentemann','2cWGhMhC',20011110155953);
INSERT INTO users VALUES (97,'enno@eressea.upb.de',NULL,'Huk Aveny 5b','0287 Oslo',154,'','Enno','Rehling','stkxSiHQ',20011110180250);
INSERT INTO users VALUES (98,'MJimBeam@aol.com',NULL,'Dresdener Ring 1','Hochheim',77,'','Michael','Simon','UzESKxUm',20011110190227);
INSERT INTO users VALUES (99,'centime@in-trier.de',NULL,'Henneystr.11','54293 Trier',77,'','Carsten','Pfennig','yC43Eob3',20011110200752);
INSERT INTO users VALUES (100,'stefan@siev.de',NULL,'Emil-von-Behring Str. 21','35041 Marburg',77,'','Stefan','Sievers','gWiiCQCZ',20011110201606);
INSERT INTO users VALUES (101,'matthias.frost@cityweb.de',NULL,'Zweibachweg 7','45279 Essen',77,'01605035882','Matthias','Frost','QQ9wHAAu',20011110204447);
INSERT INTO users VALUES (102,'gandalf@informatik.uni-bremen.de',NULL,'Dresdener Str. 1a','28844 Weyhe (bei Bremen)',77,'04203/810797','Cedrik','Duval','WDrEQTW2',20011111111333);
INSERT INTO users VALUES (103,'aramesvs@t-online.de',NULL,'Hauptstraße 48','Herzberg',77,'05521 2809','Frank','Nolte','4Ydo3fTT',20011111112533);
INSERT INTO users VALUES (104,'micha@lst.de',NULL,'Naegelsbacherstr. 49c','91052 Erlangen',77,'+49 9131 7192-325','Micha','Istine','kNgy9al3',20011111135542);
INSERT INTO users VALUES (105,'nilshorstmann@web.de',NULL,'Schwalbenstr. 4','28816 Stuhr',77,'','Nils','Horstmann','1ppXJrWV',20011111141125);
INSERT INTO users VALUES (106,'Feacor@web.de',NULL,'Nestorstr. 15','10709 Berlin',77,'','Daniel','Kohl','h48VSRLD',20011111144452);
INSERT INTO users VALUES (107,'boris_schroeder@web.de',NULL,'Kriegerstr. 42','30161 Hannover',77,'05111693916','Boris','Schröder','d3LORqkj',20011111161143);
INSERT INTO users VALUES (108,'ectorhga@linux.zrz.tu-berlin.de',NULL,'Nestorstr. 15','Berlin',77,'','Alexander','Sahm','vjdfhtOQ',20011111161623);
INSERT INTO users VALUES (109,'520020001929-0001@T-Online.de',NULL,'Ascher Str. 22','63477 Maintal',77,'','Nick','Sauter','u8rzSQAq',20011111162902);
INSERT INTO users VALUES (110,'joha_puck@web.de',NULL,'Kastanienstraße 13','24114 Kiel',77,'0431/6686648','Markus Johannes','Puck','ajWhzD34',20011111164353);
INSERT INTO users VALUES (111,'meini@einsteinfreun.de',NULL,'Auf der Schulenburg 22','33378 Rheda',77,'','Sebastian','Meinhardt','NkFXheZM',20011111184904);
INSERT INTO users VALUES (112,'Evewan@web.de',NULL,'Belßstraße 81','Berlin',77,'','Thomas','Leue','uxUiWIxE',20011111194928);
INSERT INTO users VALUES (114,'paul.fuehring@gmx.net',NULL,'Müllerstr. 30','13353 Berlin',77,'','Paul','Führing','OK2sRkAr',20011111202214);
INSERT INTO users VALUES (115,'Dragoon2913@t-online.de',NULL,'Winkelstr. 16','Herzberg',77,'','Mark','Szemeitat','FmYwtR0G',20011111214036);
INSERT INTO users VALUES (116,'MMalte@directbox.com',NULL,'Hohenloherstr 39 A','70435 Stuttgart',77,'0173/5938300','Malte','Möller','mKrRMgqo',20011111225156);
INSERT INTO users VALUES (117,'andreas.hallmann@gecits-eu.com',NULL,'Gröpelinger Heerstr. 301','28239 Bremen',77,'0421-6166311','Andreas','Hallmann','dPjRKZs2',20011112062521);
INSERT INTO users VALUES (118,'vinyambar@zerofoks.net',NULL,'Taubenstraße 18','28203 Bremen',77,'0421 7940905','Ferdinand','Steiger','9zxMjWDp',20011112081725);
INSERT INTO users VALUES (119,'gunty@talknet.de',NULL,'Nithackstr. 4','D-10585 Berlin',77,'(030) 347 02 758','Günther','Martinez Dreyer','vLN79XDJ',20011112101441);
INSERT INTO users VALUES (120,'Karsten.Meier@stim.de',NULL,'Südekumzeile 7a','13591 Berlin',77,'','Karsten','Meier','tdP0FoVr',20011112102722);
INSERT INTO users VALUES (121,'michael.tuscher@web.de',NULL,'Bussmannsfeld 109','44805 Bochum',77,'','Michael','Tuscher','eBJnJmsR',20011112102904);
INSERT INTO users VALUES (122,'daniel@boiger.com',NULL,'Rechbergstr. 1','Wendlingen',77,'','Daniel','Boiger','znUOJmZ0',20011112102923);
INSERT INTO users VALUES (123,'jb559755@rcs.urz.tu-dresden.de',NULL,'Gubener Str. 36','01237 Dresden',77,'0172/3515486','Jens','Bergmann','zxOoiau9',20011112121046);
INSERT INTO users VALUES (124,'phelan@phelan-net.de',NULL,'Klaushager Weg 17','13467 Berlin',77,'','Rainer','Schüler','uDtnGTSE',20011112141432);
INSERT INTO users VALUES (125,'Roger.Frehoff@t-online.de',NULL,'Bergheimer Weg 25','Gerlingen',77,'','Roger','Frehoff','FVv0gEJL',20011112142906);
INSERT INTO users VALUES (126,'Alexander.Miseler@SilverStyle.de',NULL,'Weidenweg 47','10249 Berlin',77,'','Alexander','Miseler','OC6kYMIS',20011112154757);
INSERT INTO users VALUES (127,'falen@freenet.de',NULL,'Hattingerstr.241','44795 Bochum',77,'','Torsten','Felske','CqrtmkIt',20011112165628);
INSERT INTO users VALUES (128,'jb559755@rcs.urz.tu-dresden.de',NULL,'Gubener Str. 36','01237 Dresden',77,'0172/3515486','Jens','Bergmann','UINiqkC1',20011112172156);
INSERT INTO users VALUES (129,'St.Ziegler@gmx.de',NULL,'Hirschgraben 24','Aachen',77,'','Stefan','Ziegler','nxQSXfF6',20011112184027);
INSERT INTO users VALUES (130,'tach.uli@t-online.de',NULL,'Limbeckstr.1b','44894 Bochum',77,'0234/261526','Ulrich','Meise','pPFz3COy',20011112204010);
INSERT INTO users VALUES (131,'cyrion_@web.de',NULL,'Bachstr. 81','28199 Bremen',77,'','Christian','Büthe','vwmvxxrH',20011112225303);
INSERT INTO users VALUES (132,'torsten@steigner.de',NULL,'Alleestraße 4','66882 Huetschenhausen',77,'','Torsten','Steigner','eZxwKddw',20011113005654);
INSERT INTO users VALUES (133,'esclarmunde@gmx.de',NULL,'Kastanienstr.13','24114 Kiel',77,'0431/6794667','Jörn','Gräbert','meXNQnPx',20011113013016);
INSERT INTO users VALUES (134,'harryrat@gmx.de',NULL,'Kastanienweg 13','52074 Aachen',77,'0241-81806','Harald','Radke','Pbzjwagv',20011113085313);
INSERT INTO users VALUES (135,'stefan@hoeffling.de',NULL,'Franziskanerstr. 1','56154 Boppard',77,'06742/82512','Stefan','Hoeffling','HLtorF5o',20011113145256);
INSERT INTO users VALUES (136,'esclarmunde@gmx.de',NULL,'Kastanienstr.13','24114 Kiel',77,'0431/6794667','Jörn','Gräbert','0HmvpMSi',20011113151218);
INSERT INTO users VALUES (137,'Frank.Adler@gmx.net',NULL,'Gertrudisweg 5','Euskirchen',77,'','Frank','Adler','6fWjD6Ib',20011113230300);
INSERT INTO users VALUES (138,'jens.otte@gmxpro.de',NULL,'Am Suedhang 5','Glinde',77,'+494075665561','Jens','Otte','m7omFMzP',20011114002043);
INSERT INTO users VALUES (139,'martin@hershoff.de',NULL,'Abtsbrede 47','33098 Paderborn',77,'','Martin','Hershoff','UTJXiCMw',20011114110237);
INSERT INTO users VALUES (140,'n.werhahn@t-online.de',NULL,'kohlenweg 10','Baden Baden',76534,'01796972416','nils','werhahn','ZfBmZlMe',20011114111939);
INSERT INTO users VALUES (141,'elvis@eressea-pbem.de',NULL,'Rockaway','Memphis',77,'','Elvis','The King','Z7VXtjiX',20011114162334);
INSERT INTO users VALUES (142,'zdomotor@axelero.hu',NULL,'Liszt Ferenc 3','H-2045 Törökbálint',93,'0036 23 336 385','Zoltán','Dömötör','p57pnZGf',20011116221701);
INSERT INTO users VALUES (143,'faber@kawo1.rwth-aachen.de',NULL,'Kastanienweg 4 / 2225','52074 Aachen',77,'0241/9810789','Michael','Ziegler','aDP4eoC0',20011118204013);

