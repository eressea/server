#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <iniparser.h>

const char *progname = "inifile";

void usage(void) {
    fprintf(stdout, "Usage: %s file.ini [add|del] <args>\n", progname);
}

int main(int argc, char ** argv) {
    const char *inifile;
    const char *command;
    dictionary *ini;
    FILE *F;

    inifile = (argc<2) ? 0 : argv[1];
    command = (argc<3) ? 0 : argv[2];

    if (!inifile) {
        fputs("Missing filename.\n", stderr);
        usage();
        return 1;
    }
    if (!command) {
        fputs("Missing command.\n", stderr);
        usage();
        return 1;
    }
    ini = iniparser_load(inifile);
    if (!ini) {
        fprintf(stderr, "could not open %s.\n", inifile);
        return 1;
    }
    if (strcmp(command, "add")==0) {
        if (argc==4) {
            iniparser_set(ini, argv[3], "");
        }
        else if (argc==5) {
            iniparser_set(ini, argv[3], argv[4]);
        }
        else {
            fputs("set needs one or two arguments.\n", stderr);
            return 1;
        }
    }
    else if (strcmp(command, "get")==0) {
        const char * str;
	if (argc==4) {
            str = iniparser_getstring(ini, argv[3], "");
        }
        else if (argc==5) {
            str = iniparser_getstring(ini, argv[3], argv[4]);
        }
        else {
            fputs("set needs one or two arguments.\n", stderr);
            return 1;
        }
	fprintf(stdout, "%s\n", str);
    }
    else if (strcmp(command, "del")==0) {
        if (argc<4) {
            fputs("del needs one argument.\n", stderr);
            return 1;
        }
        iniparser_unset(ini, argv[3]);
    } else {
        fprintf(stderr, "unknown command %s.\n", command);
        usage();
        return 1;
    }
     
    if ((F=fopen(inifile, "wt"))!=0) {
        iniparser_dump_ini(ini, F);
        fclose(F);
    }
    iniparser_freedict(ini);
    return 0;
}
