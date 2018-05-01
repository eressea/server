/* 
+-------------------+
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2004   |  Katja Zedel <katze@felidae.kn-bremen.de>
|                   |  Henning Peters <faroul@beyond.kn-bremen.de>
+-------------------+

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>

#include "xmlreader.h"

#include "kernel/building.h"
#include "kernel/equipment.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/terrain.h"
#include "kernel/skills.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"

#include "alchemy.h"
#include "kernel/calendar.h"
#include "guard.h"
#include "keyword.h"
#include "move.h"
#include "prefix.h"

#include <modules/score.h>
#include <attributes/attributes.h>

/* util includes */
#include <util/attrib.h>
#include <util/crmessage.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <util/strings.h>
#include <util/xml.h>

/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/encoding.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>


static void mask_races(xmlNodePtr node, const char *key, int *maskp) {
    xmlChar *propValue = xmlGetProp(node, BAD_CAST key);
    int mask = 0;
    assert(maskp);
    if (propValue) {
        mask = rc_get_mask((char *)propValue);
        xmlFree(propValue);
    }
    *maskp = mask;
}

static variant xml_fraction(xmlNodePtr node, const char *name) {
    xmlChar *propValue = xmlGetProp(node, BAD_CAST name);
    if (propValue != NULL) {
        int num, den = 100;
        double fval = atof((const char *)propValue);
        num = (int)(fval * den + 0.5);
        xmlFree(propValue);
        return frac_make(num, den);
    }
    return frac_make(0, 1);
}

static void xml_readtext(xmlNodePtr node, struct locale **lang, xmlChar ** text)
{
    xmlChar *propValue = xmlGetProp(node, BAD_CAST "locale");
    assert(propValue != NULL);
    *lang = get_locale((const char *)propValue);
    xmlFree(propValue);

    *text = xmlNodeListGetString(node->doc, node->children, 1);
}

static spellref *xml_spellref(xmlNode * node, const char *name)
{
    xmlChar *propValue = xmlGetProp(node, BAD_CAST name);
    if (propValue != NULL) {
        spellref *ref = spellref_create(NULL, (const char *)propValue);
        xmlFree(propValue);
        return ref;
    }
    return NULL;
}

static xmlChar *xml_cleanup_string(xmlChar * str)
{
    xmlChar *read = str;
    xmlChar *write = str;

    while (*read) {
        /* eat leading whitespace */
        if (*read && isspace(*read)) {
            while (*read && isspace(*read)) {
                ++read;
            }
            *write++ = ' ';
        }
        while (*read) {
            if (*read == '\n')
                break;
            if (*read == '\r')
                break;
            *write++ = *read++;
        }
    }
    *write = 0;
    return str;
}

static resource_mod * xml_readmodifiers(xmlXPathObjectPtr result, xmlNodePtr node) {
    /* reading eressea/resources/resource/modifier */
    if (result->nodesetval != NULL && result->nodesetval->nodeNr > 0) {
        int k;
        resource_mod * modifiers =
            calloc(result->nodesetval->nodeNr + 1, sizeof(resource_mod));
        for (k = 0; k != result->nodesetval->nodeNr; ++k) {
            xmlNodePtr node = result->nodesetval->nodeTab[k];
            xmlChar *propValue;
            building_type *btype = NULL;

            mask_races(node, "races", &modifiers[k].race_mask);

            propValue = xmlGetProp(node, BAD_CAST "building");
            if (propValue != NULL) {
                btype = bt_get_or_create((const char *)propValue);
                xmlFree(propValue);
            }
            modifiers[k].btype = btype;

            propValue = xmlGetProp(node, BAD_CAST "type");
            assert(propValue != NULL);
            if (strcmp((const char *)propValue, "skill") == 0) {
                xmlChar *propSkill;
                skill_t sk = NOSKILL;

                modifiers[k].type = RMT_PROD_SKILL;
                propSkill = xmlGetProp(node, BAD_CAST "skill");
                if (propSkill) {
                    sk = findskill((const char *)propSkill);
                    xmlFree(propSkill);
                }
                modifiers[k].value.sa[0] = (short)sk;
                modifiers[k].value.sa[1] = (short)xml_ivalue(node, "value", 0);
            }
            else if (strcmp((const char *)propValue, "material") == 0) {
                modifiers[k].value = xml_fraction(node, "value");
                modifiers[k].type = RMT_PROD_SAVE;
            }
            else {
                if (strcmp((const char *)propValue, "require") == 0) {
                    modifiers[k].type = RMT_PROD_REQUIRE;
                }
                else if (strcmp((const char *)propValue, "save") == 0) {
                    modifiers[k].type = RMT_USE_SAVE;
                    modifiers[k].value = xml_fraction(node, "value");
                }
                else {
                    log_error("unknown type '%s' for resourcelimit-modifier", (const char *)propValue);
                }
            }
            xmlFree(propValue);
        }
        return modifiers;
    }
    return NULL;
}

static void
xml_readrequirements(xmlNodePtr * nodeTab, int nodeNr, requirement ** reqArray)
{
    int req;
    requirement *radd = *reqArray;

    assert(radd == NULL);
    if (nodeNr == 0)
        return;

    radd = *reqArray = calloc(sizeof(requirement), nodeNr + 1);

    for (req = 0; req != nodeNr; ++req) {
        xmlNodePtr node = nodeTab[req];
        xmlChar *propValue;

        radd->number = xml_ivalue(node, "quantity", 1);

        propValue = xmlGetProp(node, BAD_CAST "type");
        radd->rtype = rt_get_or_create((const char *)propValue);
        xmlFree(propValue);

        ++radd;
    }
}

static construction *
xml_readconstruction(xmlXPathContextPtr xpath, xmlNodePtr node)
{
    construction *con;
    xmlChar *propValue;
    xmlXPathObjectPtr req;
    skill_t sk = NOSKILL;

    propValue = xmlGetProp(node, BAD_CAST "skill");
    if (propValue != NULL) {
        sk = findskill((const char *)propValue);
        if (sk == NOSKILL) {
            log_error("construction requires skill '%s' that does not exist.\n", (const char *)propValue);
            xmlFree(propValue);
            return NULL;
        }
        xmlFree(propValue);
    }

    con = (construction *)calloc(sizeof(construction), 1);

    con->skill = sk;
    con->maxsize = xml_ivalue(node, "maxsize", -1);
    con->minskill = xml_ivalue(node, "minskill", -1);
    con->reqsize = xml_ivalue(node, "reqsize", 1);

    /* read construction/requirement */
    xpath->node = node;
    req = xmlXPathEvalExpression(BAD_CAST "requirement", xpath);
    xml_readrequirements(req->nodesetval->nodeTab,
        req->nodesetval->nodeNr, &con->materials);
    xmlXPathFreeObject(req);

    return con;
}

static void
xml_readconstructions(xmlXPathContextPtr xpath, xmlNodeSetPtr nodeSet, building_type *btype)
{
    construction **consPtr = &btype->construction;
    int k;

    for (k = 0; k != nodeSet->nodeNr; ++k) {
        xmlNodePtr node = nodeSet->nodeTab[k];
        construction *con = xml_readconstruction(xpath, node);
        xmlChar *propValue = xmlGetProp(node, BAD_CAST "name");

        if (propValue != NULL) {
            con->name = str_strdup((const char *)propValue);
            xmlFree(propValue);
        }

        if (con) {
            *consPtr = con;
            consPtr = &con->improvement;
        }
    }
}

static int
parse_function(xmlNodePtr node, pf_generic * funPtr, xmlChar ** namePtr)
{
    pf_generic fun;
    xmlChar *propValue = xmlGetProp(node, BAD_CAST "value");
    assert(propValue != NULL);
    fun = get_function((const char *)propValue);
    xmlFree(propValue);
    propValue = xmlGetProp(node, BAD_CAST "name");
    *namePtr = propValue;
    *funPtr = fun;
    return 0;
}

static int parse_buildings(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr buildings;
    int result = 0;

    /* reading eressea/buildings/building */
    buildings =
        xmlXPathEvalExpression(BAD_CAST "/eressea/buildings/building", xpath);
    if (buildings->nodesetval != NULL) {
        xmlNodeSetPtr nodes = buildings->nodesetval;
        int i;
        result += nodes->nodeNr;
        for (i = 0; i != nodes->nodeNr; ++i) {
            xmlNodePtr node = nodes->nodeTab[i];
            xmlChar *propValue;
            building_type *btype;
            xmlXPathObjectPtr result;
            int k;

            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            btype = bt_get_or_create((const char *)propValue);
            xmlFree(propValue);

            btype->capacity = xml_ivalue(node, "capacity", btype->capacity);
            btype->maxcapacity = xml_ivalue(node, "maxcapacity", btype->maxcapacity);
            btype->maxsize = xml_ivalue(node, "maxsize", btype->maxsize);

            btype->magres = frac_make(xml_ivalue(node, "magres", 0), 100);
            btype->magresbonus = xml_ivalue(node, "magresbonus", btype->magresbonus);
            btype->fumblebonus = xml_ivalue(node, "fumblebonus", btype->fumblebonus);
            btype->auraregen = xml_fvalue(node, "auraregen", btype->auraregen);
            btype->taxes = xml_ivalue(node, "taxes", btype->taxes);

            if (xml_bvalue(node, "nodestroy", false))
                btype->flags |= BTF_INDESTRUCTIBLE;
            if (xml_bvalue(node, "oneperturn", false))
                btype->flags |= BTF_ONEPERTURN;
            if (xml_bvalue(node, "nobuild", false))
                btype->flags |= BTF_NOBUILD;
            if (xml_bvalue(node, "namechange", true))
                btype->flags |= BTF_NAMECHANGE;
            if (xml_bvalue(node, "unique", false))
                btype->flags |= BTF_UNIQUE;
            if (xml_bvalue(node, "decay", false))
                btype->flags |= BTF_DECAY;
            if (xml_bvalue(node, "magic", false))
                btype->flags |= BTF_MAGIC;
            if (xml_bvalue(node, "fort", false))
                btype->flags |= BTF_FORTIFICATION;

            /* reading eressea/buildings/building/modifier */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "modifier", xpath);
            btype->modifiers = xml_readmodifiers(result, node);
            xmlXPathFreeObject(result);

            /* reading eressea/buildings/building/construction */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "construction", xpath);
            xml_readconstructions(xpath, result->nodesetval, btype);
            xmlXPathFreeObject(result);

            /* reading eressea/buildings/building/function */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
            for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                xmlNodePtr node = result->nodesetval->nodeTab[k];
                pf_generic fun;
                parse_function(node, &fun, &propValue);

                if (fun == NULL) {
                    log_error("unknown function name '%s' for building %s\n", (const char *)propValue, btype->_name);
                    xmlFree(propValue);
                    continue;
                }
                assert(propValue != NULL);
                log_error("unknown function type '%s' for building %s\n", (const char *)propValue, btype->_name);
                xmlFree(propValue);
            }
            xmlXPathFreeObject(result);

            /* reading eressea/buildings/building/maintenance */
            result = xmlXPathEvalExpression(BAD_CAST "maintenance", xpath);
            for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                xmlNodePtr node = result->nodesetval->nodeTab[k];
                maintenance *mt;

                if (btype->maintenance == NULL) {
                    btype->maintenance = (struct maintenance *)
                        calloc(sizeof(struct maintenance), result->nodesetval->nodeNr + 1);
                }
                mt = btype->maintenance + k;
                mt->number = xml_ivalue(node, "amount", 0);

                propValue = xmlGetProp(node, BAD_CAST "type");
                assert(propValue != NULL);
                mt->rtype = rt_find((const char *)propValue);
                assert(mt->rtype != NULL);
                xmlFree(propValue);

                if (xml_bvalue(node, "variable", false))
                    mt->flags |= MTF_VARIABLE;
            }
            xmlXPathFreeObject(result);
        }
    }
    xmlXPathFreeObject(buildings);

    xmlXPathFreeContext(xpath);
    return result;
}

static int parse_ships(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr ships;
    int result = 0;

    /* reading eressea/ships/ship */
    ships = xmlXPathEvalExpression(BAD_CAST "/eressea/ships/ship", xpath);
    if (ships->nodesetval != NULL) {
        xmlNodeSetPtr nodes = ships->nodesetval;
        int i;
        result += nodes->nodeNr;
        for (i = 0; i != nodes->nodeNr; ++i) {
            xmlNodePtr child, node = nodes->nodeTab[i];
            xmlChar *propValue;
            ship_type *st;
            xmlXPathObjectPtr result;
            int k, c;

            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            st = st_get_or_create((const char *)propValue);
            xmlFree(propValue);

            st->cabins = xml_ivalue(node, "cabins", 0) * PERSON_WEIGHT;
            st->cargo = xml_ivalue(node, "cargo", st->cargo);
            st->combat = xml_ivalue(node, "combat", st->combat);
            st->damage = xml_fvalue(node, "damage", st->damage);
            if (xml_bvalue(node, "nocoast", false))
                st->flags |= SFL_NOCOAST;
            if (xml_bvalue(node, "fly", false))
                st->flags |= SFL_FLY;
            if (xml_bvalue(node, "opensea", false))
                st->flags |= SFL_OPENSEA;
            if (xml_bvalue(node, "speedy", false))
                st->flags |= SFL_SPEEDY;
            st->fishing = xml_ivalue(node, "fishing", st->fishing);
            st->cptskill = xml_ivalue(node, "cptskill", st->cptskill);
            st->minskill = xml_ivalue(node, "minskill", st->minskill);
            st->sumskill = xml_ivalue(node, "sumskill", st->sumskill);
            st->range = xml_ivalue(node, "range", st->range);
            st->range_max = xml_ivalue(node, "maxrange", st->range_max);
            st->storm = xml_fvalue(node, "storm", st->storm);

            for (child = node->children; child; child = child->next) {
                if (strcmp((const char *)child->name, "modifier") == 0) {
                    double value = xml_fvalue(child, "value", 0.0);
                    propValue = xmlGetProp(child, BAD_CAST "type");
                    if (strcmp((const char *)propValue, "tactics") == 0)
                        st->tac_bonus = (float)value;
                    else if (strcmp((const char *)propValue, "attack") == 0)
                        st->at_bonus = (int)value;
                    else if (strcmp((const char *)propValue, "defense") == 0)
                        st->df_bonus = (int)value;
                    xmlFree(propValue);
                }
                else if (strcmp((const char *)child->name, "construction") == 0) {
                    assert(!st->construction);
                    st->construction = xml_readconstruction(xpath, child);
                }
            }
            /* reading eressea/ships/ship/coast */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "coast", xpath);
            for (c = 0, k = 0; k != result->nodesetval->nodeNr; ++k) {
                xmlNodePtr node = result->nodesetval->nodeTab[k];

                if (k == 0) {
                    assert(st->coasts == NULL);
                    st->coasts =
                        (terrain_type **)malloc(sizeof(terrain_type *) *
                        (result->nodesetval->nodeNr + 1));
                    st->coasts[result->nodesetval->nodeNr] = NULL;
                }

                propValue = xmlGetProp(node, BAD_CAST "terrain");
                assert(propValue != NULL);
                st->coasts[c] = get_or_create_terrain((const char *)propValue);
                if (st->coasts[c] != NULL)
                    ++c;
                else {
                    log_warning("ship %s mentions a non-existing terrain %s.\n", st->_name, propValue);
                }
                xmlFree(propValue);
            }
            xmlXPathFreeObject(result);
        }
    }
    xmlXPathFreeObject(ships);

    xmlXPathFreeContext(xpath);
    return result;
}

static void xml_readpotion(xmlNodePtr node, item_type * itype)
{
    int level = xml_ivalue(node, "level", 0);

    if ((itype->flags & ITF_CANUSE) == 0) {
        log_error("potion %s has no use attribute", itype->rtype->_name);
        itype->flags |= ITF_CANUSE;
    }
    new_potiontype(itype, level);
}

static luxury_type *xml_readluxury(xmlNodePtr node, item_type * itype)
{
    int price = xml_ivalue(node, "price", 0);
    return new_luxurytype(itype, price);
}

static armor_type *xml_readarmor(xmlNodePtr node, item_type * itype)
{
    armor_type *atype = NULL;
    unsigned int flags = ATF_NONE;
    int ac = xml_ivalue(node, "ac", 0);
    double penalty = xml_fvalue(node, "penalty", 0.0);
    variant magres = xml_fraction(node, "magres");

    if (xml_bvalue(node, "laen", false))
        flags |= ATF_LAEN;
    if (xml_bvalue(node, "shield", false))
        flags |= ATF_SHIELD;

    atype = new_armortype(itype, penalty, magres, ac, flags);
    atype->projectile = (float)xml_fvalue(node, "projectile", 0.0);
    return atype;
}

static weapon_type *xml_readweapon(xmlXPathContextPtr xpath, item_type * itype)
{
    xmlNodePtr node = xpath->node;
    weapon_type *wtype = NULL;
    unsigned int flags = WTF_NONE;
    xmlXPathObjectPtr result;
    xmlChar *propValue;
    int k;
    skill_t sk;
    int offmod = xml_ivalue(node, "offmod", 0);
    int defmod = xml_ivalue(node, "defmod", 0);
    int reload = xml_ivalue(node, "reload", 0);
    variant magres = xml_fraction(node, "magres");

    if (xml_bvalue(node, "armorpiercing", false))
        flags |= WTF_ARMORPIERCING;
    if (xml_bvalue(node, "magical", false))
        flags |= WTF_MAGICAL;
    if (xml_bvalue(node, "missile", false))
        flags |= WTF_MISSILE;
    if (xml_bvalue(node, "pierce", false))
        flags |= WTF_PIERCE;
    if (xml_bvalue(node, "cut", false))
        flags |= WTF_CUT;
    if (xml_bvalue(node, "blunt", false))
        flags |= WTF_BLUNT;
    if (xml_bvalue(node, "siege", false))
        flags |= WTF_SIEGE;
    if (xml_bvalue(node, "horse", (flags & WTF_MISSILE) == 0))
        flags |= WTF_HORSEBONUS;
    if (xml_bvalue(node, "useshield", true))
        flags |= WTF_USESHIELD;

    propValue = xmlGetProp(node, BAD_CAST "skill");
    assert(propValue != NULL);
    sk = findskill((const char *)propValue);
    assert(sk != NOSKILL);
    xmlFree(propValue);

    wtype =
        new_weapontype(itype, flags, magres, NULL, offmod, defmod, reload, sk);

    /* reading weapon/damage */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "damage", xpath);
    assert(result->nodesetval->nodeNr <= 2);
    for (k = 0; k != result->nodesetval->nodeNr; ++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        int pos = 0;

        propValue = xmlGetProp(node, BAD_CAST "type");
        if (strcmp((const char *)propValue, "footman") != 0) {
            pos = 1;
        }
        xmlFree(propValue);

        propValue = xmlGetProp(node, BAD_CAST "value");
        wtype->damage[pos] = str_strdup((const char *)propValue); /* TODO: this is a memory leak */
        if (k == 0)
            wtype->damage[1 - pos] = wtype->damage[pos];
        xmlFree(propValue);
    }
    xmlXPathFreeObject(result);

    /* reading weapon/modifier */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "modifier", xpath);
    assert(wtype->modifiers == NULL);
    wtype->modifiers = calloc(result->nodesetval->nodeNr + 1, sizeof(weapon_mod));
    for (k = 0; k != result->nodesetval->nodeNr; ++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        int flags = 0;

        if (xml_bvalue(node, "walking", false))
            flags |= WMF_WALKING;
        if (xml_bvalue(node, "riding", false))
            flags |= WMF_RIDING;
        if (xml_bvalue(node, "against_walking", false))
            flags |= WMF_AGAINST_WALKING;
        if (xml_bvalue(node, "against_riding", false))
            flags |= WMF_AGAINST_RIDING;
        if (xml_bvalue(node, "offensive", false))
            flags |= WMF_OFFENSIVE;
        if (xml_bvalue(node, "defensive", false))
            flags |= WMF_DEFENSIVE;

        propValue = xmlGetProp(node, BAD_CAST "type");
        if (strcmp((const char *)propValue, "damage") == 0)
            flags |= WMF_DAMAGE;
        else if (strcmp((const char *)propValue, "skill") == 0)
            flags |= WMF_SKILL;
        else if (strcmp((const char *)propValue, "missile_target") == 0)
            flags |= WMF_MISSILE_TARGET;
        xmlFree(propValue);

        wtype->modifiers[k].flags = flags;
        wtype->modifiers[k].value = xml_ivalue(node, "value", 0);

        mask_races(node, "races", &wtype->modifiers[k].race_mask);
        wtype->modifiers[k].race_mask = 0;
    }
    xmlXPathFreeObject(result);

    /* reading weapon/function */
    xpath->node = node;
    result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
    for (k = 0; k != result->nodesetval->nodeNr; ++k) {
        xmlNodePtr node = result->nodesetval->nodeTab[k];
        xmlChar *propValue;
        pf_generic fun;

        parse_function(node, &fun, &propValue);
        if (fun == NULL) {
            log_error("unknown function name '%s' for item '%s'\n", (const char *)propValue, itype->rtype->_name);
            xmlFree(propValue);
            continue;
        }
        assert(propValue != NULL);
        if (strcmp((const char *)propValue, "attack") == 0) {
            wtype->attack =
                (bool(*)(const struct troop *, const struct weapon_type *,
                int *))fun;
        }
        else {
            log_error("unknown function type '%s' for item '%s'\n", (const char *)propValue, itype->rtype->_name);
        }
        xmlFree(propValue);
    }
    xmlXPathFreeObject(result);

    xpath->node = node;
    return wtype;
}

static item_type *xml_readitem(xmlXPathContextPtr xpath, resource_type * rtype)
{
    xmlNodePtr child, node = xpath->node;
    item_type *itype = NULL;
    unsigned int flags = ITF_NONE;

    if (xml_bvalue(node, "cursed", false))
        flags |= ITF_CURSED;
    if (xml_bvalue(node, "use", false))
        flags |= ITF_CANUSE;
    if (xml_bvalue(node, "notlost", false))
        flags |= ITF_NOTLOST;
    if (xml_bvalue(node, "herb", false))
        flags |= ITF_HERB;
    if (xml_bvalue(node, "big", false))
        flags |= ITF_BIG;
    if (xml_bvalue(node, "animal", false))
        flags |= ITF_ANIMAL;
    if (xml_bvalue(node, "vehicle", false))
        flags |= ITF_VEHICLE;
    itype = rtype->itype ? rtype->itype : it_get_or_create(rtype);
    itype->weight = xml_ivalue(node, "weight", 0);
    itype->capacity = xml_ivalue(node, "capacity", 0);
    itype->score = xml_ivalue(node, "score", 0);

    mask_races(node, "allow", &itype->mask_allow);
    mask_races(node, "deny", &itype->mask_deny);
    itype->flags |= flags;

    for (child = node->children; child; child = child->next) {
        if (strcmp((const char *)child->name, "construction") == 0) {
            /* reading item/construction */
            assert(!itype->construction);
            xpath->node = child;
            itype->construction = xml_readconstruction(xpath, child);
        }
        else if (strcmp((const char *)child->name, "weapon") == 0) {
            /* reading item/weapon */
            assert(!rtype->wtype);
            xpath->node = child;
            rtype->wtype = xml_readweapon(xpath, itype);
        }
        else if (strcmp((const char *)child->name, "armor") == 0) {
            /* reading item/weapon */
            assert(!rtype->atype);
            rtype->atype = xml_readarmor(child, itype);
        }
        else if (strcmp((const char *)child->name, "luxury") == 0) {
            /* reading item/luxury */
            assert(!rtype->ltype);
            rtype->ltype = xml_readluxury(child, itype);
        }
        else if (strcmp((const char *)child->name, "potion") == 0) {
            /* reading item/potion */
            xml_readpotion(child, itype);
        }
    }

    if (!itype->score) {
        /* do this last, because score depends on itype data */
        itype->score = default_score(itype);
    }
    return itype;
}

static int parse_resources(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr resources;
    int results = 0;

    /* reading eressea/resources/resource */
    resources =
        xmlXPathEvalExpression(BAD_CAST "/eressea/resources/resource", xpath);
    if (resources->nodesetval) {
        int i;
        xmlNodeSetPtr nodes = resources->nodesetval;
        results = nodes->nodeNr;
        for (i = 0; i != nodes->nodeNr; ++i) {
            xmlNodePtr node = nodes->nodeTab[i];
            xmlChar *propValue, *name, *appearance;
            resource_type *rtype;
            unsigned int flags = RTF_NONE;
            xmlXPathObjectPtr result;

            if (xml_bvalue(node, "pooled", true))
                flags |= RTF_POOLED;

            name = xmlGetProp(node, BAD_CAST "name");
            if (!name) {
                assert(name);
                log_error("invalid resource %d has no name", i);
                continue;
            }
            rtype = rt_get_or_create((const char *)name);
            rtype->flags |= flags;
            xmlFree(name);

            /* reading eressea/resources/resource/function */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "function", xpath);
            if (result->nodesetval != NULL) {
                int k;
                for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                    xmlNodePtr node = result->nodesetval->nodeTab[k];
                    pf_generic fun;

                    parse_function(node, &fun, &propValue);
                    if (fun == NULL) {
                        log_error("unknown function name '%s' for resource %s\n", (const char *)propValue, rtype->_name);
                        xmlFree(propValue);
                        continue;
                    }

                    assert(propValue != NULL);
                    if (strcmp((const char *)propValue, "change") == 0) {
                        rtype->uchange = (rtype_uchange)fun;
                    }
                    else if (strcmp((const char *)propValue, "name") == 0) {
                        rtype->name = (rtype_name)fun;
                    }
                    else {
                        log_error("unknown function type '%s' for resource %s\n", (const char *)propValue, rtype->_name);
                    }
                    xmlFree(propValue);
                }
            }
            xmlXPathFreeObject(result);

            if (xml_bvalue(node, "material", false)) {
                rmt_create(rtype);
            }

            if (xml_bvalue(node, "limited", false)) {
                rtype->flags |= RTF_LIMITED;
            }
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "modifier", xpath);
            rtype->modifiers = xml_readmodifiers(result, node);
            xmlXPathFreeObject(result);

            /* reading eressea/resources/resource/item */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "item", xpath);
            assert(result->nodesetval->nodeNr <= 1);
            if (result->nodesetval->nodeNr != 0) {
                rtype->flags |= RTF_ITEM;
                xpath->node = result->nodesetval->nodeTab[0];
                rtype->itype = xml_readitem(xpath, rtype);
                appearance = xmlGetProp(node, BAD_CAST "appearance");
                if (appearance) {
                    it_set_appearance(rtype->itype, (const char *)appearance);
                    xmlFree(appearance);
                }
            }
            xmlXPathFreeObject(result);
        }
    }
    xmlXPathFreeObject(resources);

    xmlXPathFreeContext(xpath);

    /* make sure old items (used in requirements) are available */
    init_resources();

    return results;
}

static void add_items(equipment * eq, xmlNodeSetPtr nsetItems)
{
    if (nsetItems != NULL && nsetItems->nodeNr > 0) {
        int i;
        for (i = 0; i != nsetItems->nodeNr; ++i) {
            xmlNodePtr node = nsetItems->nodeTab[i];
            xmlChar *propValue;
            const struct item_type *itype;

            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            itype = it_find((const char *)propValue);
            xmlFree(propValue);
            if (itype != NULL) {
                propValue = xmlGetProp(node, BAD_CAST "amount");
                if (propValue != NULL) {
                    equipment_setitem(eq, itype, (const char *)propValue);
                    xmlFree(propValue);
                }
            }
        }
    }
}

static void add_callbacks(equipment * eq, xmlNodeSetPtr nsetItems)
{
    if (nsetItems != NULL && nsetItems->nodeNr > 0) {
        int i;
        for (i = 0; i != nsetItems->nodeNr; ++i) {
            xmlNodePtr node = nsetItems->nodeTab[i];
            xmlChar *propValue;
            pf_generic fun;

            propValue = xmlGetProp(node, BAD_CAST "name");
            if (propValue != NULL) {
                fun = get_function((const char *)propValue);
                if (fun) {
                    equipment_setcallback(eq, (void(*)(const struct equipment *,
                    struct unit *))fun);
                }
                xmlFree(propValue);
            }
        }
    }
}

static void add_spells(equipment * eq, xmlNodeSetPtr nsetItems)
{
    if (nsetItems != NULL && nsetItems->nodeNr > 0) {
        int i;
        for (i = 0; i != nsetItems->nodeNr; ++i) {
            xmlNodePtr node = nsetItems->nodeTab[i];
            xmlChar *propValue;
            int level;
            const char *name;

            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            name = (const char *)propValue;
            level = xml_ivalue(node, "level", 0);
            if (level > 0) {
                equipment_addspell(eq, name, level);
            }
            else {
                log_error("spell '%s' for equipment-set '%s' has no level\n", name, equipment_name(eq));
            }
            xmlFree(propValue);
        }
    }
}

static void add_skills(equipment * eq, xmlNodeSetPtr nsetSkills)
{
    if (nsetSkills != NULL && nsetSkills->nodeNr > 0) {
        int i;
        for (i = 0; i != nsetSkills->nodeNr; ++i) {
            xmlNodePtr node = nsetSkills->nodeTab[i];
            xmlChar *propValue;
            skill_t sk;

            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            sk = findskill((const char *)propValue);
            if (sk == NOSKILL) {
                log_error("unknown skill '%s' in equipment-set %s\n", (const char *)propValue, equipment_name(eq));
                xmlFree(propValue);
            }
            else {
                xmlFree(propValue);
                propValue = xmlGetProp(node, BAD_CAST "level");
                if (propValue != NULL) {
                    equipment_setskill(eq, sk, (const char *)propValue);
                    xmlFree(propValue);
                }
            }
        }
    }
}

static void
add_subsets(xmlDocPtr doc, equipment * eq, xmlNodeSetPtr nsetSubsets)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    if (nsetSubsets != NULL && nsetSubsets->nodeNr > 0) {
        int i;

        eq->subsets = calloc(nsetSubsets->nodeNr + 1, sizeof(subset));
        for (i = 0; i != nsetSubsets->nodeNr; ++i) {
            xmlXPathObjectPtr xpathResult;
            xmlNodePtr node = nsetSubsets->nodeTab[i];
            xmlChar *propValue;

            eq->subsets[i].chance = 1.0f;
            propValue = xmlGetProp(node, BAD_CAST "chance");
            if (propValue != NULL) {
                eq->subsets[i].chance = (float)atof((const char *)propValue);
                xmlFree(propValue);
            }
            xpath->node = node;
            xpathResult = xmlXPathEvalExpression(BAD_CAST "set", xpath);
            if (xpathResult->nodesetval) {
                xmlNodeSetPtr nsetSets = xpathResult->nodesetval;
                float totalChance = 0.0f;

                if (nsetSets->nodeNr > 0) {
                    int set;
                    eq->subsets[i].sets =
                        calloc(nsetSets->nodeNr + 1, sizeof(subsetitem));
                    for (set = 0; set != nsetSets->nodeNr; ++set) {
                        xmlNodePtr nodeSet = nsetSets->nodeTab[set];
                        float chance = 1.0f;

                        propValue = xmlGetProp(nodeSet, BAD_CAST "chance");
                        if (propValue != NULL) {
                            chance = (float)atof((const char *)propValue);
                            xmlFree(propValue);
                        }
                        totalChance += chance;

                        propValue = xmlGetProp(nodeSet, BAD_CAST "name");
                        assert(propValue != NULL);
                        eq->subsets[i].sets[set].chance = chance;
                        eq->subsets[i].sets[set].set =
                            get_or_create_equipment((const char *)propValue);
                        xmlFree(propValue);
                    }
                }
                if (totalChance > 1.0f) {
                    log_error("total chance exceeds 1.0: %f in equipment set %s.\n", totalChance, equipment_name(eq));
                }
            }
            xmlXPathFreeObject(xpathResult);
        }
    }
    xmlXPathFreeContext(xpath);
}

static int parse_equipment(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr xpathRaces;
    int result = 0;

    /* reading eressea/equipment/set */
    xpathRaces = xmlXPathEvalExpression(BAD_CAST "/eressea/equipment/set", xpath);
    if (xpathRaces->nodesetval) {
        xmlNodeSetPtr nsetRaces = xpathRaces->nodesetval;
        int i;

        result += nsetRaces->nodeNr;
        for (i = 0; i != nsetRaces->nodeNr; ++i) {
            xmlNodePtr node = nsetRaces->nodeTab[i];
            xmlChar *propName = xmlGetProp(node, BAD_CAST "name");

            if (propName != NULL) {
                equipment *eq = get_equipment((const char *)propName);
                xmlXPathObjectPtr xpathResult;

                if (!eq) {
                    eq = create_equipment((const char *)propName);
                }

                xpath->node = node;

                xpathResult = xmlXPathEvalExpression(BAD_CAST "callback", xpath);
                assert(!eq->callback);
                add_callbacks(eq, xpathResult->nodesetval);
                xmlXPathFreeObject(xpathResult);

                xpathResult = xmlXPathEvalExpression(BAD_CAST "item", xpath);
                assert(!eq->items);
                add_items(eq, xpathResult->nodesetval);
                xmlXPathFreeObject(xpathResult);

                xpathResult = xmlXPathEvalExpression(BAD_CAST "spell", xpath);
                assert(!eq->spells);
                add_spells(eq, xpathResult->nodesetval);
                xmlXPathFreeObject(xpathResult);

                xpathResult = xmlXPathEvalExpression(BAD_CAST "skill", xpath);
                add_skills(eq, xpathResult->nodesetval);
                xmlXPathFreeObject(xpathResult);

                xpathResult = xmlXPathEvalExpression(BAD_CAST "subset", xpath);
                assert(!eq->subsets);
                add_subsets(doc, eq, xpathResult->nodesetval);
                xmlXPathFreeObject(xpathResult);

                xmlFree(propName);
            }
        }
    }

    xmlXPathFreeObject(xpathRaces);
    xmlXPathFreeContext(xpath);

    return result;
}

static int parse_spellbooks(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr spellbooks;
    int results = 0;

    /* reading eressea/spells/spell */
    spellbooks = xmlXPathEvalExpression(BAD_CAST "/eressea/spellbook", xpath);

    if (spellbooks->nodesetval != NULL) {
        xmlNodeSetPtr nodes = spellbooks->nodesetval;
        int i, k;

        results += nodes->nodeNr;
        for (i = 0; i != nodes->nodeNr; ++i) {
            xmlXPathObjectPtr result;
            xmlNodePtr node = nodes->nodeTab[i];
            xmlChar *propValue;
            spellbook * sb;

            propValue = xmlGetProp(node, BAD_CAST "name");
            if (propValue) {
                sb = get_spellbook((const char *)propValue);
                xmlFree(propValue);
            }
            else {
                log_error("spellbook at index '%d' has n name\n", i);
                continue;
            }

            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "entry", xpath);

            if (result->nodesetval->nodeNr > 0) {
                for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                    xmlNodePtr node = result->nodesetval->nodeTab[k];
                    spell * sp = 0;
                    int level = xml_ivalue(node, "level", -1);

                    propValue = xmlGetProp(node, BAD_CAST "spell");
                    if (propValue) {
                        sp = find_spell((const char *)propValue);
                        xmlFree(propValue);
                    }
                    if (sp && level > 0) {
                        spellbook_add(sb, sp, level);
                    }
                    else {
                        log_error("invalid entry at index '%d' in spellbook '%s'\n", k, sb->name);
                    }
                }
            }
            xmlXPathFreeObject(result);
        }
    }
    xmlXPathFreeObject(spellbooks);
    xmlXPathFreeContext(xpath);
    return results;
}

static int parse_spells(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr spells;
    int result = 0;

    /* reading eressea/spells/spell */
    spells = xmlXPathEvalExpression(BAD_CAST "/eressea/spells/spell", xpath);
    if (spells->nodesetval != NULL) {
        xmlNodeSetPtr nodes = spells->nodesetval;
        int i;

        result += nodes->nodeNr;
        for (i = 0; i != nodes->nodeNr; ++i) {
            xmlXPathObjectPtr result;
            xmlNodePtr node = nodes->nodeTab[i];
            xmlChar *propValue;
            int k;
            spell_component *component;
            spell *sp;
            static int modes[] = { 0, PRECOMBATSPELL, COMBATSPELL, POSTCOMBATSPELL };

            /* spellname */
            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            sp = create_spell((const char *)propValue);
            xmlFree(propValue);
            if (!sp) {
                continue;
            }

            propValue = xmlGetProp(node, BAD_CAST "parameters");
            if (propValue) {
                sp->parameter = str_strdup((const char *)propValue);
                xmlFree(propValue);
            }

            propValue = xmlGetProp(node, BAD_CAST "syntax");
            if (propValue) {
                sp->syntax = str_strdup((const char *)propValue);
                xmlFree(propValue);
            }
#ifdef TODO /* no longer need it, spellbooks! */
            /* magic type */
            propValue = xmlGetProp(node, BAD_CAST "type");
            assert(propValue != NULL);
            for (sp->magietyp = 0; sp->magietyp != MAXMAGIETYP; ++sp->magietyp) {
                if (strcmp(magic_school[sp->magietyp], (const char *)propValue) == 0)
                    break;
            }
            assert(sp->magietyp != MAXMAGIETYP);
            xmlFree(propValue);
            /* level, rank and flags */
#endif
            sp->rank = (char)xml_ivalue(node, "rank", -1);
            if (xml_bvalue(node, "los", false))
                sp->sptyp |= TESTCANSEE;        /* must see or have contact */
            if (!xml_bvalue(node, "target_global", false))
                sp->sptyp |= SEARCHLOCAL;       /* must be in same region */
            if (xml_bvalue(node, "ship", false))
                sp->sptyp |= ONSHIPCAST;
            if (xml_bvalue(node, "ocean", false))
                sp->sptyp |= OCEANCASTABLE;
            if (xml_bvalue(node, "far", false))
                sp->sptyp |= FARCASTING;
            if (xml_bvalue(node, "variable", false))
                sp->sptyp |= SPELLLEVEL;

            if (xml_bvalue(node, "buildingtarget", false))
                sp->sptyp |= BUILDINGSPELL;
            if (xml_bvalue(node, "shiptarget", false))
                sp->sptyp |= SHIPSPELL;
            if (xml_bvalue(node, "unittarget", false))
                sp->sptyp |= UNITSPELL;
            if (xml_bvalue(node, "regiontarget", false))
                sp->sptyp |= REGIONSPELL;

            k = xml_ivalue(node, "combat", 0);
            if (k >= 0 && k <= 3) {
                sp->sptyp |= modes[k];
            }
            /* reading eressea/spells/spell/resource */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "resource", xpath);
            if (result->nodesetval->nodeNr) {
                sp->components =
                    (spell_component *)malloc(sizeof(spell_component) *
                    (result->nodesetval->nodeNr + 1));
                sp->components[result->nodesetval->nodeNr].type = 0;
            }
            for (component = sp->components, k = 0; k != result->nodesetval->nodeNr;
                ++k) {
                const resource_type *rtype;
                xmlNodePtr node = result->nodesetval->nodeTab[k];
                propValue = xmlGetProp(node, BAD_CAST "name");
                assert(propValue);
                rtype = rt_find((const char *)propValue);
                if (!rtype) {
                    log_error("spell %s uses unknown component %s.\n", sp->sname, (const char *)propValue);
                    xmlFree(propValue);
                    continue;
                }
                component->type = rtype;
                xmlFree(propValue);
                component->amount = xml_ivalue(node, "amount", 1);
                component->cost = SPC_FIX;
                propValue = xmlGetProp(node, BAD_CAST "cost");
                if (propValue != NULL) {
                    if (strcmp((const char *)propValue, "linear") == 0) {
                        if ((sp->sptyp&SPELLLEVEL) == 0) {
                            log_error("spell '%s' has linear cost but fixed level\n", sp->sname);
                        }
                        component->cost = SPC_LINEAR;
                    }
                    else if (strcmp((const char *)propValue, "level") == 0) {
                        if ((sp->sptyp&SPELLLEVEL) == 0) {
                            log_error("spell '%s' has levelled cost but fixed level\n", sp->sname);
                        }
                        component->cost = SPC_LEVEL;
                    }
                    xmlFree(propValue);
                }
                component++;
            }
            xmlXPathFreeObject(result);
        }
    }

    xmlXPathFreeObject(spells);

    xmlXPathFreeContext(xpath);

    return result;
}

static void parse_ai(race * rc, xmlNodePtr node)
{
    xmlChar *propValue;
    
    propValue = xmlGetProp(node, BAD_CAST "scare");
    if (propValue) {
        rc_set_param(rc, "ai.scare", (const char *)propValue);
        xmlFree(propValue);
    }
    rc->splitsize = xml_ivalue(node, "splitsize", 0);
    if (xml_bvalue(node, "killpeasants", false))
        rc->flags |= RCF_KILLPEASANTS;
    if (xml_bvalue(node, "moverandom", false))
        rc->flags |= RCF_MOVERANDOM;
    if (xml_bvalue(node, "learn", false))
        rc->flags |= RCF_LEARN;
    if (xml_bvalue(node, "moveattack", false))
        rc->flags |= RCF_ATTACK_MOVED;
}

static void set_study_speed(race *rc, skill_t sk, int modifier) {
    if (!rc->study_speed)
        rc->study_speed = calloc(1, MAXSKILLS);
    rc->study_speed[sk] = (char)modifier;
}

static int parse_races(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr races;
    int results = 0;

    /* reading eressea/races/race */
    races = xmlXPathEvalExpression(BAD_CAST "/eressea/races/race", xpath);
    if (races->nodesetval) {
        xmlNodeSetPtr nodes = races->nodesetval;
        int i;
        results += nodes->nodeNr;
        for (i = 0; i != nodes->nodeNr; ++i) {
            xmlNodePtr node = nodes->nodeTab[i];
            xmlNodePtr child;
            xmlChar *propValue;
            race *rc, *frc = 0;
            xmlXPathObjectPtr result;
            int k, study_speed_base, attacks;
            struct att *attack;
            skill_t sk;

            propValue = xmlGetProp(node, BAD_CAST "name");
            assert(propValue != NULL);
            rc = rc_get_or_create((const char *)propValue);
            xmlFree(propValue);

            propValue = xmlGetProp(node, BAD_CAST "damage");
            assert(propValue != NULL);
            rc->def_damage = str_strdup((const char *)propValue);
            xmlFree(propValue);

            rc->magres = frac_make(xml_ivalue(node, "magres", 100), 100);
            rc->healing = (int)(xml_fvalue(node, "healing", rc->healing) * 100); /* TODO: store as int in XML */
            rc->maxaura = (int)(xml_fvalue(node, "maxaura", rc->maxaura) * 100); /* TODO: store as int in XML */
            rc->regaura = (float)xml_fvalue(node, "regaura", rc->regaura);
            rc->recruitcost = xml_ivalue(node, "recruitcost", rc->recruitcost);
            rc->maintenance = xml_ivalue(node, "maintenance", rc->maintenance);
            rc->weight = xml_ivalue(node, "weight", rc->weight);
            rc->capacity = xml_ivalue(node, "capacity", rc->capacity);
            rc->income = xml_ivalue(node, "income", rc->income);
            rc->speed = (float)xml_fvalue(node, "speed", rc->speed);
            rc->hitpoints = xml_ivalue(node, "hp", rc->hitpoints);
            rc->armor = (char)xml_ivalue(node, "ac", rc->armor);
            study_speed_base = xml_ivalue(node, "studyspeed", 0);
            if (study_speed_base != 0) {
                for (sk = 0; sk < MAXSKILLS; ++sk) {
                    set_study_speed(rc, sk, study_speed_base);
                }
            }

            rc->at_default = (char)xml_ivalue(node, "unarmedattack", -2);
            rc->df_default = (char)xml_ivalue(node, "unarmeddefense", -2);
            rc->at_bonus = (char)xml_ivalue(node, "attackmodifier", rc->at_bonus);
            rc->df_bonus = (char)xml_ivalue(node, "defensemodifier", rc->df_bonus);

            if (!xml_bvalue(node, "playerrace", false)) {
                assert(rc->recruitcost == 0);
                rc->flags |= RCF_NPC;
            }
            if (xml_bvalue(node, "scarepeasants", false))
                rc->flags |= RCF_SCAREPEASANTS;
            if (!xml_bvalue(node, "cansteal", true))
                rc->flags |= RCF_NOSTEAL;
            if (xml_bvalue(node, "cansail", true))
                rc->flags |= RCF_CANSAIL;
            if (xml_bvalue(node, "cannotmove", false))
                rc->flags |= RCF_CANNOTMOVE;
            if (xml_bvalue(node, "fly", false))
                rc->flags |= RCF_FLY;
            if (xml_bvalue(node, "invisible", false))
                rc->flags |= RCF_INVISIBLE;
            if (xml_bvalue(node, "coastal", false))
                rc->flags |= RCF_COASTAL;
            if (xml_bvalue(node, "unarmedguard", false))
                rc->flags |= RCF_UNARMEDGUARD;
            if (xml_bvalue(node, "swim", false))
                rc->flags |= RCF_SWIM;
            if (xml_bvalue(node, "walk", false))
                rc->flags |= RCF_WALK;
            if (!xml_bvalue(node, "canlearn", true))
                rc->flags |= RCF_NOLEARN;
            if (!xml_bvalue(node, "canteach", true))
                rc->flags |= RCF_NOTEACH;
            if (xml_bvalue(node, "horse", false))
                rc->flags |= RCF_HORSE;
            if (xml_bvalue(node, "desert", false))
                rc->flags |= RCF_DESERT;
            if (xml_bvalue(node, "absorbpeasants", false))
                rc->flags |= RCF_ABSORBPEASANTS;
            if (xml_bvalue(node, "noheal", false))
                rc->flags |= RCF_NOHEAL;
            if (xml_bvalue(node, "noweapons", false))
                rc->flags |= RCF_NOWEAPONS;
            if (xml_bvalue(node, "shapeshift", false))
                rc->flags |= RCF_SHAPESHIFT;
            if (xml_bvalue(node, "shapeshiftany", false))
                rc->flags |= RCF_SHAPESHIFTANY;
            if (xml_bvalue(node, "illusionary", false))
                rc->flags |= RCF_ILLUSIONARY;
            if (xml_bvalue(node, "undead", false))
                rc->flags |= RCF_UNDEAD;
            if (xml_bvalue(node, "dragon", false))
                rc->flags |= RCF_DRAGON;
            if (xml_bvalue(node, "shipspeed", false))
                rc->flags |= RCF_SHIPSPEED;

            if (xml_bvalue(node, "giveperson", false))
                rc->ec_flags |= ECF_GIVEPERSON;
            if (xml_bvalue(node, "giveunit", false))
                rc->ec_flags |= ECF_GIVEUNIT;
            if (xml_bvalue(node, "getitem", false))
                rc->ec_flags |= ECF_GETITEM;
            if (xml_bvalue(node, "recruitethereal", false))
                rc->ec_flags |= ECF_REC_ETHEREAL;
            if (xml_bvalue(node, "recruitunlimited", false))
                rc->ec_flags |= ECF_REC_UNLIMITED;
            if (xml_bvalue(node, "stonegolem", false))
                rc->ec_flags |= ECF_STONEGOLEM;
            if (xml_bvalue(node, "irongolem", false))
                rc->ec_flags |= ECF_IRONGOLEM;

            if (xml_bvalue(node, "equipment", false))
                rc->battle_flags |= BF_EQUIPMENT; /* TODO: invert this flag, so rc_get_or_create gets simpler */
            if (xml_bvalue(node, "noblock", false))
                rc->battle_flags |= BF_NOBLOCK;
            if (xml_bvalue(node, "invinciblenonmagic", false))
                rc->battle_flags |= BF_INV_NONMAGIC;
            if (xml_bvalue(node, "resistbash", false))
                rc->battle_flags |= BF_RES_BASH;
            if (xml_bvalue(node, "resistcut", false))
                rc->battle_flags |= BF_RES_CUT;
            if (xml_bvalue(node, "resistpierce", false))
                rc->battle_flags |= BF_RES_PIERCE;
            if (xml_bvalue(node, "noattack", false))
                rc->battle_flags |= BF_NO_ATTACK;

            rc->recruit_multi = 1.0;
            for (child = node->children; child; child = child->next) {
                if (strcmp((const char *)child->name, "ai") == 0) {
                    parse_ai(rc, child);
                }
                else if (strcmp((const char *)child->name, "param") == 0) {
                    xmlChar *propName = xmlGetProp(child, BAD_CAST "name");
                    xmlChar *propValue = xmlGetProp(child, BAD_CAST "value");
                    rc_set_param(rc, (const char *)propName, (const char *)propValue);
                    xmlFree(propName);
                    xmlFree(propValue);
                }
            }

            /* reading eressea/races/race/skill */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "skill", xpath);
            memset(rc->bonus, 0, sizeof(rc->bonus));
            for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                xmlNodePtr node = result->nodesetval->nodeTab[k];
                int mod = xml_ivalue(node, "modifier", 0);
                int speed = xml_ivalue(node, "speed", study_speed_base);

                propValue = xmlGetProp(node, BAD_CAST "name");
                assert(propValue != NULL);
                sk = findskill((const char *)propValue);
                if (sk != NOSKILL) {
                    rc->bonus[sk] = (char)mod;
                    if (speed) {
                        set_study_speed(rc, sk, speed);
                    }
                }
                else {
                    log_error("unknown skill '%s' in race '%s'\n", (const char *)propValue, rc->_name);
                }
                xmlFree(propValue);
            }
            xmlXPathFreeObject(result);

            /* reading eressea/races/race/familiar */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "familiar", xpath);
            if (result->nodesetval->nodeNr > MAXMAGIETYP) {
                log_error("race %s has %d potential familiars", rc->_name, result->nodesetval->nodeNr);
            }
            else {
                for (k = 0; k != MAXMAGIETYP; ++k) {
                    if (k < result->nodesetval->nodeNr) {
                        xmlNodePtr node = result->nodesetval->nodeTab[k];

                        propValue = xmlGetProp(node, BAD_CAST "race");
                        assert(propValue != NULL);
                        frc = rc_get_or_create((const char *)propValue);
                        frc->flags |= RCF_FAMILIAR;
                        if (xml_bvalue(node, "default", false)) {
                            rc->familiars[k] = rc->familiars[0];
                            rc->familiars[0] = frc;
                        }
                        else {
                            rc->familiars[k] = frc;
                        }
                        xmlFree(propValue);
                    }
                    else {
                        rc->familiars[k] = frc;
                    }
                }
            }
            xmlXPathFreeObject(result);

            /* reading eressea/races/race/attack */
            xpath->node = node;
            result = xmlXPathEvalExpression(BAD_CAST "attack", xpath);
            attack = rc->attack;
            attacks = 0;
            for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                xmlNodePtr node = result->nodesetval->nodeTab[k];
                while (attack->type != AT_NONE) {
                    ++attack;
                    if (attacks++ >= RACE_ATTACKS) {
                        log_error("too many attacks for race '%s'\n", rc->_name);
                        assert(!"aborting");
                    }
                }

                propValue = xmlGetProp(node, BAD_CAST "damage");
                if (propValue != NULL) {
                    attack->data.dice = str_strdup((const char *)propValue);
                    xmlFree(propValue);
                }
                else {
                    attack->data.sp = xml_spellref(node, "spell");
                    if (attack->data.sp) {
                        attack->level = xml_ivalue(node, "level", 0);
                        if (attack->level <= 0) {
                            log_error("magical attack '%s' for race '%s' needs a level: %d\n", attack->data.sp->name, rc->_name, attack->level);
                        }
                    }
                }
                attack->type = xml_ivalue(node, "type", 0);
                attack->flags = xml_ivalue(node, "flags", 0);
            }
            xmlXPathFreeObject(result);
        }
    }
    xmlXPathFreeObject(races);

    xmlXPathFreeContext(xpath);

    return results;
}

static int parse_messages(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath;
    xmlXPathObjectPtr messages;
    xmlNodeSetPtr nodes;
    int i, results;

    xpath = xmlXPathNewContext(doc);

    /* reading eressea/messages/message */
    messages =
        xmlXPathEvalExpression(BAD_CAST "/eressea/messages/message", xpath);
    nodes = messages->nodesetval;
    results = nodes->nodeNr;
    for (i = 0; i != nodes->nodeNr; ++i) {
        xmlNodePtr node = nodes->nodeTab[i];
        const char *default_section = "events";
        xmlChar *propSection;
        xmlChar *propValue;
        xmlXPathObjectPtr result;
        int k;
        char **argv = NULL;
        const message_type *mtype;

        /* arguments */
        xpath->node = node;
        result = xmlXPathEvalExpression(BAD_CAST "type/arg", xpath);
        if (result->nodesetval && result->nodesetval->nodeNr > 0) {
            argv = malloc(sizeof(char *) * (result->nodesetval->nodeNr + 1));
            for (k = 0; k != result->nodesetval->nodeNr; ++k) {
                xmlNodePtr node = result->nodesetval->nodeTab[k];
                char zBuffer[128];
                xmlChar *propName, *propType;

                propName = xmlGetProp(node, BAD_CAST "name");
                propType = xmlGetProp(node, BAD_CAST "type");
                sprintf(zBuffer, "%s:%s", (const char *)propName,
                    (const char *)propType);
                xmlFree(propName);
                xmlFree(propType);
                argv[k] = str_strdup(zBuffer);
            }
            argv[result->nodesetval->nodeNr] = NULL;
        }
        xmlXPathFreeObject(result);

        /* add the messagetype */
        propValue = xmlGetProp(node, BAD_CAST "name");
        mtype = mt_find((const char *)propValue);
        if (mtype == NULL) {
            mtype = mt_register(mt_new((const char *)propValue, (const char **)argv));
        }
        else {
            assert(argv != NULL || !"cannot redefine arguments of message now");
        }
        xmlFree(propValue);

        /* register the type for the CR */
        crt_register(mtype);

        /* let's clean up the mess */
        if (argv != NULL) {
            for (k = 0; argv[k] != NULL; ++k)
                free(argv[k]);
            free(argv);
        }

        propSection = xmlGetProp(node, BAD_CAST "section");
        if (propSection == NULL)
            propSection = BAD_CAST default_section;

        /* strings */
        xpath->node = node;
        result = xmlXPathEvalExpression(BAD_CAST "text", xpath);
        assert(result->nodesetval->nodeNr>0);
        for (k = 0; k != result->nodesetval->nodeNr; ++k) {
            xmlNodePtr node = result->nodesetval->nodeTab[k];
            struct locale *lang;
            xmlChar *propText;

            xml_readtext(node, &lang, &propText);
            if (lang) {
                xml_cleanup_string(propText);
                nrt_register(mtype, lang, (const char *)propText, 0,
                    (const char *)propSection);
            }
            xmlFree(propText);

        }
        xmlXPathFreeObject(result);

        if (propSection != BAD_CAST default_section)
            xmlFree(propSection);
    }

    xmlXPathFreeObject(messages);

    xmlXPathFreeContext(xpath);
    return results;
}

static void
xml_readstrings(xmlXPathContextPtr xpath, xmlNodePtr * nodeTab, int nodeNr,
bool names)
{
    int i;

    for (i = 0; i != nodeNr; ++i) {
        xmlNodePtr stringNode = nodeTab[i];
        xmlChar *propName = xmlGetProp(stringNode, BAD_CAST "name");
        xmlChar *propNamespace = NULL;
        xmlXPathObjectPtr result;
        int k;
        char zName[128];

        assert(propName != NULL);
        if (names)
            propNamespace = xmlGetProp(stringNode->parent, BAD_CAST "name");
        mkname_buf((const char *)propNamespace, (const char *)propName, zName);
        if (propNamespace != NULL)
            xmlFree(propNamespace);
        xmlFree(propName);

        /* strings */
        xpath->node = stringNode;
        result = xmlXPathEvalExpression(BAD_CAST "text", xpath);
        for (k = 0; k != result->nodesetval->nodeNr; ++k) {
            xmlNodePtr textNode = result->nodesetval->nodeTab[k];
            struct locale *lang;
            xmlChar *propText;

            xml_readtext(textNode, &lang, &propText);
            if (propText != NULL) {
                assert(strcmp(zName,
                    (const char *)xml_cleanup_string(BAD_CAST zName)) == 0);
                if (lang) {
                    xml_cleanup_string(propText);
                    locale_setstring(lang, zName, (const char *)propText);
                }
                xmlFree(propText);
            }
            else {
                log_warning("string %s has no text in locale %s\n", zName, locale_name(lang));
            }
        }
        xmlXPathFreeObject(result);
    }
}

static int parse_strings(xmlDocPtr doc)
{
    xmlXPathContextPtr xpath = xmlXPathNewContext(doc);
    xmlXPathObjectPtr strings;
    int results = 0;

    /* reading eressea/strings/string */
    strings = xmlXPathEvalExpression(BAD_CAST "/eressea/strings/string", xpath);
    xml_readstrings(xpath, strings->nodesetval->nodeTab,
        strings->nodesetval->nodeNr, false);
    results += strings->nodesetval->nodeNr;
    xmlXPathFreeObject(strings);

    strings =
        xmlXPathEvalExpression(BAD_CAST "/eressea/strings/namespace/string", xpath);
    xml_readstrings(xpath, strings->nodesetval->nodeTab,
        strings->nodesetval->nodeNr, true);
    results += strings->nodesetval->nodeNr;
    xmlXPathFreeObject(strings);

    xmlXPathFreeContext(xpath);
    return results;
}

void register_xmlreader(void)
{
    xml_register_callback(parse_races);
    xml_register_callback(parse_resources);

    xml_register_callback(parse_buildings); /* requires resources */
    xml_register_callback(parse_ships);     /* requires resources, terrains */
    xml_register_callback(parse_equipment); /* requires resources */

    xml_register_callback(parse_spells); /* requires resources */
    xml_register_callback(parse_spellbooks);  /* requires spells */

    xml_register_callback(parse_strings);
    xml_register_callback(parse_messages);
}
