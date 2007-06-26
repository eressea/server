/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "xmlreport.h"

/* tweakable features */
#define ENCODE_SPECIAL 1
#define RENDER_CRMESSAGES

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
#include <kernel/movement.h>
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
#include <goodies.h>
#include <base36.h>
#include <language.h>

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

static xmlChar*
xml_s(const char * str)
{
  static xmlChar buffer[1024];
  const char * inbuf = str;
  xmlChar * outbuf = buffer;
  int inbytes = (int)strlen(str)+1;
  int outbytes = (int)sizeof(buffer);

  isolat1ToUTF8(outbuf, &outbytes, BAD_CAST inbuf, &inbytes);
  return buffer;
}

static xmlChar*
xml_i(double number)
{
  static char buffer[128];
  sprintf(buffer, "%.0lf", number);
  return (xmlChar*)buffer;
}

static xmlNodePtr
report_faction(report_context * ctx, faction * f)
{
  xmlNodePtr node = xmlNewNode(NULL, BAD_CAST "faction");
  xmlNewProp(node, BAD_CAST "id", xml_i(f->no));
  xmlNewProp(node, BAD_CAST "name", f->name);
  xmlNewProp(node, BAD_CAST "email", xml_s(f->email));
  if (f->banner && *f->banner) xmlNewProp(node, BAD_CAST "banner", f->banner);
  if (f==ctx->f) {
    const char * s;
    xmlNewProp(node, BAD_CAST "locale", BAD_CAST locale_name(f->locale));
    xmlNewProp(node, BAD_CAST "age", xml_i(f->age));
    xmlNewProp(node, BAD_CAST "options", xml_i(f->options));
    xmlNewProp(node, BAD_CAST "race", xml_s(L10N(rc_name(f->race, 0))));
    xmlNewProp(node, BAD_CAST "magic", xml_s(L10N(magietypen[f->magiegebiet])));
    s = get_prefix(f->attribs);;
    if (s) {
      xmlNewProp(node, BAD_CAST "prefix", xml_s(L10N(s)));
    }
  }
  return node;
}

static xmlNodePtr
report_region(report_context * ctx, seen_region * sr)
{
  const region * r = sr->r;
  xmlNodePtr node = xmlNewNode(NULL, BAD_CAST "region");
  xmlNewProp(node, BAD_CAST "terrain", xml_s(L10N(terrain_name(r))));
  xmlNewProp(node, BAD_CAST "x", xml_i(r->x));
  xmlNewProp(node, BAD_CAST "y", xml_i(r->y));
  xmlNewProp(node, BAD_CAST "view", xml_s(visibility[sr->mode]));
  if (r->planep) {
    xmlNewProp(node, BAD_CAST "plane", xml_s(r->planep->name));
  }
  if (r->land!=NULL) {
    xmlNewProp(node, BAD_CAST "name", xml_s(r->land->name));
  }
  return node;
}

/* main function of the xmlreport. creates the header and traverses all regions */
static int
report_xml(const char * filename, report_context * ctx)
{
  xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
  xmlNodePtr xmlReport = xmlNewNode(NULL, BAD_CAST "report");
  const faction_list * address;
  region * r = ctx->first, * rend = ctx->last;

  for (address=ctx->addresses;address;address=address->next) {
    xmlAddChild(xmlReport, report_faction(ctx, address->data));
  }

  for (;r!=rend;r=r->next) {
    seen_region * sr = find_seen(ctx->seen, r);
    if (sr!=NULL) xmlAddChild(xmlReport, report_region(ctx, sr));
  }
  xmlDocSetRootElement(doc, xmlReport);
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

