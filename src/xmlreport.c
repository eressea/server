/* 
+-------------------+  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Christian Schlittchen <corwin@amber.kn-bremen.de>
| (c) 1998 - 2008   |  Katja Zedel <katze@felidae.kn-bremen.de>
+-------------------+  
This program may not be used, modified or distributed 
without prior permission by the authors of Eressea.
*/

#include <platform.h>
#include <kernel/config.h>
#include "xmlreport.h"

#define XML_ATL_NAMESPACE (const xmlChar *) "http://www.eressea.de/XML/2008/atlantis"
#define XML_XML_LANG (const xmlChar *) "lang"

/* modules include */
#include <modules/score.h>

/* attributes include */
#include <attributes/follow.h>
#include <attributes/racename.h>
#include <attributes/orcification.h>
#include <attributes/otherfaction.h>
#include <attributes/raceprefix.h>

/* gamecode includes */
#include "laws.h"
#include "economy.h"
#include "move.h"

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/alliance.h>
#include <kernel/ally.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/magic.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/save.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/message.h>
#include <quicklist.h>
#include <util/unicode.h>
#include <util/xml.h>

#ifdef USE_LIBXML2
/* libxml2 includes */
#include <libxml/tree.h>
#include <libxml/encoding.h>
#ifdef USE_ICONV
#include <iconv.h>
#endif
#endif

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define L10N(x) x

typedef struct xml_context {
  xmlDocPtr doc;
  xmlNsPtr ns_atl;
  xmlNsPtr ns_xml;
} xml_context;

static xmlNodePtr
xml_link(report_context * ctx, const xmlChar * rel, const xmlChar * ref)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "link");

  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "rel", rel);
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "ref", ref);

  return node;
}

static const xmlChar *xml_ref_unit(const unit * u)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "unit_%d", u->no);
  return (const xmlChar *)idbuf;
}

static const xmlChar *xml_ref_faction(const faction * f)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "fctn_%d", f->no);
  return (const xmlChar *)idbuf;
}

static const xmlChar *xml_ref_group(const group * g)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "grp_%d", g->gid);
  return (const xmlChar *)idbuf;
}

static const xmlChar *xml_ref_prefix(const char *str)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "pref_%s", str);
  return (const xmlChar *)idbuf;
}

static const xmlChar *xml_ref_building(const building * b)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "bldg_%d", b->no);
  return (const xmlChar *)idbuf;
}

static const xmlChar *xml_ref_ship(const ship * sh)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "shp_%d", sh->no);
  return (const xmlChar *)idbuf;
}

static const xmlChar *xml_ref_region(const region * r)
{
  static char idbuf[20];
  _snprintf(idbuf, sizeof(idbuf), "rgn_%d", r->uid);
  return (const xmlChar *)idbuf;
}

static xmlNodePtr xml_inventory(report_context * ctx, item * items, unit * u)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "items");
  item *itm;

  for (itm = items; itm; itm = itm->next) {
    xmlNodePtr child;
    const char *name;
    int n;

    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "item"));
    report_item(u, itm, ctx->f, NULL, &name, &n, true);
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", (xmlChar *) name);
    xmlNodeAddContent(child, (xmlChar *) itoab(n, 10));
  }
  return node;
}

#ifdef TODO /*spellbooks */
static xmlNodePtr
xml_spells(report_context * ctx, quicklist * slist, int maxlevel)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr child, node = xmlNewNode(xct->ns_atl, BAD_CAST "spells");
  quicklist *ql;
  int qi;

  for (ql = slist, qi = 0; ql; ql_advance(&ql, &qi, 1)) {
    spell *sp = (spell *) ql_get(ql, qi);

    if (sp->level <= maxlevel) {
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "spell"));
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "name", BAD_CAST sp->sname);
    }
  }
  return node;
}
#endif

static xmlNodePtr xml_skills(report_context * ctx, unit * u)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr child, node = xmlNewNode(xct->ns_atl, BAD_CAST "skills");
  skill *sv;

  for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
    if (sv->level > 0) {
      skill_t sk = sv->id;
      int esk = eff_skill(u, sk, u->region);

      child =
        xmlNewTextChild(node, xct->ns_atl, BAD_CAST "skill", BAD_CAST itoab(esk,
          10));
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", BAD_CAST skillnames[sk]);
    }
  }

  return node;
}

static xmlNodePtr xml_unit(report_context * ctx, unit * u, int mode)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "unit");
  static const curse_type *itemcloak_ct = 0;
  static bool init = false;
  xmlNodePtr child;
  const char *str, *rcname, *rcillusion;
  bool disclosure = (ctx->f == u->faction || omniscient(ctx->f));

  /* TODO: hitpoints, aura, combatspells, curses */

  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, xml_ref_unit(u));
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "key", BAD_CAST itoa36(u->no));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name", (const xmlChar *)u->name);
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "number",
    (const xmlChar *)itoab(u->number, 10));

  /* optional description */
  str = u_description(u, ctx->f->locale);
  if (str) {
    child =
      xmlNewTextChild(node, xct->ns_atl, BAD_CAST "text", (const xmlChar *)str);
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "public");
    if (str != u->display) {
      xmlNewNsProp(child, xct->ns_atl, XML_XML_LANG,
        BAD_CAST locale_name(ctx->f->locale));
    }
  }

  /* possible <guard/> info */
  if (is_guard(u, GUARD_ALL) != 0) {
    xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "guard"));
  }

  /* siege */
  if (fval(u, UFL_SIEGE)) {
    building *b = usiege(u);
    if (b) {
      xmlAddChild(node, xml_link(ctx, BAD_CAST "siege", xml_ref_building(b)));
    }
  }

  /* TODO: temp/alias */

  /* race information */
  report_race(u, &rcname, &rcillusion);
  if (disclosure) {
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "race"));
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "true");
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", (const xmlChar *)rcname);
    if (rcillusion) {
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "race"));
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "stealth");
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
        (const xmlChar *)rcillusion);
    }
  } else {
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "race"));
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
      (const xmlChar *)(rcillusion ? rcillusion : rcname));
  }

  /* group and prefix information. we only write the prefix if we really must */
  if (fval(u, UFL_GROUP)) {
    attrib *a = a_find(u->attribs, &at_group);
    if (a != NULL) {
      const group *g = (const group *)a->data.v;
      if (disclosure) {
        child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "group"));
        xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", xml_ref_group(g));
      } else {
        const char *prefix = get_prefix(g->attribs);
        if (prefix) {
          child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "prefix"));
          xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
            xml_ref_prefix(prefix));
        }
      }
    }
  }

  if (disclosure) {
    unit *mage;

    str = uprivate(u);
    if (str) {
      child =
        xmlNewTextChild(node, xct->ns_atl, BAD_CAST "text",
        (const xmlChar *)str);
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "private");
    }

    /* familiar info */
    mage = get_familiar_mage(u);
    if (mage)
      xmlAddChild(node, xml_link(ctx, BAD_CAST "familiar_of",
          xml_ref_unit(mage)));

    /* combat status */
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "status"));
    xmlSetNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "combat");
    xmlSetNsProp(child, xct->ns_atl, BAD_CAST "value",
      BAD_CAST combatstatus[u->status]);

    if (fval(u, UFL_NOAID)) {
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "status"));
      xmlSetNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "aid");
      xmlSetNsProp(child, xct->ns_atl, BAD_CAST "value", BAD_CAST "false");
    }

    if (fval(u, UFL_STEALTH)) {
      int i = u_geteffstealth(u);
      if (i >= 0) {
        child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "status"));
        xmlSetNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "stealth");
        xmlSetNsProp(child, xct->ns_atl, BAD_CAST "value", BAD_CAST itoab(i,
            10));
      }
    }
    if (fval(u, UFL_HERO)) {
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "status"));
      xmlSetNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "hero");
      xmlSetNsProp(child, xct->ns_atl, BAD_CAST "value", BAD_CAST "true");
    }

    if (fval(u, UFL_HUNGER)) {
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "status"));
      xmlSetNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "hunger");
      xmlSetNsProp(child, xct->ns_atl, BAD_CAST "value", BAD_CAST "true");
    }

    /* skills */
    if (u->skill_size) {
      xmlAddChild(node, xml_skills(ctx, u));
    }

#ifdef TODO /*spellbooks */
    /* spells */
    if (is_mage(u)) {
      sc_mage *mage = get_mage(u);
      quicklist *slist = mage->spells;
      if (slist) {
        xmlAddChild(node, xml_spells(ctx, slist, effskill(u, SK_MAGIC)));
      }
    }
#endif
  }

  /* faction information w/ visibiility */
  child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "faction"));
  if (disclosure) {
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "true");
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
      xml_ref_faction(u->faction));

    if (fval(u, UFL_ANON_FACTION)) {
      const faction *sf = visible_faction(NULL, u);
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "faction"));
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "stealth");
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", xml_ref_faction(sf));
    }
  } else {
    const faction *sf = visible_faction(ctx->f, u);
    if (sf == ctx->f) {
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "stealth");
    }
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", xml_ref_faction(sf));
  }

  /* the inventory */
  if (u->items) {
    item result[MAX_INVENTORY];
    item *show = NULL;

    if (!init) {
      init = true;
      itemcloak_ct = ct_find("itemcloak");
    }

    if (disclosure) {
      show = u->items;
    } else {
      bool see_items = (mode >= see_unit);
      if (see_items) {
        if (itemcloak_ct && curse_active(get_curse(u->attribs, itemcloak_ct))) {
          see_items = false;
        } else {
          see_items = effskill(u, SK_STEALTH) < 3;
        }
      }
      if (see_items) {
        int n = report_items(u->items, result, MAX_INVENTORY, u, ctx->f);
        assert(n >= 0);
        if (n > 0)
          show = result;
        else
          show = NULL;
      } else {
        show = NULL;
      }
    }

    if (show) {
      xmlAddChild(node, xml_inventory(ctx, show, u));
    }
  }

  return node;
}

static xmlNodePtr xml_resources(report_context * ctx, const seen_region * sr)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr node = NULL;
  resource_report result[MAX_RAWMATERIALS];
  int n, size = report_resources(sr, result, MAX_RAWMATERIALS, ctx->f);

  if (size) {
    node = xmlNewNode(xct->ns_atl, BAD_CAST "resources");
    for (n = 0; n < size; ++n) {
      if (result[n].number >= 0) {
        xmlNodePtr child;

        child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "resource"));
        xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
          (xmlChar *) result[n].name);
        if (result[n].level >= 0) {
          xmlNewNsProp(child, xct->ns_atl, BAD_CAST "level",
            (xmlChar *) itoab(result[n].level, 10));
        }
        xmlNodeAddContent(child, (xmlChar *) itoab(result[n].number, 10));
      }
    }
  }
  return node;
}

static xmlNodePtr xml_diplomacy(report_context * ctx, const struct ally *allies)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr child, node = xmlNewNode(xct->ns_atl, BAD_CAST "diplomacy");
  const struct ally *sf;

  for (sf = allies; sf; sf = sf->next) {
    int i, status = sf->status;
    for (i = 0; helpmodes[i].name; ++i) {
      if (sf->faction && (status & helpmodes[i].status) == helpmodes[i].status) {
        status -= helpmodes[i].status;
        child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "status"));
        xmlNewNsProp(child, xct->ns_xml, BAD_CAST "faction",
          xml_ref_faction(sf->faction));
        xmlNewNsProp(child, xct->ns_xml, BAD_CAST "status",
          (xmlChar *) helpmodes[i].name);
      }
    }
  }
  return node;
}

static xmlNodePtr xml_groups(report_context * ctx, const group * groups)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr child, node = xmlNewNode(xct->ns_atl, BAD_CAST "faction");
  const group *g;

  for (g = groups; g; g = g->next) {
    const char *prefix = get_prefix(g->attribs);
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "group"));
    xmlNewNsProp(child, xct->ns_xml, XML_XML_ID, xml_ref_group(g));
    xmlNewTextChild(child, xct->ns_atl, BAD_CAST "name",
      (const xmlChar *)g->name);

    if (g->allies)
      xmlAddChild(child, xml_diplomacy(ctx, g->allies));

    if (prefix) {
      child = xmlAddChild(child, xmlNewNode(xct->ns_atl, BAD_CAST "prefix"));
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", xml_ref_prefix(prefix));
    }
  }

  return node;
}

static xmlNodePtr xml_faction(report_context * ctx, faction * f)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr child, node = xmlNewNode(xct->ns_atl, BAD_CAST "faction");

  /* TODO: alliance, locale */

  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, xml_ref_faction(f));
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "key", BAD_CAST itoa36(f->no));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name", (const xmlChar *)f->name);
  if (f->email)
    xmlNewTextChild(node, xct->ns_atl, BAD_CAST "email",
      (const xmlChar *)f->email);
  if (f->banner) {
    child =
      xmlNewTextChild(node, xct->ns_atl, BAD_CAST "text",
      (const xmlChar *)f->banner);
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "public");
  }

  if (ctx->f == f) {
    xmlAddChild(node, xml_link(ctx, BAD_CAST "race",
        BAD_CAST f->race->_name[0]));

    if (f->items)
      xmlAddChild(node, xml_inventory(ctx, f->items, NULL));
    if (f->allies)
      xmlAddChild(node, xml_diplomacy(ctx, f->allies));
    if (f->groups)
      xmlAddChild(node, xml_groups(ctx, f->groups));

    /* TODO: age, options, score, prefix, magic, immigrants, heroes, nmr, groups */
  }
  return node;
}

static xmlNodePtr
xml_building(report_context * ctx, seen_region * sr, const building * b,
  const unit * owner)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "building");
  xmlNodePtr child;
  const char *bname, *billusion;

  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, xml_ref_building(b));
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "key", BAD_CAST itoa36(b->no));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name", (const xmlChar *)b->name);
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "size",
    (const xmlChar *)itoab(b->size, 10));
  if (b->display && b->display[0]) {
    child =
      xmlNewTextChild(node, xct->ns_atl, BAD_CAST "text",
      (const xmlChar *)b->display);
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "public");
  }
  if (b->besieged) {
    xmlNewTextChild(node, xct->ns_atl, BAD_CAST "siege",
      (const xmlChar *)itoab(b->besieged, 10));
  }
  if (owner)
    xmlAddChild(node, xml_link(ctx, BAD_CAST "owner", xml_ref_unit(owner)));

  report_building(b, &bname, &billusion);
  if (owner && owner->faction == ctx->f) {
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "type"));
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "true");
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", (const xmlChar *)bname);
    if (billusion) {
      child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "type"));
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "illusion");
      xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
        (const xmlChar *)billusion);
    }
  } else {
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "type"));
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
      (const xmlChar *)(billusion ? billusion : bname));
  }

  return node;
}

static xmlNodePtr
xml_ship(report_context * ctx, const seen_region * sr, const ship * sh,
  const unit * owner)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr child, node = xmlNewNode(xct->ns_atl, BAD_CAST "ship");

  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, xml_ref_ship(sh));
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "key", BAD_CAST itoa36(sh->no));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name",
    (const xmlChar *)sh->name);
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "size",
    (const xmlChar *)itoab(sh->size, 10));

  if (sh->damage) {
    xmlNewTextChild(node, xct->ns_atl, BAD_CAST "damage",
      (const xmlChar *)itoab(sh->damage, 10));
  }

  if (fval(sr->r->terrain, SEA_REGION) && sh->coast != NODIRECTION) {
    xmlNewTextChild(node, xct->ns_atl, BAD_CAST "coast",
      BAD_CAST directions[sh->coast]);
  }

  child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "type"));
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
    (const xmlChar *)sh->type->name[0]);

  if (sh->display && sh->display[0]) {
    child =
      xmlNewTextChild(node, xct->ns_atl, BAD_CAST "text",
      (const xmlChar *)sh->display);
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "public");
  }

  if (owner)
    xmlAddChild(node, xml_link(ctx, BAD_CAST "owner", xml_ref_unit(owner)));

  if ((owner && owner->faction == ctx->f) || omniscient(ctx->f)) {
    int n = 0, p = 0;
    ship_weight(sh, &n, &p);
    xmlNewTextChild(node, xct->ns_atl, BAD_CAST "cargo",
      (const xmlChar *)itoab(n, 10));
  }
  return node;
}

static xmlNodePtr xml_region(report_context * ctx, seen_region * sr)
{
  xml_context *xct = (xml_context *) ctx->userdata;
  const region *r = sr->r;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "region");
  xmlNodePtr child;
  int stealthmod = stealth_modifier(sr->mode);
  unit *u;
  ship *sh = r->ships;
  building *b = r->buildings;
  plane *pl = rplane(r);
  int nx = r->x, ny = r->y;

  pnormalize(&nx, &ny, pl);
  adjust_coordinates(ctx->f, &nx, &ny, pl, r);

  /* TODO: entertain-quota, recruits, salary, prices, curses, borders, apparitions (Schemen), spells, travelthru, messages */
  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, xml_ref_region(r));

  child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "coordinate"));
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "x", xml_i(nx));
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "y", xml_i(ny));
  if (pl && pl->name) {
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "plane", (xmlChar *) pl->name);
  }

  child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "terrain"));
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", (xmlChar *) terrain_name(r));

  if (r->land != NULL) {
    child =
      xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name",
      (const xmlChar *)r->land->name);
    if (r->land->items) {
      xmlAddChild(node, xml_inventory(ctx, r->land->items, NULL));
    }
  }
  if (r->display && r->display[0]) {
    child =
      xmlNewTextChild(node, xct->ns_atl, BAD_CAST "text",
      (const xmlChar *)r->display);
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "rel", BAD_CAST "public");
  }
  child = xml_resources(ctx, sr);
  if (child)
    xmlAddChild(node, child);

  child = xmlNewNode(xct->ns_atl, BAD_CAST "terrain");
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref",
    (const xmlChar *)terrain_name(r));

  if (sr->mode > see_neighbour) {
    /* report all units. they are pre-sorted in an efficient manner */
    u = r->units;
    while (b) {
      while (b && (!u || u->building != b)) {
        xmlAddChild(node, xml_building(ctx, sr, b, NULL));
        b = b->next;
      }
      if (b) {
        child = xmlAddChild(node, xml_building(ctx, sr, b, u));
        while (u && u->building == b) {
          xmlAddChild(child, xml_unit(ctx, u, sr->mode));
          u = u->next;
        }
        b = b->next;
      }
    }
    while (u && !u->ship) {
      if (stealthmod > INT_MIN) {
        if (u->faction == ctx->f || cansee(ctx->f, r, u, stealthmod)) {
          xmlAddChild(node, xml_unit(ctx, u, sr->mode));
        }
      }
      u = u->next;
    }
    while (sh) {
      while (sh && (!u || u->ship != sh)) {
        xmlAddChild(node, xml_ship(ctx, sr, sh, NULL));
        sh = sh->next;
      }
      if (sh) {
        child = xmlAddChild(node, xml_ship(ctx, sr, sh, u));
        while (u && u->ship == sh) {
          xmlAddChild(child, xml_unit(ctx, u, sr->mode));
          u = u->next;
        }
        sh = sh->next;
      }
    }
  }
  return node;
}

static xmlNodePtr report_root(report_context * ctx)
{
  int qi;
  quicklist *address;
  region *r = ctx->first, *rend = ctx->last;
  xml_context *xct = (xml_context *) ctx->userdata;
  xmlNodePtr node, child, xmlReport = xmlNewNode(NULL, BAD_CAST "atlantis");
  const char *mailto = locale_string(ctx->f->locale, "mailto");
  const char *mailcmd = locale_string(ctx->f->locale, "mailcmd");
  char zText[128];
  /* TODO: locale, age, options, messages */

  xct->ns_xml = xmlNewNs(xmlReport, XML_XML_NAMESPACE, BAD_CAST "xml");
  xct->ns_atl = xmlNewNs(xmlReport, XML_ATL_NAMESPACE, NULL);
  xmlSetNs(xmlReport, xct->ns_atl);

  node = xmlAddChild(xmlReport, xmlNewNode(xct->ns_atl, BAD_CAST "server"));
  if (mailto) {
    _snprintf(zText, sizeof(zText), "mailto:%s?subject=%s", mailto, mailcmd);
    child = xmlAddChild(node, xmlNewNode(xct->ns_atl, BAD_CAST "delivery"));
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "method", BAD_CAST "mail");
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "href", BAD_CAST zText);
  }
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "game",
    (xmlChar *) global.gamename);
  strftime(zText, sizeof(zText), "%Y-%m-%dT%H:%M:%SZ",
    gmtime(&ctx->report_time));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "time", (xmlChar *) zText);
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "turn", (xmlChar *) itoab(turn,
      10));

  for (qi = 0, address = ctx->addresses; address; ql_advance(&address, &qi, 1)) {
    faction *f = (faction *) ql_get(address, qi);
    xmlAddChild(xmlReport, xml_faction(ctx, f));
  }

  for (; r != rend; r = r->next) {
    seen_region *sr = find_seen(ctx->seen, r);
    if (sr != NULL)
      xmlAddChild(xmlReport, xml_region(ctx, sr));
  }
  return xmlReport;
}

/* main function of the xmlreport. creates the header and traverses all regions */
static int
report_xml(const char *filename, report_context * ctx, const char *encoding)
{
  xml_context xct;
  xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");

  xct.doc = doc;
  assert(ctx->userdata == NULL);
  ctx->userdata = &xct;

  xmlDocSetRootElement(doc, report_root(ctx));
  xmlKeepBlanksDefault(0);
  xmlSaveFormatFileEnc(filename, doc, "utf-8", 1);
  xmlFreeDoc(doc);

  ctx->userdata = NULL;

  return 0;
}

void register_xr(void)
{
  register_reporttype("xml", &report_xml, 1 << O_XML);
#ifdef USE_ICONV
  utf8 = iconv_open("UTF-8", "");
#endif
}

void xmlreport_cleanup(void)
{
#ifdef USE_ICONV
  iconv_close(utf8);
#endif
}
