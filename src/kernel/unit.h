#ifndef H_KRNL_UNIT_H
#define H_KRNL_UNIT_H

#include <util/resolve.h>
#include "database.h"
#include "status.h"

#include <stddef.h>

/* alle vierstelligen zahlen: */
#define MAX_UNIT_NR (36*36*36*36-1)

#define UFL_DEAD          (1<<0)        /* unit died in battle and cannot receive anything */
#define UFL_ISNEW         (1<<1)        /* unit was created this turn */
#define UFL_LONGACTION    (1<<2)        /* 4 */
#define UFL_OWNER         (1<<3)        /* 8 */
#define UFL_ANON_FACTION  (1<<4)        /* 16 */
#define UFL_WARMTH        (1<<6)        /* 64 */
#define UFL_HERO          (1<<7)
#define UFL_MOVED         (1<<8)
#define UFL_NOTMOVING     (1<<9)        /* Die Einheit kann sich wg. langen Kampfes nicht bewegen */
#define UFL_HUNGER        (1<<11)       /* kann im Folgemonat keinen langen Befehl ausser ARBEITE ausfuehren */
#define UFL_TARGET        (1<<13)       /* speedup: hat ein target, siehe attribut */
#define UFL_WERE          (1<<14)
#define UFL_ENTER         (1<<15)       /* unit has entered a ship/building and will not leave it */

/* warning: von 512/1024 gewechslet, wegen konflikt mit NEW_FOLLOW */
#define UFL_LOCKED        (1<<16)       /* Einheit kann keine Personen aufnehmen oder weggeben, nicht rekrutieren. */
#define UFL_FLEEING       (1<<17)       /* unit was in a battle, fleeing. */
#define UFL_STORM         (1<<19)       /* Kapitaen war in einem Sturm */
#define UFL_FOLLOWING     (1<<20)
#define UFL_FOLLOWED      (1<<21)
#define UFL_NOAID         (1<<22)       /* Einheit hat Noaid-Status */
#define UFL_MARK          (1<<23)       /* same as FL_MARK */

#define UFL_TAKEALL       (1<<25)       /* Einheit nimmt alle Gegenstaende an */

    /* flags that speed up attribute access: */
#define UFL_STEALTH       (1<<26)
#define UFL_GUARD         (1<<27)
#define UFL_GROUP         (1<<28)

    /* Flags, die gespeichert werden sollen: */
#define UFL_SAVEMASK (UFL_MOVED|UFL_NOAID|UFL_ANON_FACTION|UFL_LOCKED|UFL_HUNGER|UFL_TAKEALL|UFL_GUARD|UFL_STEALTH|UFL_GROUP|UFL_HERO)

#define UNIT_MAXSIZE 128 * 1024

typedef struct reservation {
    struct reservation* next;
    const struct item_type* type;
    int value;
} reservation;

typedef struct unit {
    struct unit* next;          /* needs to be first entry, for region's unitlist */
    struct unit* nextF;         /* naechste Einheit der Partei */
    struct unit* prevF;         /* vorherige Einheit der Partei */
    struct region* region;
    int no;                     /* id */
    int hp;
    char* _name;
    dbrow_id display_id;
    struct faction* faction;
    struct building* building;
    struct ship* ship;
    int number;      /* persons */
    int age;

    /* skill data */
    struct skill* skills;
    struct item* items;
    reservation* reservations;

    /* orders */
    struct order* orders; /* orders to be executed this turn */
    struct order* thisorder; /* long turn to be executed this turn */
    struct order* defaults; /* repeatable default orders to be saved for next turn */

    /* race and illusionary race */
    const struct race* _race;
    const struct race* irace;

    int flags;
    struct attrib* attribs;
    status_t status;
    int n;                      /* helper temporary variable, used in economy, enno: attribut? */
    int wants;                  /* enno: attribut? */
} unit;

extern struct attrib_type at_creator;
extern struct attrib_type at_potionuser;
extern struct attrib_type at_effect;
extern struct attrib_type at_private;
extern struct attrib_type at_showskchange;

struct faction;
struct unit;
struct race;
struct skill;
struct item;
struct locale;
struct gamedata;
enum skill_t;

int max_heroes(int num_people);
int countheroes(const struct faction* f);

int ualias(const struct unit* u);
void usetalias(struct unit* u, int alias);
void setguard(struct unit * u, bool enabled);

int weight(const struct unit* u);

void renumber_unit(struct unit* u, int no);
bool count_unit(const unit* u); /* unit counts towards faction.num_units and faction.num_people */

const struct race* u_irace(const struct unit* u);
const struct race* u_race(const struct unit* u);
void u_setrace(struct unit* u, const struct race*);

const char* uprivate(const struct unit* u);
void usetprivate(struct unit* u, const char* c);

struct unit* findnewunit(const struct region* r, const struct faction* f,
    int alias);

const char* u_description(const unit* u, const struct locale* lang);
struct skill* add_skill(struct unit* u, enum skill_t id);
void remove_skill(struct unit* u, enum skill_t sk);
struct skill* unit_skill(const struct unit* u, enum skill_t id);
bool has_skill(const unit* u, enum skill_t sk);
int effskill(const struct unit* u, enum skill_t sk, const struct region* r);

void set_level(struct unit* u, enum skill_t id, unsigned int level);
unsigned int get_level(const struct unit* u, enum skill_t id);
void transfermen(struct unit* src, struct unit* dst, int n);
void clone_men(const struct unit* src, struct unit* dst, int n); /* like transfer, but do not subtract from src */

int eff_skill(const struct unit* u, const struct skill* sv, const struct region* r);
int effskill_study(const struct unit* u, enum skill_t sk);

int get_modifier(const struct unit* u, enum skill_t sk, int level,
    const struct region* r, bool noitem);
int remove_unit(struct unit** ulist, struct unit* u);
void erase_unit(struct unit** ulist, struct unit* u);
void leave_region(struct unit* u);

/* looking up dead units' factions: */
struct faction* dfindhash(int no);

#define GIFT_SELF     1<<0
#define GIFT_FRIENDS  1<<1
#define GIFT_PEASANTS 1<<2
int gift_items(struct unit* u, int flags);
void make_zombie(struct unit* u);

/* see resolve.h */
#define RESOLVE_UNIT (TYP_UNIT << 24)
void resolve_unit(struct unit* u);
void write_unit_reference(const struct unit* u, struct storage* store);
int read_unit_reference(struct gamedata* data, struct unit** up, resolve_fun fun);

bool leave(struct unit* u, bool force);
bool can_leave(struct unit* u);

double u_heal_factor(const struct unit* u);
void u_set_building(struct unit* u, struct building* b);
void u_set_ship(struct unit* u, struct ship* sh);
void leave_ship(struct unit* u);
void leave_building(struct unit* u);

void set_leftship(struct unit* u, struct ship* sh);
struct ship* leftship(const struct unit*);
bool can_survive(const struct unit* u, const struct region* r);
void move_unit(struct unit* u, struct region* target,
    struct unit** ulist);

struct building* inside_building(const struct unit* u);

/* cleanup code for this module */
void free_units(void);
void u_setfaction(struct unit* u, struct faction* f);
void u_freeorders(struct unit* u);
void set_number(struct unit* u, int count);

int invisible(const struct unit* target, const struct unit* viewer);
void free_unit(struct unit* u);

void name_unit(struct unit* u);
struct unit* unit_create(int id);
struct unit* create_unit(struct region* r1, struct faction* f,
    int number, const struct race* rc, int id, const char* dname,
    struct unit* creator);

void uhash(struct unit* u);
void uunhash(struct unit* u);
struct unit* ufindhash(int i);

const char* unit_getname(const struct unit* u);
void unit_setname(struct unit* u, const char* name);
const char* unit_getinfo(const struct unit* u);
void unit_setinfo(struct unit* u, const char* name);
int unit_getid(const unit* u);
void unit_setid(unit* u, int id);
int unit_gethp(const unit* u);
void unit_sethp(unit* u, int id);
enum status_t unit_getstatus(const unit* u);
void unit_setstatus(unit* u, enum status_t status);
int unit_getweight(const unit* u);
int unit_getcapacity(const unit* u);
void unit_addorder(unit* u, struct order* ord);
int unit_max_hp(const struct unit* u);
void scale_number(struct unit* u, int n);

void remove_empty_units_in_region(struct region* r);
void remove_empty_units(void);

struct unit* findunit(int n);
struct unit* findunitr(const struct region* r, int n);

void default_name(const unit* u, char name[], int len);
const char* unitname(const struct unit* u);
char* write_unitname(const struct unit* u, char* buffer, size_t size);
bool unit_name_equals_race(const struct unit* u);

void unit_convert_race(struct unit* u, const struct race* rc, const char* rcname);
void translate_orders(struct unit* u, const struct locale* lang, struct order** list, bool del);

/* getunit results: */
#define GET_UNIT 0
#define GET_NOTFOUND 1
#define GET_PEASANTS 2

int getunit(const struct region* r, const struct faction* f, struct unit** uresult);
int read_unitid(const struct faction* f, const struct region* r);

/* !< sets combatstatus of a unit */
bool has_horses(const struct unit* u);
int maintenance_cost(const struct unit* u);
bool has_limited_skills(const struct unit* u);
bool is_limited_skill(enum skill_t sk);

#endif
