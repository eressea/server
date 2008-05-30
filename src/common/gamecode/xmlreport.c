/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "xmlreport.h"

/* tweakable features */
#define ENCODE_SPECIAL 1
#define RENDER_CRMESSAGES

#define XML_ATL_NAMESPACE (const xmlChar *) "http://www.eressea.de/XML/2008/atlantis"

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

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/alliance.h>
#include <kernel/border.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/move.h>
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
#include <util/message.h>
#include <util/goodies.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/unicode.h>

/* libxml2 includes */
#include <libxml/tree.h>
#include <libxml/encoding.h>
#ifdef USE_ICONV
#include <iconv.h>
#endif

/* libc includes */
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define L10N(x) x

typedef struct xml_context {
  xmlDocPtr doc;
  xmlNsPtr ns_atl;
  xmlNsPtr ns_xml;
} xml_context;

static const xmlChar *
xml_s(const char * str)
{
  static xmlChar buffer[1024];
  const char * inbuf = str;
  char * outbuf = (char *)buffer;
  size_t inbytes = strlen(str)+1;
  size_t outbytes = sizeof(buffer) - 1;

  unicode_latin1_to_utf8(outbuf, &outbytes, inbuf, &inbytes);
  buffer[outbytes] = 0;
  return buffer;
}

static const xmlChar *
xml_i(double number)
{
  static char buffer[128];
  sprintf(buffer, "%.0lf", number);
  return (const xmlChar *)buffer;
}

static xmlNodePtr
report_unit(report_context * ctx, unit * u, int mode)
{
  xml_context* xct = (xml_context*)ctx->userdata;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "unit");
  char unit_id[20];

  sprintf(unit_id, "unit_%d", u->no);
  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, (xmlChar *)unit_id);
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "key", BAD_CAST itoa36(u->no));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name", (const xmlChar *)u->name);

  return node;
}

static xmlNodePtr
report_faction(report_context * ctx, faction * f)
{
  xml_context* xct = (xml_context*)ctx->userdata;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "faction");
  char faction_id[20];

  sprintf(faction_id, "faction_%d", f->no);
  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, (xmlChar *)faction_id);
  xmlNewNsProp(node, xct->ns_atl, BAD_CAST "key", BAD_CAST itoa36(f->no));
  xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name", (const xmlChar *)f->name);

  return node;
}

static xmlNodePtr
report_region(report_context * ctx, seen_region * sr)
{
  xml_context* xct = (xml_context*)ctx->userdata;
  const region * r = sr->r;
  xmlNodePtr node = xmlNewNode(xct->ns_atl, BAD_CAST "region");
  xmlNodePtr child;
  char region_id[20];
  int stealthmod = stealth_modifier(sr->mode);
  unit * u;

  sprintf(region_id, "region_%u", r->uid);
  xmlNewNsProp(node, xct->ns_xml, XML_XML_ID, BAD_CAST region_id);

  child = xmlNewNode(xct->ns_atl, BAD_CAST "coordinate");
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "x", xml_i(r->x));
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "y", xml_i(r->y));
  if (r->planep) {
    xmlNewNsProp(child, xct->ns_atl, BAD_CAST "plane", xml_s(r->planep->name));
  }
  xmlAddChild(node, child);

  if (r->land!=NULL) {
    child = xmlNewTextChild(node, xct->ns_atl, BAD_CAST "name", (const xmlChar *)r->land->name);
  }

  child = xmlNewNode(xct->ns_atl, BAD_CAST "terrain");
  xmlNewNsProp(child, xct->ns_atl, BAD_CAST "ref", (const xmlChar *)terrain_name(r));

  for (u=r->units;u;u=u->next) {
    if (u->building || u->ship || (stealthmod>INT_MIN && cansee(ctx->f, r, u, stealthmod))) {
      xmlAddChild(node, report_unit(ctx, u, sr->mode));
    }
  }
  return node;
}

static xmlNodePtr
report_root(report_context * ctx)
{
  const faction_list * address;
  region * r = ctx->first, * rend = ctx->last;
  xml_context* xct = (xml_context*)ctx->userdata;
  xmlNodePtr xmlReport = xmlNewNode(NULL, BAD_CAST "atlantis");

  xct->ns_xml = xmlNewNs(xmlReport, XML_XML_NAMESPACE, BAD_CAST "xml");
  xct->ns_atl = xmlNewNs(xmlReport, XML_ATL_NAMESPACE, NULL);
  xmlSetNs(xmlReport, xct->ns_atl);

  for (address=ctx->addresses;address;address=address->next) {
    xmlAddChild(xmlReport, report_faction(ctx, address->data));
  }

  for (;r!=rend;r=r->next) {
    seen_region * sr = find_seen(ctx->seen, r);
    if (sr!=NULL) xmlAddChild(xmlReport, report_region(ctx, sr));
  }
  return xmlReport;
};

/* main function of the xmlreport. creates the header and traverses all regions */
static int
report_xml(const char * filename, report_context * ctx, const char * encoding)
{
  xml_context xct;
  xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");

  xct.doc = doc;
  assert(ctx->userdata==NULL);
  ctx->userdata = &xct;

  xmlDocSetRootElement(doc, report_root(ctx));
  xmlKeepBlanksDefault(0);
  xmlSaveFormatFileEnc(filename, doc, "utf-8", 1);
  xmlFreeDoc(doc);

  return 0;
}

void
xmlreport_init(void)
{
  register_reporttype("xml", &report_xml, 1<<O_XML);
#ifdef USE_ICONV
  utf8 = iconv_open("UTF-8", "");
#endif
}

void
xmlreport_cleanup(void)
{
#ifdef USE_ICONV
  iconv_close(utf8);
#endif
}
