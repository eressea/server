#include <config.h>

#include <stdio.h>
#include <string.h>

static struct vartype {
	const char * name;
	const char * type;
	const char * msg;
} vartype[] = {
	{ "from", "unit", "donation"},
	{ "to", "unit", "donation" },

	/* strange and to be changed */
	{ "destruction", "int", "siege" },
	{ "mode", "int", "travel" },
	{ "discover", "string", "givedumb" },
	{ "receipient", "unit", "givecommand" },
	{ "sink", "string", "entermaelstrom" },
	{ "sink", "string", "storm" },
	{ "using", "resource", "errusingpotion" },
	{ "type", "string", "scunicorn" },
	{ "special", "string", "new_fspecial" },
	{ "special", "string", "new_fspecial_level" },

	/* broadband */

	{ "regions", "string" },
	{ "succ", "string" },
	{ "renamed", "string" },
	{ "reason", "string" },
	{ "message", "string" },
	{ "value", "string" },
	{ "error", "string" },
	{ "string", "string" },
	{ "command", "string" },
	{ "spell", "string" }, /* ? */

	{ "building", "building" },

	{ "ship", "ship" },

	{ "resource", "resource" },
	{ "potion", "resource" },
	{ "item", "resource" },
	{ "herb", "resource" },

	{ "teacher", "unit" },
	{ "student", "unit" },
	{ "unit", "unit" },
	{ "renamer", "unit" },
	{ "spy", "unit" },
	{ "mage", "unit" },
	{ "opfer", "unit" },
	{ "target", "unit" },
	{ "recipient", "unit" },
	{ "follower", "unit" },

	{ "skill", "skill" },

	{ "faction", "faction" },

	{ "region", "region" },
	{ "source", "region" },
	{ "regionn", "region" },
	{ "regionv", "region" },
	{ "end", "region" },
	{ "start", "region" },
	{ "runto", "region" },
	{ "to", "region" },
	{ "from", "region" },

	{ "kills", "int" },
	{ "fallen", "int" },
	{ "amount", "int" },
	{ "aura", "int" },
	{ "months", "int" },
	{ "wanted", "int" },
	{ "money", "int" },
	{ "dead", "int" },
	{ "level", "int" },
	{ "days", "int" },
	{ "damage", "int" },
	{ "cost", "int" },
	{ "want", "int" },
	{ "size", "int" },
	{ "alive", "int" },
	{ "run", "int" },
	{ "hits", "int" },
	{ "turns", "int" },

	{ "race", "race" },

	{ "direction", "direction" },
	{ "dir", "direction" },

	{ "id", "int36" },
	{ NULL, NULL }
};

static const char * 
type(const char * name,const char * msg)
{
	int i = 0;
	while (vartype[i].name) {
		if (strcmp(name, vartype[i].name)==0 && 
			(vartype[i].msg==NULL || strcmp(vartype[i].msg, msg)==0)) {
			return vartype[i].type;
		}
		++i;
	}
	fprintf(stderr, "unknown type for \"%s\" in message \"%s\".\n", name, msg);
	return "unknown";
}

static void
parse_message(char * b, FILE * ostream)
{
	const char * vtype;
	char *m, *a = NULL, message[8192];
	char * name;
	char * language;
	char * section = NULL;
	int i, level = 0;
	char * args[16];
	boolean f_symbol = false;

	/* skip comments */
	if (b[0]=='#' || b[0]==0) return;

	/* the name of this type */
	name = b;
	while (*b && *b!=';') ++b;
	if (!*b) return;
	*b++ = 0;

	/* the section for this type */
	section = b;
	while (*b && *b!=';' && *b!=':') ++b;
	if (!strcmp(section, "none")) section=NULL;

	/* if available, the level for this type */
	if (*b==':') {
		char * x;
		*b++ = 0;
		x = b;
		while (*b && *b!=';') ++b;
		level=atoi(x);
	}
	*b++ = 0;

	/* the locale */
	language = b;
	while (*b && *b!=';') ++b;
	*b++ = 0;

	/* parse the message */
	i = 0;
	m = message;
	*m++='\"';
	while (*b) {
		switch (*b) {
		case '{':
			f_symbol = true;
			a = ++b;
			break;
		case '}':
			*b++ = '\0';
			args[i] = strdup(a);
			vtype = type(args[i], name);
			if (strcmp(vtype, "string")==0) {
				sprintf(m, "$%s", args[i]);
			} else {
				sprintf(m, "$%s($%s)", vtype, args[i]);
			}
			m+=strlen(m);
			i++;
			f_symbol = false;
			break;
		case ' ':
			if (f_symbol) {
				a = ++b;
				break;
			}
			/* fall-through intended */
		default:
			if (!f_symbol) {
				*m++ = *b++;
			} else b++;
		}
	}
	strcpy(m, "\"");
	args[i] = NULL;

	/* add the messagetype */
	fprintf(ostream, "<message name=\"%s\">\n", name);
	fputs("\t<type>\n", ostream);
	for (i=0;args[i];++i) {
		fprintf(ostream, "\t\t<arg name=\"%s\" type=\"%s\"></arg>\n", args[i], type(args[i], name));
	}
	fputs("\t</type>\n", ostream);
	fprintf(ostream, "\t<locale name=\"%s\">\n", language);
	fprintf(ostream, "\t\t<nr section=\"%s\">\n", 
		section);
	fprintf(ostream, "\t\t\t<text>%s</text>\n", message);
	fputs("\t\t</nr>\n", ostream);
	fputs("\t</locale>\n", ostream);
	fputs("</message>\n\n", ostream);
}

void
read_messages(FILE * istream, FILE * ostream)
{
	char buf[8192];
	fputs("<messages>\n", ostream);
	while (fgets(buf, sizeof(buf), istream)) {
		buf[strlen(buf)-1] = 0; /* \n weg */
		parse_message(buf, ostream);
	}
	fputs("</messages>\n", ostream);
}

int 
main(int argc, char** argv)
{
	FILE * istream = stdin;
	FILE * ostream = stdout;
	int nretval = -1;
	if (argc>1) istream = fopen(argv[1], "rt+");
	if (argc>2) ostream = fopen(argv[2], "wt+");
	if (istream && ostream) {
		read_messages(istream, ostream);
	}
	return nretval;
}
