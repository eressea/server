#include <config.h>

#include <message.h>
#include <crmessage.h>
#include <nrmessage.h>
#include <translation.h>

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char * sample = "\"enno and $if($eq($i,0),\"nobody else\",$if($eq($i,1),\"another guy\",\"$int($i) other people\")).\"";

void
test_translation(void)
{
	int x;
	char * c;

	c = translate_va("\"$name is a godlike $role.\"", "name role", "enno", "coder");
	if (c) puts(c);
	free(c);

	for (x=0;x!=4;++x) {
		c = translate_va(sample, "i", x);
		if (c) puts(c);
		free(c);
	}
}


void
test_message(void)
{
	char buffer[1024];
	struct message_type * mt_example = mt_new_va("example", "subject:string", "number:int", "object:string", NULL);
	message * msg = msg_create_va(mt_example, "enno", (void*)7, "autos");

	tsf_register("string", &cr_string);
	tsf_register("int", &cr_int);

	crt_register(mt_example, NULL);
	nrt_register(mt_example, NULL, "\"$subject hat $int($number) $object\"");
	cr_render(msg, NULL, buffer);
	puts(buffer);
	nr_render(msg, NULL, buffer);
	puts(buffer);
}

#include <string.h>
#include <language.h>

message *
new_message(struct faction * receiver, const char* sig, ...)
	/* compatibility function, to keep the old function calls valid *
	 * all old messagetypes are converted into a message with ONLY string parameters,
	 * this function will convert given parameters to a string representation
	 * based on the signature - like render() once did */
{
	const message_type * mtype;
	va_list marker;
	const char * signature = strchr(sig, '%');
	char buffer[128];
	int i=0;
	const char * c = sig;
	const char * args[16];

	strncpy(buffer, sig, signature-sig);
	buffer[signature-sig] = '\0';
	mtype = mt_find(buffer);

	if (!mtype) {
		fprintf(stderr, "trying to create message of unknown type \"%s\"\n", buffer);
		return NULL;
	}

	while(*c!='%') buffer[i++] = *(c++);
	buffer[i] = 0;

	va_start(marker, sig);
	while (*c) {
		char type;
		char *p = buffer;
		assert(*c=='%');
		type = *(++c);
	/*
			case 'f': (*ep)->type = IT_FACTION; break;
			case 'u': (*ep)->type = IT_UNIT; break;
			case 'r': (*ep)->type = IT_REGION; break;
			case 'h': (*ep)->type = IT_SHIP; break;
			case 'b': (*ep)->type = IT_BUILDING; break;
			case 'X': (*ep)->type = IT_RESOURCETYPE; break;
			case 'x': (*ep)->type = IT_RESOURCE; break;
			case 't': (*ep)->type = IT_SKILL; break;
			case 's': (*ep)->type = IT_STRING; break;
			case 'i': (*ep)->type = IT_INT; break;
			case 'd': (*ep)->type = IT_DIRECTION; break;
			case 'S': (*ep)->type = IT_FSPECIAL; break;
*/
		c+=2;
		while (*c && isalnum(*(unsigned char*)c)) *(p++) = *(c++);
		*p = '\0';
		for (i=0;i!=mtype->nparameters;++i) {
			if (!strcmp(buffer, mtype->pnames[i])) break;
		}
		assert(i!=mtype->nparameters || !"unknown parameter");

		switch(type) {
			case 's':
				args[i] = va_arg(marker, const char *);
				break;
			case 'i':
				itoa(va_arg(marker, int), buffer, sizeof(buffer));
				args[i] = strdup(buffer);
				break;
#ifdef ERESSEA_KERNEL
			case 'f':
				args[i] = factionname(va_arg(marker, const struct faction*));
				break;
			case 'u':
				args[i] = unitname(va_arg(marker, const struct unit*));
				break;
			case 'r':
				args[i] = rname(va_arg(marker, const struct region*), receiver->lang);
				break;
			case 'h':
				args[i] = shipname(va_arg(marker, const struct ship*));
				break;
			case 'b':
				args[i] = buildingname(va_arg(marker, const struct ship*));
				break;
			case 'X':
				args[i] = resourcename(va_arg(marker, const resource_type *), 0);
				break;
			case 'x':
				args[i] = resourcename(oldresourcetype[(resource_t)va_arg(marker, resource_t)], 0);
				break;
			case 't':
				args[i] = skillnames[va_arg(marker, skill_t)];
				break;
			case 'd':
				args[i] = directions[i];
				break;
			case 'S':
#endif
			default:
				args[i] = NULL;
		}
	}
	return msg_create(mtype, args);
}

static void
parse_message(char * b)
{
	char *m, *a, message[8192];
	char * name;
	char * language;
	struct locale * lang;
	char * section = NULL;
	int i, level = 0;
	char * args[16];
	boolean f_symbol = false;
	const struct message_type * mtype;

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
	lang = find_locale(language);
	if (!lang) lang = make_locale(language);

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
			sprintf(m, "$%s", args[i]);
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
	mtype = mt_register(mt_new(name, args));
	nrt_register(mtype, lang, message);
	crt_register(mtype, lang);
}

static void
read_messages(FILE * F)
{
	char buf[8192];
	while (fgets(buf, sizeof(buf), F)) {
		buf[strlen(buf)-1] = 0; /* \n weg */
		parse_message(buf);
	}
}

static void 
test_compat()
{
	char buffer[1024];
	FILE * F = fopen("res/de_DE/messages.txt", "rt");
	message * msg;
	if (F) {
		read_messages(F);
		fclose(F);
	}
	msg = new_message(NULL, "entrise%s:region", "Porzel (8,7)");
	if (cr_render(msg, NULL, buffer)==0) puts(buffer);
	if (nr_render(msg, NULL, buffer)==0) puts(buffer);
}

int 
main(int argc, char**argv)
{
	translation_init();

	test_message();
	test_translation();
	test_compat();

	translation_done();
	return 0;
}
