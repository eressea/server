struct attrib;
struct unit;

void init_gmcmd(void);
void gm_command(const char * cmd, struct unit * u);

struct attrib * find_key(struct attrib * attribs, int key);

