/* vi: set ts=2:
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <config.h>
#include "eressea.h"
#include "xmlreader.h"

#include <xml.h>

/* kernel includes */
#include "building.h"
#include "equipment.h"
#include "item.h"
#include "message.h"
#include "race.h"
#include "ship.h"
#include "terrain.h"
#include "skill.h"
#include "spell.h"
#include "calendar.h"

/* util includes */
#include <util/functions.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <util/crmessage.h>

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <iconv.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

static boolean gamecode_enabled = false;

void
enable_xml_gamecode(void)
{
  gamecode_enabled = true;
}

static void
xml_readtext(xmlNodePtr node, struct locale ** lang, xmlChar **text)
{
  xmlChar * property = xmlGetProp(node, BAD_CAST "locale");
  assert(property!=NULL);
  *lang = find_locale((const char*)property);
  if (*lang==NULL) *lang = make_locale((const char*)property);
  xmlFree(property);

  *text = xmlNodeListGetString(node->doc, node->children, 1);
}

static const spell *
xml_spell(xmlNode * node, const char * name)
{
  const spell * sp = NULL;
  xmlChar * property = xmlGetProp(node, BAD_CAST name);
  if (property!=NULL) {
    sp = find_spell(M_NONE, (const char *)property);
    assert(sp);
    xmlFree(property);
  }
  return sp;
}

static const char *
xml_to_locale(const xmlChar * xmlStr)
{
  static iconv_t context = (iconv_t)-1;
  static char zText[1024];
  char * inbuf = (char*)xmlStr;
  char * outbuf = zText;
  size_t inbytes = strlen((const char*)xmlStr)+1;
  size_t outbytes = sizeof(zText);

  if (context==(iconv_t)-1) {
    context = iconv_open("", "UTF-8");
  }
  assert(context!=(iconv_t)-1);

  iconv(context, &inbuf, &inbytes, &outbuf, &outbytes);
  if (inbytes!=0) {
    log_error(("string is too long: %d chars remain in %s.\n", inbytes, (const char*)xmlStr));
  }
  return zText;
}

static xmlChar *
xml_cleanup_string(xmlChar * str)
{
  xmlChar * read = str;
  xmlChar * write = str;

  while (*read) {
    /* eat leading whitespace */
	if (*read && isspace(*read)) {
	  while (*read && isspace(*read)) {
		++read;
	  }
	  *write++ = ' ';
	}
    while (*read) {
      if (*read== '\n') break;
      if (*read== '\r') break;
      *write++ = *read++;
    }
  }
  *write = 0;
  return str;
}

static const resource_type *
rt_findorcreate(const char * name)
{
  resource_type * rtype = rt_find(name);
  if (rtype==NULL) {
    const char * names[2];
    char * namep = strcat(strcpy((char*)malloc(strlen(name)+3), name), "_p");
    /* we'll make a placeholder */
    names[0] = name;
    names[1] = namep;
    rtype = new_resourcetype(names, NULL, RTF_NONE);
    free(namep);
  }
  return rtype;
}

static void
xml_readrequirements(xmlNodePtr * nodeTab, int nodeNr, requirement ** reqArray)
{
  int req;
  requirement * radd = *reqArray;

  assert (radd==NULL);
  if (nodeNr==0) return;

  radd = *reqArray = calloc(sizeof(requirement), nodeNr+1);

  for (req=0;req!=nodeNr;++req) {
    xmlNodePtr node = nodeTab[req];
    xmlChar * property;

    radd->number = xml_ivalue(node, "quantity", 1);
    radd->recycle = xml_fvalue(node, "recycle", 0.0);

    property = xmlGetProp(node, BAD_CAST "type");
    radd->rtype = rt_findorcreate((const char*)property);
    xmlFree(property);

    ++radd;
  }
}

static void
xml_readconstruction(xmlXPathContextPtr xpath, xmlNodePtr * nodeTab, int nodeNr, construction ** consPtr)
{
  xmlNodePtr pushNode = xpath->node;
  int k;
  for (k=0;k!=nodeNr;++k) {
    xmlNodePtr node = nodeTab[k];
    xmlChar * property;
    construction * con;
    xmlXPathObjectPtr req;
    int m;

    assert(*consPtr==NULL);
    *consPtr = con = calloc(sizeof(construction), 1);
    consPtr = &con->improvement;

    property = xmlGetProp(node, BAD_CAST "skill");
    assert(property!=NULL);
    con->skill = sk_find((const char*)property);
    assert(con->skill!=NOSKILL);
    xmlFree(property);

    con->maxsize = xml_ivalue(node, "maxsize", -1);
    con->minskill = xml_ivalue(node, "minskill", -1);
    con->reqsize = xml_ivalue(node, "reqsize", -1);

    /* read construction/requirement */
    xpath->node = node;
    req = xmlXPathEvalExpression(BAD_CAST "requirement", xpath);
    xml_readrequirements(req->nodesetval->nodeTab,
      req->nodesetval->nodeNr, &con->materials);
    xmlXPathFreeObject(req);

    /* read construction/modifier */
    xpath->node = node;
    req = xmlXPathEvalExpression(BAD_CAST "modifier", xpath);
    for (m=0;m!=req->nodesetval->nodeNr;++m) {
      xmlNodePtr node = req->nodesetval->nodeTab[m];

      property = xmlGetProp(node, BAD_CAST "function");
      if (property!=NULL) {
        pf_generic foo = get_function((const char*)property);
        a_add(&con->attribs, make_skillmod(NOSKILL, SMF_PRODUCTION, (skillmod_fun)foo, 1.0, 0));
        xmlFree(property);
      }

    }
    xmlXPathFreeObject(req);

  }
  xpath->node = pushNode;
}

static int
parse_function(xmlNodePtr node, pf_generic * funPtr, xmlChar ** namePtr)
{
  pf_generic fun;
  xmlChar * property = xmlGetProp(node, BAD_CAST "value");
  assert(property!=NULL);
  fun = get_function((const char*)property);
  if (fun!=NULL) {
    xmlFree(property);

    property = xmlGetProp(node, BAD_CAST "name");
  }
  *namePtr = property;
  *funPtr = fun;
  return 0;
}

static int
parse_buildings(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr buildings;
  int i;

  /* reading eressea/buildings/building */
  buildings = xmlXPathEvalExpression(BAD_CAST "/eressea/buildings/building", xpath);
  if (buildings->nodesetval!=NULL) {
    xmlNodeSetPtr nodes = buildings->nodesetval;
    for (i=0;i!=nodes->nodeNr;++i) {
      xmlNodePtr node = nodes->nodeTab[i];
      xmlChar * property;
      building_type * bt = calloc(sizeof(building_type), 1);
      xmlXPathObjectPtr result;
      int k;

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      bt->_name = strdup((const char *)property);
      xmlFree(property);

      bt->capacity = xml_ivalue(node, "capacity", -1);
      bt->maxcapacity = xml_ivalue(node, "maxcapacity", -1);
      bt->maxsize = xml_ivalue(node, "maxsize", -1);

      bt->magres = xml_ivalue(node, "magres", 0);
      bt->magresbonus = xml_ivalue(node, "magresbonus", 0);
      bt->fumblebonus = xml_ivalue(node, "fumblebonus", 0);
      bt->auraregen = xml_fvalue(node, "auraregen", 1.0);

      if (xml_bvalue(node, "nodestroy", false)) bt->flags |= BTF_INDESTRUCTIBLE;
      if (xml_bvalue(node, "oneperturn", false)) bt->flags |= BTF_ONEPERTURN;
      if (xml_bvalue(node, "nobuild", false)) bt->flags |= BTF_NOBUILD;
      if (xml_bvalue(node, "unique", false)) bt->flags |= BTF_UNIQUE;
      if (xml_bvalue(node, "decay", false)) bt->flags |= BTF_DECAY;
      if (xml_bvalue(node, "magic", false)) bt->flags |= BTF_MAGIC;
      if (xml_bvalue(node, "protection", false)) bt->flags |= BTF_PROTECTION;

      /* reading eressea/buildings/building/construction */
      xpath->node = node;
      result = xmlXPathEvalExpression(BAD_CAST "construction", xpath);
      xml_readconstruction(xpath, result->nodesetval->nodeTab, result->nodesetval->nodeNr, &bt->construction);
      xmlXPathFreeObject(result);

      if (gamecode_enabled) {
        /* reading eressea/buildings/building/function */
        xpath->node = node;
        result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
        for (k=0;k!=result->nodesetval->nodeNr;++k) {
          xmlNodePtr node = result->nodesetval->nodeTab[k];
          pf_generic fun;
          parse_function(node, &fun, &property);

          if (fun==NULL) {
            log_error(("unknown function name '%s' for building %s\n",
              (const char*)property, bt->_name));
            xmlFree(property);
            continue;
          }
          assert(property!=NULL);
          if (strcmp((const char*)property, "name")==0) {
            bt->name = (const char * (*)(int size))fun;
          } else if (strcmp((const char*)property, "init")==0) {
            bt->init = (void (*)(struct building_type*))fun;
          } else {
            log_error(("unknown function type '%s' for building %s\n",
              (const char*)property, bt->_name));
          }
          xmlFree(property);
        }
        xmlXPathFreeObject(result);
      }

      /* reading eressea/buildings/building/maintenance */
      result = xmlXPathEvalExpression(BAD_CAST "maintenance", xpath);
      for (k=0;k!=result->nodesetval->nodeNr;++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        maintenance * mt;

        if (bt->maintenance==NULL) {
          bt->maintenance = calloc(sizeof(struct maintenance), result->nodesetval->nodeNr+1);
        }
        mt = bt->maintenance + k;
        mt->number = xml_ivalue(node, "amount", 0);

        property = xmlGetProp(node, BAD_CAST "type");
        assert(property!=NULL);
        mt->rtype = rt_find((const char*)property);
        assert(mt->rtype!=NULL);
        xmlFree(property);

        if (xml_bvalue(node, "variable", false)) mt->flags |= MTF_VARIABLE;
        if (xml_bvalue(node, "vital", false)) mt->flags |= MTF_VITAL;

      }
      xmlXPathFreeObject(result);

      /* finally, register the new building type */
      bt_register(bt);
    }
  }
  xmlXPathFreeObject(buildings);

  xmlXPathFreeContext(xpath);
  return 0;
}

static int
parse_calendar(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr xpathCalendars;
  xmlNodeSetPtr nsetCalendars;
  int rv = 0;

  /* reading eressea/buildings/building */
  xpathCalendars = xmlXPathEvalExpression(BAD_CAST "/eressea/calendar", xpath);
  nsetCalendars = xpathCalendars->nodesetval;
  if (nsetCalendars==NULL || nsetCalendars->nodeNr!=1) {
    log_error(("invalid or missing calendar data in %s\n", doc->name));
    rv = -1;
  } else {
    xmlNodePtr calendar = nsetCalendars->nodeTab[0];
    xmlXPathObjectPtr xpathWeeks, xpathMonths, xpathSeasons;
    xmlNodeSetPtr nsetWeeks, nsetMonths, nsetSeasons;
    xmlChar * property = xmlGetProp(calendar, BAD_CAST "name");
    xmlChar * newyear = xmlGetProp(calendar, BAD_CAST "newyear");

    first_turn = xml_ivalue(calendar, "start", 0);
    if (property) {
      agename = strdup(mkname("calendar", (const char*)property));
      xmlFree(property);
    }

    xpath->node = calendar;
    xpathWeeks = xmlXPathEvalExpression(BAD_CAST "week", xpath);
    nsetWeeks = xpathWeeks->nodesetval;
    if (nsetWeeks!=NULL) {
      int i;

      weeks_per_month = nsetWeeks->nodeNr;
      weeknames = malloc(sizeof(char *) * weeks_per_month);
      weeknames2 = malloc(sizeof(char *) * weeks_per_month);
      for (i=0;i!=nsetWeeks->nodeNr;++i) {
        xmlNodePtr week = nsetWeeks->nodeTab[i];
        xmlChar * property = xmlGetProp(week, BAD_CAST "name");
        if (property) {
          weeknames[i] = strdup(mkname("calendar", (const char*)property));
          weeknames2[i] = malloc(strlen(weeknames[i])+3);
          sprintf(weeknames2[i], "%s_d", weeknames[i]);
          xmlFree(property);
        }
      }
    }
    xmlXPathFreeObject(xpathWeeks);

    months_per_year = 0;
    xpathSeasons = xmlXPathEvalExpression(BAD_CAST "season", xpath);
    nsetSeasons = xpathSeasons->nodesetval;
    if (nsetSeasons!=NULL) {
      int i;

      seasons = nsetSeasons->nodeNr;
      seasonnames = malloc(sizeof(char *) * seasons);

      for (i=0;i!=nsetSeasons->nodeNr;++i) {
        xmlNodePtr season = nsetSeasons->nodeTab[i];
        xmlChar * property = xmlGetProp(season, BAD_CAST "name");
        if (property) {
          seasonnames[i] = strdup(mkname("calendar", (const char*)property));
          xmlFree(property);
        }
      }
    }

    xpathMonths = xmlXPathEvalExpression(BAD_CAST "season/month", xpath);
    nsetMonths = xpathMonths->nodesetval;
    if (nsetMonths!=NULL) {
      int i;

      months_per_year = nsetMonths->nodeNr;
      monthnames = malloc(sizeof(char *) * months_per_year);
      month_season = malloc(sizeof(int) * months_per_year);
      storms = malloc(sizeof(int) * months_per_year);

      for (i=0;i!=nsetMonths->nodeNr;++i) {
        xmlNodePtr month = nsetMonths->nodeTab[i];
        xmlChar * property = xmlGetProp(month, BAD_CAST "name");
        int j;

        if (property) {
          if (newyear && strcmp((const char*)newyear, (const char*)property)==0) {
            first_month = i;
            xmlFree(newyear);
            newyear = NULL;
          }
          monthnames[i] = strdup(mkname("calendar", (const char*)property));
          xmlFree(property);
        }
        for (j=0;j!=seasons;++j) {
          xmlNodePtr season = month->parent;
          if (season==nsetSeasons->nodeTab[j]) {
            month_season[i] = j;
            break;
          }
        }
        assert(j!=seasons);
        storms[i] = xml_ivalue(nsetMonths->nodeTab[i], "storm", 0);
      }
    }
    xmlXPathFreeObject(xpathMonths);
    xmlXPathFreeObject(xpathSeasons);
  }
  xmlXPathFreeObject(xpathCalendars);
  xmlXPathFreeContext(xpath);
 
  return rv;
}

static int
parse_ships(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr ships;
  int i;

  /* reading eressea/ships/ship */
  ships = xmlXPathEvalExpression(BAD_CAST "/eressea/ships/ship", xpath);
  if (ships->nodesetval!=NULL) {
    xmlNodeSetPtr nodes = ships->nodesetval;
    for (i=0;i!=nodes->nodeNr;++i) {
      xmlNodePtr node = nodes->nodeTab[i];
      xmlChar * property;
      ship_type * st = calloc(sizeof(ship_type), 1);
      xmlXPathObjectPtr result;
      int k, c;

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      st->name[0] = strdup((const char *)property);
      st->name[1] = strcat(strcpy(malloc(strlen(st->name[0])+3), st->name[0]),"_a");
      xmlFree(property);

      st->cabins = xml_ivalue(node, "cabins", 0);
      st->cargo = xml_ivalue(node, "cargo", 0);
      st->combat = xml_ivalue(node, "combat", 0);
      st->cptskill = xml_ivalue(node, "cptskill", 0);
      st->damage = xml_fvalue(node, "damage", 0.0);
      if (xml_bvalue(node, "fly", false)) st->flags |= SFL_FLY;
      if (xml_bvalue(node, "opensea", false)) st->flags |= SFL_OPENSEA;
      st->minskill = xml_ivalue(node, "minskill", 0);
      st->range = xml_ivalue(node, "range", 0);
      st->storm = xml_fvalue(node, "storm", 1.0);
      st->sumskill = xml_ivalue(node, "sumskill", 0);

	  /* reading eressea/ships/ship/construction */
      xpath->node = node;
      result = xmlXPathEvalExpression(BAD_CAST "construction", xpath);
      xml_readconstruction(xpath, result->nodesetval->nodeTab, result->nodesetval->nodeNr, &st->construction);
      xmlXPathFreeObject(result);

	  /* reading eressea/ships/ship/coast */
      xpath->node = node;
      result = xmlXPathEvalExpression(BAD_CAST "coast", xpath);
      for (c=0,k=0;k!=result->nodesetval->nodeNr;++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];

        if (k==0) {
          assert(st->coasts==NULL);
          st->coasts = (const terrain_type**)malloc(sizeof(const terrain_type*) * (result->nodesetval->nodeNr+1));
          st->coasts[result->nodesetval->nodeNr] = NULL;
        }

        property = xmlGetProp(node, BAD_CAST "terrain");
        assert(property!=NULL);
        st->coasts[c] = get_terrain((const char*)property);
        if (st->coasts[c]!=NULL) ++c;
        else {
          log_warning(("ship %s mentions a non-existing terrain %s.\n", st->name[0], property));
        }
        xmlFree(property);
      }
      xmlXPathFreeObject(result);

      /* finally, register the new building type */
      st_register(st);
    }
  }
  xmlXPathFreeObject(ships);

  xmlXPathFreeContext(xpath);
  return 0;
}

static void
race_compat(void)
{
  /* required for old_race, do not change order! */
  const char * oldracenames[MAXRACES] = {
    "dwarf", "elf", "orc", "goblin", "human", "troll", "demon", "insect",
    "halfling", "cat", "aquarian", "uruk", "snotling", "undead", "illusion",
    "youngdragon", "dragon", "wyrm", "ent", "catdragon", "dracoid",
    "special", "spell", "irongolem", "stonegolem", "shadowdemon",
    "shadowmaster", "mountainguard", "alp", "toad", "braineater", "peasant",
    "wolf", "lynx", "tunnelworm", "eagle", "rat", "songdragon", "nymph",
    "unicorn", "direwolf", "ghost", "imp", "dreamcat", "fairy", "owl",
    "hellcat", "tiger", "dolphin", "giantturtle", "kraken", "seaserpent",
    "shadowknight", "centaur", "skeleton", "skeletonlord", "zombie",
    "juju-zombie", "ghoul", "ghast", "museumghost", "gnome", "template",
    "clone", "shadowdragon", "shadowbat", "nightmare", "vampunicorn",
    "phoenix"
  };
  int i;

  for (i=0;i!=MAXRACES;++i) {
    race * rc = rc_find(oldracenames[i]);
    if (rc) {
      new_race[i] = rc;
      if (rc == new_race[RC_TROLL]) {
        a_add(&rc->attribs, make_skillmod(NOSKILL, SMF_RIDING, NULL, 0.0, -1));
      }
    } else {
      log_warning(("could not find old race %s\n", oldracenames[i]));
    }
  }
}

static potion_type *
xml_readpotion(xmlXPathContextPtr xpath, item_type * itype)
{
  int level = xml_ivalue(xpath->node, "level", 0);

  assert(level>0);
  return new_potiontype(itype, level);
}

static luxury_type *
xml_readluxury(xmlXPathContextPtr xpath, item_type * itype)
{
  int price = xml_ivalue(xpath->node, "price", 0);
  return new_luxurytype(itype, price);
}


static armor_type *
xml_readarmor(xmlXPathContextPtr xpath, item_type * itype)
{
  xmlNodePtr node = xpath->node;
  armor_type * atype = NULL;
  unsigned int flags = ATF_NONE;
  int ac = xml_ivalue(node, "ac", 0);
  double penalty = xml_fvalue(node, "penalty", 0.0);
  double magres = xml_fvalue(node, "magres", 0.0);

  if (xml_bvalue(node, "laen", false)) flags |= ATF_LAEN;
  if (xml_bvalue(node, "shield", false)) flags |= ATF_SHIELD;

  atype = new_armortype(itype, penalty, magres, ac, flags);
  return atype;
}

static weapon_type *
xml_readweapon(xmlXPathContextPtr xpath, item_type * itype)
{
  xmlNodePtr node = xpath->node;
  weapon_type * wtype = NULL;
  unsigned int flags = WTF_NONE;
  xmlXPathObjectPtr result;
  xmlChar * property;
  int k;
  skill_t sk;
  int minskill = xml_ivalue(node, "minskill", 0);
  int offmod = xml_ivalue(node, "offmod", 0);
  int defmod = xml_ivalue(node, "defmod", 0);
  int reload = xml_ivalue(node, "reload", 0);
  double magres = xml_fvalue(node, "magres", 0.0);

  if (xml_bvalue(node, "armorpiercing", false)) flags |= WTF_ARMORPIERCING;
  if (xml_bvalue(node, "magical", false)) flags |= WTF_MAGICAL;
  if (xml_bvalue(node, "missile", false)) flags |= WTF_MISSILE;
  if (xml_bvalue(node, "pierce", false)) flags |= WTF_PIERCE;
  if (xml_bvalue(node, "cut", false)) flags |= WTF_CUT;
  if (xml_bvalue(node, "blunt", false)) flags |= WTF_BLUNT;

  property = xmlGetProp(node, BAD_CAST "skill");
  assert(property!=NULL);
  sk = sk_find((const char*)property);
  assert(sk!=NOSKILL);
  xmlFree(property);

  wtype = new_weapontype(itype, flags, magres, NULL, offmod, defmod, reload, sk, minskill);

  /* reading weapon/damage */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "damage", xpath);
  assert(result->nodesetval->nodeNr<=2);
  for (k=0;k!=result->nodesetval->nodeNr;++k) {
    xmlNodePtr node = result->nodesetval->nodeTab[k];
    int pos = 0;

    property = xmlGetProp(node, BAD_CAST "type");
    if (strcmp((const char *)property, "footman")!=0) {
      pos = 1;
    }
    xmlFree(property);

    property = xmlGetProp(node, BAD_CAST "value");
    wtype->damage[pos] = gc_add(strdup((const char*)property));
    if (k==0) wtype->damage[1-pos] = wtype->damage[pos];
    xmlFree(property);
  }
  xmlXPathFreeObject(result);

  /* reading weapon/modifier */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "modifier", xpath);
  assert(wtype->modifiers==NULL);
  wtype->modifiers = calloc(sizeof(weapon_mod), result->nodesetval->nodeNr+1);
  for (k=0;k!=result->nodesetval->nodeNr;++k) {
    xmlNodePtr node = result->nodesetval->nodeTab[k];
    xmlXPathObjectPtr races;
    int r, flags = 0;

    if (xml_bvalue(node, "walking", false)) flags|=WMF_WALKING;
    if (xml_bvalue(node, "riding", false)) flags|=WMF_RIDING;
    if (xml_bvalue(node, "against_walking", false)) flags|=WMF_AGAINST_WALKING;
    if (xml_bvalue(node, "against_riding", false)) flags|=WMF_AGAINST_RIDING;
    if (xml_bvalue(node, "offensive", false)) flags|=WMF_OFFENSIVE;
    if (xml_bvalue(node, "defensive", false)) flags|=WMF_DEFENSIVE;

    property = xmlGetProp(node, BAD_CAST "type");
    if (strcmp((const char*)property, "damage")==0) flags|=WMF_DAMAGE;
    else if (strcmp((const char*)property, "skill")==0) flags|=WMF_SKILL;
    else if (strcmp((const char*)property, "missile_target")==0) flags|=WMF_MISSILE_TARGET;
    xmlFree(property);

    wtype->modifiers[k].flags = flags;
    wtype->modifiers[k].value = xml_ivalue(node, "value", 0);

    xpath->node = node;
    races = xmlXPathEvalExpression(BAD_CAST "race", xpath);
    for (r=0;r!=races->nodesetval->nodeNr;++r) {
      xmlNodePtr node = races->nodesetval->nodeTab[r];

      property = xmlGetProp(node, BAD_CAST "name");
      if (property!=NULL) {
        const race * rc = rc_find((const char*)property);
        if (rc==NULL) rc = rc_add(rc_new((const char*)property));
        racelist_insert(&wtype->modifiers[k].races, rc);
        xmlFree(property);
      }
    }
    xmlXPathFreeObject(races);
  }
  xmlXPathFreeObject(result);

  if (gamecode_enabled) {
    /* reading weapon/function */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      xmlChar * property;
      pf_generic fun;

      parse_function(node, &fun, &property);
      if (fun==NULL) {
        log_error(("unknown function name '%s' for item '%s'\n",
          (const char*)property, itype->rtype->_name[0]));
        xmlFree(property);
        continue;
      }
      assert(property!=NULL);
      if (strcmp((const char*)property, "attack")==0) {
        wtype->attack = (boolean (*)(const struct troop*, const struct weapon_type *, int*, int))fun;
      } else {
        log_error(("unknown function type '%s' for item '%s'\n",
          (const char*)property, itype->rtype->_name[0]));
      }
      xmlFree(property);
    }
    xmlXPathFreeObject(result);
  }

  xpath->node = node;
  return wtype;
}

static item_type *
xml_readitem(xmlXPathContextPtr xpath, resource_type * rtype)
{
  xmlNodePtr node = xpath->node;
  item_type * itype = NULL;
  unsigned int flags = ITF_NONE;
  xmlXPathObjectPtr result;
  int k;

  int weight = xml_ivalue(node, "weight", 0);
  int capacity = xml_ivalue(node, "capacity", 0);

  if (xml_bvalue(node, "cursed", false)) flags |= ITF_CURSED;
  if (xml_bvalue(node, "notlost", false)) flags |= ITF_NOTLOST;
  if (xml_bvalue(node, "herb", false)) flags |= ITF_HERB;
  if (xml_bvalue(node, "big", false)) flags |= ITF_BIG;
  if (xml_bvalue(node, "animal", false)) flags |= ITF_ANIMAL;
  itype = new_itemtype(rtype, flags, weight, capacity);
#ifdef SCORE_MODULE
  itype->score = xml_ivalue(node, "score", 0);
#endif

  /* reading item/construction */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "construction", xpath);
  xml_readconstruction(xpath, result->nodesetval->nodeTab, result->nodesetval->nodeNr, &itype->construction);
  xmlXPathFreeObject(result);

  /* reading item/weapon */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "weapon", xpath);
  assert(result->nodesetval->nodeNr<=1);
  if (result->nodesetval->nodeNr!=0) {
    xpath->node = result->nodesetval->nodeTab[0];
    rtype->wtype = xml_readweapon(xpath, itype);
  }
  xmlXPathFreeObject(result);

  /* reading item/potion */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "potion", xpath);
  assert(result->nodesetval->nodeNr<=1);
  if (result->nodesetval->nodeNr!=0) {
    xpath->node = result->nodesetval->nodeTab[0];
    rtype->ptype = xml_readpotion(xpath, itype);
  }
  xmlXPathFreeObject(result);

  /* reading item/luxury */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "luxury", xpath);
  assert(result->nodesetval->nodeNr<=1);
  if (result->nodesetval->nodeNr!=0) {
    xpath->node = result->nodesetval->nodeTab[0];
    rtype->ltype = xml_readluxury(xpath, itype);
  }
  xmlXPathFreeObject(result);

  /* reading item/armor */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "armor", xpath);
  assert(result->nodesetval->nodeNr<=1);
  if (result->nodesetval->nodeNr!=0) {
    xpath->node = result->nodesetval->nodeTab[0];
    rtype->atype = xml_readarmor(xpath, itype);
  }
  xmlXPathFreeObject(result);

  if (gamecode_enabled) {
    /* reading item/function */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      xmlChar * property;
      pf_generic fun;

      parse_function(node, &fun, &property);
      if (fun==NULL) {
        log_error(("unknown function name '%s' for item '%s'\n",
          (const char*)property, rtype->_name[0]));
        xmlFree(property);
        continue;
      }
      assert(property!=NULL);
      if (strcmp((const char*)property, "give")==0) {
        itype->give = (boolean (*)(const struct unit*, const struct unit*, const struct item_type *, int, struct order *))fun;
      }
      else if (strcmp((const char*)property, "use")==0) {
        itype->use = (int (*)(struct unit *, const struct item_type *, int, struct order *))fun;
      } else {
        log_error(("unknown function type '%s' for item '%s'\n",
          (const char*)property, rtype->_name[0]));
      }
      xmlFree(property);
    }
    xmlXPathFreeObject(result);
  }

  return itype;
}

static int
parse_resources(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr resources;
  xmlNodeSetPtr nodes;
  int i;

  /* reading eressea/resources/resource */
  resources = xmlXPathEvalExpression(BAD_CAST "/eressea/resources/resource", xpath);
  nodes = resources->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    xmlChar *property, *name, *appearance;
    const char *names[2], *appearances[2];
    char * namep = NULL, * appearancep = NULL;
    resource_type * rtype;
    unsigned int flags = RTF_NONE;
    xmlXPathObjectPtr result;
    int k;

    if (xml_bvalue(node, "pooled", true)) flags |= RTF_POOLED;
    if (xml_bvalue(node, "limited", false)) flags |= RTF_LIMITED;
    if (xml_bvalue(node, "sneak", false)) flags |= RTF_SNEAK;

    name = xmlGetProp(node, BAD_CAST "name");
    appearance = xmlGetProp(node, BAD_CAST "appearance");
    assert(name!=NULL);

    if (appearance!=NULL) {
      appearancep = strcat(strcpy((char*)malloc(strlen((char*)appearance)+3), (char*)appearance), "_p");
    }

    rtype = rt_find((const char *)name);
    if (rtype!=NULL) {
      /* dependency from another item, was created earlier */
      rtype->flags |= flags;
      if (appearance) {
        rtype->_appearance[0] = strdup((const char*)appearance);
        rtype->_appearance[1] = appearancep;
        free(appearancep);
      }
    } else {
      namep = strcat(strcpy((char*)malloc(strlen((char*)name)+3), (char*)name), "_p");
      names[0] = (const char*)name;
      names[1] = namep;
      if (appearance) {
        appearances[0] = (const char*)appearance;
        appearances[1] = appearancep;
        rtype = new_resourcetype((const char**)names, (const char**)appearances, flags);
        free(appearancep);
      } else {
        rtype = new_resourcetype(names, NULL, flags);
      }
      free(namep);
    }

    if (name) xmlFree(name);
    if (appearance) xmlFree(appearance);

    if (gamecode_enabled) {
      /* reading eressea/resources/resource/function */
      xpath->node = node;
      result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
      if (result->nodesetval!=NULL) for (k=0;k!=result->nodesetval->nodeNr;++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        pf_generic fun;

        parse_function(node, &fun, &property);
        if (fun==NULL) {
          log_error(("unknown function name '%s' for resource %s\n",
            (const char*)property, rtype->_name[0]));
          xmlFree(property);
          continue;
        }

        assert(property!=NULL);
        if (strcmp((const char*)property, "change")==0) {
          rtype->uchange = (rtype_uchange)fun;
        } else if (strcmp((const char*)property, "get")==0) {
          rtype->uget = (rtype_uget)fun;
        } else if (strcmp((const char*)property, "name")==0) {
          rtype->name = (rtype_name)fun;
        } else {
          log_error(("unknown function type '%s' for resource %s\n",
            (const char*)property, rtype->_name[0]));
        }
        xmlFree(property);
      }
      xmlXPathFreeObject(result);
    }

    /* reading eressea/resources/resource/resourcelimit/function */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "resourcelimit/function", xpath);
    if (result->nodesetval!=NULL) for (k=0;k!=result->nodesetval->nodeNr;++k) {
      attrib * a = a_find(rtype->attribs, &at_resourcelimit);
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      pf_generic fun;

      property = xmlGetProp(node, BAD_CAST "value");
      assert(property!=NULL);
      fun = get_function((const char*)property);
      if (fun==NULL) {
        log_error(("unknown limit '%s' for resource %s\n",
          (const char*)property, rtype->_name[0]));
        xmlFree(property);
        continue;
      }
      xmlFree(property);

      if (a==NULL) a = a_add(&rtype->attribs, a_new(&at_resourcelimit));

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      if (strcmp((const char*)property, "use")==0) {
        resource_limit * rdata = (resource_limit*)a->data.v;
        rdata->use = (rlimit_use)fun;
      } else if (strcmp((const char*)property, "limit")==0) {
        resource_limit * rdata = (resource_limit*)a->data.v;
        rdata->limit = (rlimit_limit)fun;
      } else {
        log_error(("unknown limit '%s' for resource %s\n",
          (const char*)property, rtype->_name[0]));
      }
      xmlFree(property);
    }
    xmlXPathFreeObject(result);

    /* reading eressea/resources/resource/item */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "item", xpath);
    assert(result->nodesetval->nodeNr<=1);
    if (result->nodesetval->nodeNr!=0) {
      rtype->flags |= RTF_ITEM;
      xpath->node = result->nodesetval->nodeTab[0];
      rtype->itype = xml_readitem(xpath, rtype);
    }
    xmlXPathFreeObject(result);
  }
  xmlXPathFreeObject(resources);

  xmlXPathFreeContext(xpath);

  /* make sure old items (used in requirements) are available */
  init_resources();

  return 0;
}

static void
add_items(equipment * eq, xmlNodeSetPtr nsetItems)
{
  if (nsetItems!=NULL && nsetItems->nodeNr>0) {
    int i;
    for (i=0;i!=nsetItems->nodeNr;++i) {
      xmlNodePtr node = nsetItems->nodeTab[i];
      xmlChar * property;
      const struct item_type * itype;

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      itype = it_find((const char*)property);
      xmlFree(property);
      if (itype!=NULL) {
        property = xmlGetProp(node, BAD_CAST "amount");
        if (property!=NULL) {
          equipment_setitem(eq, itype, (const char*)property);
          xmlFree(property);
        }
      }
    }
  }
}

static void
add_spells(equipment * eq, xmlNodeSetPtr nsetItems)
{
  if (nsetItems!=NULL && nsetItems->nodeNr>0) {
    int i;
    for (i=0;i!=nsetItems->nodeNr;++i) {
      xmlNodePtr node = nsetItems->nodeTab[i];
      xmlChar * property;
      magic_t mtype = M_GRAU;
      struct spell * sp;

      property = xmlGetProp(node, BAD_CAST "school");
      if (property!=NULL) {
        for (mtype=0;mtype!=MAXMAGIETYP;++mtype) {
          if (strcmp((const char*)property, magietypen[mtype])==0) break;
        }
        assert(mtype!=MAXMAGIETYP);
        xmlFree(property);
      }

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      sp = find_spell(mtype, (const char*)property);
      assert(sp);
      xmlFree(property);
      if (sp!=NULL) {
        equipment_addspell(eq, sp);
      }
    }
  }
}

static void
add_skills(equipment * eq, xmlNodeSetPtr nsetSkills)
{
  if (nsetSkills!=NULL && nsetSkills->nodeNr>0) {
    int i;
    for (i=0;i!=nsetSkills->nodeNr;++i) {
      xmlNodePtr node = nsetSkills->nodeTab[i];
      xmlChar * property;
      skill_t sk;

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      sk = sk_find((const char*)property);
      xmlFree(property);
      if (sk!=NOSKILL) {
        property = xmlGetProp(node, BAD_CAST "level");
        if (property!=NULL) {
          equipment_setskill(eq, sk, (const char*)property);
          xmlFree(property);
        } 
      }
    }
  }
}

static void
add_subsets(xmlDocPtr doc, equipment * eq, xmlNodeSetPtr nsetSkills)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  if (nsetSkills!=NULL && nsetSkills->nodeNr>0) {
    int i;

    eq->subsets = calloc(nsetSkills->nodeNr+1, sizeof(subset));
    for (i=0;i!=nsetSkills->nodeNr;++i) {
      xmlXPathObjectPtr xpathResult;
      xmlNodePtr node = nsetSkills->nodeTab[i];
      xmlChar * property;

      eq->subsets[i].chance = 1.0f;
      property = xmlGetProp(node, BAD_CAST "chance");
      if (property!=NULL) {
        eq->subsets[i].chance = (float)atof((const char *)property);
        xmlFree(property);
      }
      xpath->node = node;
      xpathResult = xmlXPathEvalExpression(BAD_CAST "set", xpath);
      if (xpathResult->nodesetval) {
        xmlNodeSetPtr nsetSets = xpathResult->nodesetval;
        float totalChance = 0.0f;

        if (nsetSets->nodeNr>0) {
          int set;
          eq->subsets[i].sets = calloc(nsetSets->nodeNr+1, sizeof(subsetitem));
          for (set=0;set!=nsetSets->nodeNr;++set) {
            xmlNodePtr nodeSet = nsetSets->nodeTab[set];
            float chance = 1.0f;

            property = xmlGetProp(nodeSet, BAD_CAST "chance");
            if (property!=NULL) {
              chance = (float)atof((const char *)property);
              xmlFree(property);
            }
            totalChance += chance;

            property = xmlGetProp(nodeSet, BAD_CAST "name");
            assert(property!=NULL);
            eq->subsets[i].sets[set].chance = chance;
            eq->subsets[i].sets[set].set = create_equipment((const char*)property);
            xmlFree(property);
          }
        }
        if (totalChance>1.0f) {
          log_error(("total chance exceeds 1.0: %f in equipment set %s.\n", 
            totalChance, eq->name));
        }
      }
      xmlXPathFreeObject(xpathResult);
    }
  }
  xmlXPathFreeContext(xpath);
}

static int
parse_equipment(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr xpathRaces;

  /* reading eressea/races/race */
  xpathRaces = xmlXPathEvalExpression(BAD_CAST "/eressea/equipment/set", xpath);
  if (xpathRaces->nodesetval) {
    xmlNodeSetPtr nsetRaces = xpathRaces->nodesetval;
    int i;

    for (i=0;i!=nsetRaces->nodeNr;++i) {
      xmlNodePtr node = nsetRaces->nodeTab[i];
      xmlChar * property = xmlGetProp(node, BAD_CAST "name");

      if (property!=NULL) {
        equipment * eq = create_equipment((const char*)property);
        xmlXPathObjectPtr xpathResult;

        xpath->node = node;

        xpathResult = xmlXPathEvalExpression(BAD_CAST "item", xpath);
        add_items(eq, xpathResult->nodesetval);
        xmlXPathFreeObject(xpathResult);

        xpathResult = xmlXPathEvalExpression(BAD_CAST "spell", xpath);
        add_spells(eq, xpathResult->nodesetval);
        xmlXPathFreeObject(xpathResult);

        xpathResult = xmlXPathEvalExpression(BAD_CAST "skill", xpath);
        add_skills(eq, xpathResult->nodesetval);
        xmlXPathFreeObject(xpathResult);

        xpathResult = xmlXPathEvalExpression(BAD_CAST "subset", xpath);
        add_subsets(doc, eq, xpathResult->nodesetval);
        xmlXPathFreeObject(xpathResult);

        xmlFree(property);
      }
    }
  }

  xmlXPathFreeObject(xpathRaces);
  xmlXPathFreeContext(xpath);

  return 0;
}

static int
parse_spells(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr spells;

  /* reading eressea/spells/spell */
  spells = xmlXPathEvalExpression(BAD_CAST "/eressea/spells/spell", xpath);
  if (spells->nodesetval!=NULL) {
    xmlNodeSetPtr nodes = spells->nodesetval;
    int i;

    for (i=0;i!=nodes->nodeNr;++i) {
      xmlXPathObjectPtr result;
      xmlNodePtr node = nodes->nodeTab[i];
      xmlChar * property;
      int k;
      spell * sp = calloc(1, sizeof(spell));

      /* spellname */
      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      sp->sname = strdup((const char*)property);
      xmlFree(property);

      /* magic type */
      property = xmlGetProp(node, BAD_CAST "type");
      assert(property!=NULL);
      for (sp->magietyp=0;sp->magietyp!=MAXMAGIETYP;++sp->magietyp) {
        if (strcmp(magietypen[sp->magietyp], (const char *)property)==0) break;
      }
      assert(sp->magietyp!=MAXMAGIETYP);
      xmlFree(property);

      /* level, rank and flags */
      sp->id = xml_ivalue(node, "index", 0);
      sp->level = xml_ivalue(node, "level", -1);
      sp->rank = (char)xml_ivalue(node, "rank", -1);
      if (xml_bvalue(node, "ship", false)) sp->sptyp |= ONSHIPCAST;
      if (xml_bvalue(node, "ocean", false)) sp->sptyp |= OCEANCASTABLE;
      if (xml_bvalue(node, "far", false)) sp->sptyp |= FARCASTING;

      if (gamecode_enabled) {
        /* reading eressea/spells/spell/function */
        xpath->node = node;
        result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
        for (k=0;k!=result->nodesetval->nodeNr;++k) {
          xmlNodePtr node = result->nodesetval->nodeTab[k];
          pf_generic fun;

          parse_function(node, &fun, &property);
          if (fun==NULL) {
            log_error(("unknown function name '%s' for spell '%s'\n",
              (const char*)property, sp->sname));
            xmlFree(property);
            continue;
          }
          assert(property!=NULL);
          if (strcmp((const char*)property, "cast")==0) {
            sp->sp_function = (spell_f)fun;
          } else if (strcmp((const char*)property, "fumble")==0) {
            sp->patzer = (pspell_f)fun;
          } else {
            log_error(("unknown function type '%s' for spell %s\n", 
              (const char*)property, sp->sname));
          }
          xmlFree(property);
        }
        xmlXPathFreeObject(result);
      }

      /* reading eressea/spells/spell/resource */
      xpath->node = node;
      result = xmlXPathEvalExpression(BAD_CAST "resource", xpath);
      if (result->nodesetval->nodeNr) {
        sp->components = malloc(sizeof(spell_component)*(result->nodesetval->nodeNr+1));
        sp->components[result->nodesetval->nodeNr].type = 0;
      }
      for (k=0;k!=result->nodesetval->nodeNr;++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        property = xmlGetProp(node, BAD_CAST "name");
        assert(property);
        sp->components[k].type = rt_find((const char *)property);
        assert(sp->components[k].type);
        xmlFree(property);
        sp->components[k].amount = xml_ivalue(node, "amount", 1);
        sp->components[k].cost = SPC_FIX;
        property = xmlGetProp(node, BAD_CAST "cost");
        if (property!=NULL) {
          if (strcmp((const char *)property, "linear")==0) {
            sp->components[k].cost = SPC_LINEAR;
          } else if (strcmp((const char *)property, "level")==0) {
            sp->components[k].cost = SPC_LEVEL;
          }
          xmlFree(property);
        }
      }
      xmlXPathFreeObject(result);
      register_spell(sp);
    }
  }

  xmlXPathFreeObject(spells);

  xmlXPathFreeContext(xpath);

  init_spells();

  return 0;
}

static int
parse_races(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr races;
  xmlNodeSetPtr nodes;
  int i;

  /* reading eressea/races/race */
  races = xmlXPathEvalExpression(BAD_CAST "/eressea/races/race", xpath);
  nodes = races->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    xmlChar * property;
    race * rc;
    xmlXPathObjectPtr result;
    int k;
    struct att * attack;

    property = xmlGetProp(node, BAD_CAST "name");
    assert(property!=NULL);
    rc = rc_find((const char*)property);
    if (rc==NULL) rc = rc_add(rc_new((const char*)property));
    xmlFree(property);

    property = xmlGetProp(node, BAD_CAST "damage");
    assert(property!=NULL);
    rc->def_damage = strdup((const char*)property);
    xmlFree(property);

    rc->magres = (float)xml_fvalue(node, "magres", 0.0);
    rc->maxaura = (float)xml_fvalue(node, "maxaura", 0.0);
    rc->regaura = (float)xml_fvalue(node, "regaura", 1.0);
    rc->recruitcost = xml_ivalue(node, "recruitcost", 0);
    rc->maintenance = xml_ivalue(node, "maintenance", 0);
    rc->weight = xml_ivalue(node, "weight", 0);
#ifdef RACE_CAPACITY
    rc->capacity = xml_ivalue(node, "capacity", 0);
#endif
    rc->speed = (float)xml_fvalue(node, "speed", 1.0F);
    rc->hitpoints = xml_ivalue(node, "hp", 0);
    rc->armor = (char)xml_ivalue(node, "ac", 0);

    rc->at_default = (char)xml_ivalue(node, "unarmedattack", -2);
    rc->df_default = (char)xml_ivalue(node, "unarmeddefense", -2);
    rc->at_bonus = (char)xml_ivalue(node, "attackmodifier", 0);
    rc->df_bonus = (char)xml_ivalue(node, "defensemodifier", 0);

    if (xml_bvalue(node, "playerrace", false)) rc->flags |= RCF_PLAYERRACE;
    if (xml_bvalue(node, "scarepeasants", false)) rc->flags |= RCF_SCAREPEASANTS;
    if (xml_bvalue(node, "cannotmove", false)) rc->flags |= RCF_CANNOTMOVE;
    if (xml_bvalue(node, "fly", false)) rc->flags |= RCF_FLY;
    if (xml_bvalue(node, "coastal", false)) rc->flags |= RCF_COASTAL;
    if (xml_bvalue(node, "swim", false)) rc->flags |= RCF_SWIM;
    if (xml_bvalue(node, "walk", false)) rc->flags |= RCF_WALK;
    if (xml_bvalue(node, "nolearn", false)) rc->flags |= RCF_NOLEARN;
    if (xml_bvalue(node, "noteach", false)) rc->flags |= RCF_NOTEACH;
    if (xml_bvalue(node, "horse", false)) rc->flags |= RCF_HORSE;
    if (xml_bvalue(node, "desert", false)) rc->flags |= RCF_DESERT;
    if (xml_bvalue(node, "absorbpeasants", false)) rc->flags |= RCF_ABSORBPEASANTS;
    if (xml_bvalue(node, "noheal", false)) rc->flags |= RCF_NOHEAL;
    if (xml_bvalue(node, "noweapons", false)) rc->flags |= RCF_NOWEAPONS;
    if (xml_bvalue(node, "shapeshift", false)) rc->flags |= RCF_SHAPESHIFT;
    if (xml_bvalue(node, "shapeshiftany", false)) rc->flags |= RCF_SHAPESHIFTANY;
    if (xml_bvalue(node, "illusionary", false)) rc->flags |= RCF_ILLUSIONARY;
    if (xml_bvalue(node, "undead", false)) rc->flags |= RCF_UNDEAD;
    if (xml_bvalue(node, "dragon", false)) rc->flags |= RCF_DRAGON;

    if (xml_bvalue(node, "nogive", false)) rc->ec_flags |= NOGIVE;
    if (xml_bvalue(node, "giveitem", false)) rc->ec_flags |= GIVEITEM;
    if (xml_bvalue(node, "giveperson", false)) rc->ec_flags |= GIVEPERSON;
    if (xml_bvalue(node, "giveunit", false)) rc->ec_flags |= GIVEUNIT;
    if (xml_bvalue(node, "getitem", false)) rc->ec_flags |= GETITEM;
    if (xml_bvalue(node, "canguard", false)) rc->ec_flags |= CANGUARD;
    if (xml_bvalue(node, "recruithorses", false)) rc->ec_flags |= ECF_REC_HORSES;
    if (xml_bvalue(node, "recruitethereal", false)) rc->ec_flags |= ECF_REC_ETHEREAL;
    if (xml_bvalue(node, "recruitunlimited", false)) rc->ec_flags |= ECF_REC_UNLIMITED;

    if (xml_bvalue(node, "equipment", false)) rc->battle_flags |= BF_EQUIPMENT;
    if (xml_bvalue(node, "noblock", false)) rc->battle_flags |= BF_NOBLOCK;
    if (xml_bvalue(node, "invinciblenonmagic", false)) rc->battle_flags |= BF_INV_NONMAGIC;
    if (xml_bvalue(node, "resistbash", false)) rc->battle_flags |= BF_RES_BASH;
    if (xml_bvalue(node, "resistcut", false)) rc->battle_flags |= BF_RES_CUT;
    if (xml_bvalue(node, "resistpierce", false)) rc->battle_flags |= BF_RES_PIERCE;

    /* reading eressea/races/race/ai */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "ai", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];

      rc->splitsize = xml_ivalue(node, "splitsize", 0);
      rc->aggression = (float)xml_fvalue(node, "aggression", 0.04);
      if (xml_bvalue(node, "killpeasants", false)) rc->flags |= RCF_KILLPEASANTS;
      if (xml_bvalue(node, "moverandom", false)) rc->flags |= RCF_MOVERANDOM;
      if (xml_bvalue(node, "learn", false)) rc->flags |= RCF_LEARN;
    }
    xmlXPathFreeObject(result);

    /* reading eressea/races/race/skill */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "skill", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      int mod = xml_ivalue(node, "modifier", 0);

      property = xmlGetProp(node, BAD_CAST "name");
      assert(property!=NULL);
      if (mod!=0) {
        skill_t sk = sk_find((const char*)property);
        if (sk!=NOSKILL) {
          rc->bonus[sk] = (char)mod;
        } else {
          log_error(("unknown skill '%s' in race '%s'\n",
            (const char*)property, rc->_name[0]));
        }
      }
      xmlFree(property);
    }
    xmlXPathFreeObject(result);

    if (gamecode_enabled) {
      /* reading eressea/races/race/function */
      xpath->node = node;
      result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
      for (k=0;k!=result->nodesetval->nodeNr;++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        pf_generic fun;

        parse_function(node, &fun, &property);
        if (fun==NULL) {
          log_error(("unknown function name '%s' for race %s\n",
            (const char*)property, rc->_name[0]));
          xmlFree(property);
          continue;
        }
        assert(property!=NULL);
        if (strcmp((const char*)property, "name")==0) {
          rc->generate_name = (const char* (*)(const struct unit*))fun;
        } else if (strcmp((const char*)property, "age")==0) {
          rc->age = (void(*)(struct unit*))fun;
        } else if (strcmp((const char*)property, "move")==0) {
          rc->move_allowed = (boolean(*)(const struct region *, const struct region *))fun;
        } else if (strcmp((const char*)property, "itemdrop")==0) {
          rc->itemdrop = (struct item *(*)(const struct race *, int))fun;
        } else if (strcmp((const char*)property, "initfamiliar")==0) {
          rc->init_familiar = (void(*)(struct unit *))fun;
        } else {
          log_error(("unknown function type '%s' for race %s\n",
            (const char*)property, rc->_name[0]));
        }
        xmlFree(property);
      }
      xmlXPathFreeObject(result);
    }

    /* reading eressea/races/race/familiar */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "familiar", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      race * frc;

      property = xmlGetProp(node, BAD_CAST "race");
      assert(property!=NULL);
      frc = rc_find((const char *)property);
      if (frc == NULL) {
        log_error(("%s not registered, is familiar for %s\n",
          (const char*)property, rc->_name[0]));
        assert(frc!=NULL);
        frc = rc_add(rc_new((const char*)property));
      }
      if (xml_bvalue(node, "default", false)) {
        rc->familiars[k] = rc->familiars[0];
        rc->familiars[0] = frc;
      } else {
        rc->familiars[k] = frc;
      }
      xmlFree(property);
    }
    xmlXPathFreeObject(result);

    /* reading eressea/races/race/precombatspell */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "precombatspell", xpath);
    assert(rc->precombatspell==NULL || !"precombatspell is already initialized");
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      rc->precombatspell = xml_spell(node, "spell");
    }
    xmlXPathFreeObject(result);

    /* reading eressea/races/race/attack */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "attack", xpath);
    attack = rc->attack;
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      while (attack->type!=AT_NONE) ++attack;

      property = xmlGetProp(node, BAD_CAST "damage");
      if (property!=NULL) {
        attack->data.dice = strdup((const char*)property);
        xmlFree(property);
      } else {
        attack->data.sp = xml_spell(node, "spell");
      }
      attack->type = xml_ivalue(node, "type", 0);
      attack->flags = xml_ivalue(node, "flags", 0);
    }
    xmlXPathFreeObject(result);
  }
  xmlXPathFreeObject(races);

  xmlXPathFreeContext(xpath);

  race_compat();
  return 0;
}

static int
parse_terrains(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath;
  xmlXPathObjectPtr terrains;
  xmlNodeSetPtr nodes;
  int i;

  xpath = xmlXPathNewContext(doc);

  /* reading eressea/strings/string */
  terrains = xmlXPathEvalExpression(BAD_CAST "/eressea/terrains/terrain", xpath);
  nodes = terrains->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    terrain_type * terrain = calloc(1, sizeof(terrain_type));
    xmlChar * property;
    xmlXPathObjectPtr xpathChildren;
    xmlNodeSetPtr children;

    property = xmlGetProp(node, BAD_CAST "name");
    assert(property!=NULL);
    terrain->_name = strdup((const char *)property);
    xmlFree(property);

    terrain->max_road = (short)xml_ivalue(node, "road", -1);
    terrain->size = xml_ivalue(node, "size", 0);

    if (xml_bvalue(node, "forbidden", false)) terrain->flags |= FORBIDDEN_REGION;
    else {
      if (xml_bvalue(node, "fly", true)) terrain->flags |= FLY_INTO;
      if (xml_bvalue(node, "sail", true)) terrain->flags |= SAIL_INTO;
      if (xml_bvalue(node, "walk", true)) terrain->flags |= WALK_INTO;
      if (xml_bvalue(node, "swim", false)) terrain->flags |= SWIM_INTO;
      if (xml_bvalue(node, "shallow", true)) terrain->flags |= LARGE_SHIPS;
      if (xml_bvalue(node, "cavalry", false)) terrain->flags |= CAVALRY_REGION;
    }
    if (xml_bvalue(node, "sea", false)) terrain->flags |= SEA_REGION;
    if (xml_bvalue(node, "arctic", false)) terrain->flags |= ARCTIC_REGION;
    if (xml_bvalue(node, "land", true)) terrain->flags |= LAND_REGION;
    if (xml_bvalue(node, "forest", false)) terrain->flags |= FOREST_REGION;

    terrain->distribution = (short)xml_ivalue(node, "seed", 0);

    xpath->node = node;
    xpathChildren = xmlXPathEvalExpression(BAD_CAST "herb", xpath);
    children = xpathChildren->nodesetval;
    if (children->nodeNr>0) {
      int k;

      terrain->herbs = malloc((children->nodeNr+1) * sizeof(item_type*));
      terrain->herbs[children->nodeNr] = NULL;
      for (k=0;k!=children->nodeNr;++k) {
        xmlNodePtr nodeHerb = children->nodeTab[k];
        const struct resource_type * rtype;

        property = xmlGetProp(nodeHerb, BAD_CAST "name");
        assert(property!=NULL);
        rtype = rt_find((const char*)property);
        assert(rtype!=NULL && rtype->itype!=NULL && fval(rtype->itype, ITF_HERB));
        terrain->herbs[k] = rtype->itype;
        xmlFree(property);
      }
    }
    xmlXPathFreeObject(xpathChildren);

    xpath->node = node;
    xpathChildren = xmlXPathEvalExpression(BAD_CAST "resource", xpath);
    children = xpathChildren->nodesetval;
    if (children->nodeNr>0) {
      int k;

      terrain->production = malloc((children->nodeNr+1) * sizeof(terrain_production));
      terrain->production[children->nodeNr].type = NULL;
      for (k=0;k!=children->nodeNr;++k) {
        xmlNodePtr nodeProd = children->nodeTab[k];

        property = xmlGetProp(nodeProd, BAD_CAST "name");
        assert(property!=NULL);
        terrain->production[k].type = rt_find((const char*)property);
        assert(terrain->production[k].type);
        xmlFree(property);

        property = xmlGetProp(nodeProd, BAD_CAST "level");
        assert(property);
        terrain->production[k].startlevel = strdup((const char *)property);
        xmlFree(property);

        property = xmlGetProp(nodeProd, BAD_CAST "base");
        assert(property);
        terrain->production[k].base = strdup((const char *)property);
        xmlFree(property);

        property = xmlGetProp(nodeProd, BAD_CAST "div");
        assert(property);
        terrain->production[k].divisor = strdup((const char *)property);
        xmlFree(property);

        terrain->production[k].chance = (float)xml_fvalue(nodeProd, "chance", 1.0);
      }
    }
    xmlXPathFreeObject(xpathChildren);

    register_terrain(terrain);
  }
  xmlXPathFreeObject(terrains);

  xmlXPathFreeContext(xpath);

  init_terrains();
  return 0;
}

static int
parse_messages(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath;
  xmlXPathObjectPtr messages;
  xmlNodeSetPtr nodes;
  int i;

  if (!gamecode_enabled) return 0;

  xpath = xmlXPathNewContext(doc);

  /* reading eressea/strings/string */
  messages = xmlXPathEvalExpression(BAD_CAST "/eressea/messages/message", xpath);
  nodes = messages->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    const char * default_section = "events";
    xmlChar * section;
    xmlChar * property;
    xmlXPathObjectPtr result;
    int k;
    char ** argv = NULL;
    const message_type * mtype;

    /* arguments */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "type/arg", xpath);
    if (result->nodesetval->nodeNr>0) {
      argv = malloc(sizeof(char*)*(result->nodesetval->nodeNr+1));
      for (k=0;k!=result->nodesetval->nodeNr;++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        char zBuffer[128];
        xmlChar * name, * type;

        name = xmlGetProp(node, BAD_CAST "name");
        type = xmlGetProp(node, BAD_CAST "type");
        sprintf(zBuffer, "%s:%s", (const char*)name, (const char*)type);
        xmlFree(name);
        xmlFree(type);
        argv[k] = strdup(zBuffer);
      }
      argv[result->nodesetval->nodeNr] = NULL;
    }
    xmlXPathFreeObject(result);

    /* add the messagetype */
    property = xmlGetProp(node, BAD_CAST "name");
    mtype = mt_find((const char *)property);
    if (mtype==NULL) {
      mtype = mt_register(mt_new((const char *)property, (const char**)argv));
    } else {
      assert(argv!=NULL || !"cannot redefine arguments of message now");
    }
    xmlFree(property);

    /* register the type for the CR */
    crt_register(mtype);

    /* let's clean up the mess */
    if (argv!=NULL) {
      for (k=0;argv[k]!=NULL;++k) free(argv[k]); 
      free(argv);
    }

    section = xmlGetProp(node, BAD_CAST "section");
    if (section==NULL) section = BAD_CAST default_section;
    mc_add((const char*)section);

    /* strings */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "text", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr node = result->nodesetval->nodeTab[k];
      struct locale * lang;
      xmlChar * text;

      xml_readtext(node, &lang, &text);
      xml_cleanup_string(text);
      nrt_register(mtype, lang, xml_to_locale(text), 0, (const char*)section);
      xmlFree(text);

    }
    xmlXPathFreeObject(result);

    if (section != BAD_CAST default_section) xmlFree(section);
  }

  xmlXPathFreeObject(messages);

  xmlXPathFreeContext(xpath);
  return 0;
}

static void
xml_readstrings(xmlXPathContextPtr xpath, xmlNodePtr * nodeTab, int nodeNr, boolean names)
{
  int i;

  for (i=0;i!=nodeNr;++i) {
    xmlNodePtr stringNode = nodeTab[i];
    xmlChar * name = xmlGetProp(stringNode, BAD_CAST "name");
    xmlChar * nspc = NULL;
    xmlXPathObjectPtr result;
    int k;
    char zName[128];

    assert(name!=NULL);
    if (names) nspc = xmlGetProp(stringNode->parent, BAD_CAST "name");
    mkname_buf((const char*)nspc, (const char*)name, zName);
    if (nspc!=NULL) xmlFree(nspc);
    xmlFree(name);

    /* strings */
    xpath->node = stringNode;
    result = xmlXPathEvalExpression(BAD_CAST "text", xpath);
    for (k=0;k!=result->nodesetval->nodeNr;++k) {
      xmlNodePtr textNode = result->nodesetval->nodeTab[k];
      struct locale * lang;
      xmlChar * text;

      xml_readtext(textNode, &lang, &text);
      if (text!=NULL) {
        assert(strcmp(zName, (const char*)xml_cleanup_string(BAD_CAST zName))==0);
        xml_cleanup_string(text);
        locale_setstring(lang, zName, xml_to_locale(text));
        xmlFree(text);
      } else {
        log_warning(("string %s has no text in locale %s\n",
          zName, locale_name(lang)));
      }
    }
    xmlXPathFreeObject(result);
  }
}

static int
parse_strings(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr strings;

  /* reading eressea/strings/string */
  strings = xmlXPathEvalExpression(BAD_CAST "/eressea/strings/string", xpath);
  xml_readstrings(xpath, strings->nodesetval->nodeTab, strings->nodesetval->nodeNr, false);
  xmlXPathFreeObject(strings);

  strings = xmlXPathEvalExpression(BAD_CAST "/eressea/strings/namespace/string", xpath);
  xml_readstrings(xpath, strings->nodesetval->nodeTab, strings->nodesetval->nodeNr, true);
  xmlXPathFreeObject(strings);

  xmlXPathFreeContext(xpath);
  return 0;
}

static void
xml_readprefixes(xmlXPathContextPtr xpath, xmlNodePtr * nodeTab, int nodeNr, boolean names)
{
  int i;

  for (i=0;i!=nodeNr;++i) {
    xmlNodePtr node = nodeTab[i];
    xmlChar * text = xmlNodeListGetString(node->doc, node->children, 1);

    if (text!=NULL) {
      add_raceprefix((const char*)text);
      xmlFree(text);
    }
  }
}

static int
parse_prefixes(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr strings;

  /* reading eressea/strings/string */
  strings = xmlXPathEvalExpression(BAD_CAST "/eressea/prefixes/prefix", xpath);
  xml_readprefixes(xpath, strings->nodesetval->nodeTab, strings->nodesetval->nodeNr, false);
  xmlXPathFreeObject(strings);

  xmlXPathFreeContext(xpath);
  return 0;
}

static int
parse_main(xmlDocPtr doc)
{
  xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
  xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "/eressea/game", xpath);
  xmlNodeSetPtr nodes = result->nodesetval;
  int i;

  xmlChar * property;
  xmlNodePtr node = nodes->nodeTab[0];

  property = xmlGetProp(node, BAD_CAST "welcome");
  if (property!=NULL) {
    global.welcomepath = strdup((const char*)property);
    xmlFree(property);
  }
 
  global.unitsperalliance = xml_bvalue(node, "unitsperalliance", false);
  global.maxunits = xml_ivalue(node, "units", INT_MAX);

  property = xmlGetProp(node, BAD_CAST "name");
  if (property!=NULL) {
    global.gamename = strdup((const char*)property);
    xmlFree(property);
  }

  xmlXPathFreeObject(result);

  /* reading eressea/game/param */
  xpath->node = node;
  result = xmlXPathEvalExpression(BAD_CAST "param", xpath);
  nodes = result->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    xmlChar * name = xmlGetProp(node, BAD_CAST "name");
    xmlChar * value = xmlGetProp(node, BAD_CAST "value");

    set_param(&global.parameters, (const char*)name, (const char*)value);

    xmlFree(name);
    xmlFree(value);
  }
  xmlXPathFreeObject(result);

  /* reading eressea/game/order */
  result = xmlXPathEvalExpression(BAD_CAST "order", xpath);
  nodes = result->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    xmlChar * name = xmlGetProp(node, BAD_CAST "name");
    boolean disable = xml_bvalue(node, "disable", false);

    if (disable) {
      int k;
      for (k=0;k!=MAXKEYWORDS;++k) {
        if (strcmp(keywords[k], (const char*)name)==0) {
          global.disabled[k]=1;
          break;
        }
      }
      if (k==MAXKEYWORDS) {
        log_error(("trying to disable unknown comand %s\n", (const char*)name));
      }
    }
    xmlFree(name);
  }
  xmlXPathFreeObject(result);

  /* reading eressea/game/skill */
  result = xmlXPathEvalExpression(BAD_CAST "skill", xpath);
  nodes = result->nodesetval;
  for (i=0;i!=nodes->nodeNr;++i) {
    xmlNodePtr node = nodes->nodeTab[i];
    xmlChar * name = xmlGetProp(node, BAD_CAST "name");
    boolean enable = xml_bvalue(node, "enable", true);
    enable_skill((const char*)name, enable);
    xmlFree(name);
  }
  xmlXPathFreeObject(result);

  xmlXPathFreeContext(xpath);
  return 0;
}

void
register_xmlreader(void)
{
  xml_register_callback(parse_main);

  xml_register_callback(parse_strings);
  xml_register_callback(parse_prefixes);
  xml_register_callback(parse_messages);
  xml_register_callback(parse_resources);

  xml_register_callback(parse_terrains); /* requires resources */
  xml_register_callback(parse_buildings); /* requires resources */
  xml_register_callback(parse_ships); /* requires terrains */
  xml_register_callback(parse_spells); /* requires resources */
  xml_register_callback(parse_equipment); /* requires spells */
  xml_register_callback(parse_races); /* requires spells */
  xml_register_callback(parse_calendar);
}
