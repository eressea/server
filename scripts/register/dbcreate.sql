# MySQL dump 8.13
#
# Host: localhost    Database: vinyambar
#--------------------------------------------------------
# Server version	3.23.36-log

#
# Table structure for table 'countries'
#

DROP table countries;
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
# Table structure for table 'factions'
#

DROP table factions;
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

drop table games;
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

#
# Table structure for table 'races'
#

drop table races;
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

drop table subscriptions;
CREATE TABLE subscriptions (
  game int(11) NOT NULL default '0',
  user int(11) NOT NULL default '0',
  credits int(11) NOT NULL default '0',
  race varchar(10) default NULL,
  id int(10) NOT NULL auto_increment,
  status varchar(10) NOT NULL default 'NEW',
  updated timestamp(14) NOT NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;


#
# Table structure for table 'users'
#

drop table users;
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
