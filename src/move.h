#pragma once
#include "kernel/direction.h"
#include <stdbool.h>

struct attrib_type;
struct building_type;
struct locale;
struct order;
struct region;
struct region_list;
struct ship;
struct unit;

extern struct attrib_type at_shiptrail;
extern int *storms;

/* Bewegungsweiten: */
#define BP_WALKING 4
#define BP_RIDING  6
#define BP_UNICORN 9
#define BP_DRAGON  4
#define BP_NORMAL 3
#define BP_ROAD   2

#define HORSES_PER_CART    2 /* number of horses for a cart */
#define STRENGTHMULTIPLIER  50   /* multiplier for trollbelt */

/* ein mensch wiegt 10, traegt also 5, ein pferd wiegt 50, traegt also 20. ein
** wagen wird von zwei pferden gezogen und traegt total 140, davon 40 die
** pferde, macht nur noch 100, aber samt eigenem gewicht (40) macht also 140. */

/* movewhere error codes */
enum {
    E_MOVE_OK = 0,              /* possible to move */
    E_MOVE_NOREGION,            /* no region exists in this direction */
    E_MOVE_BLOCKED              /* cannot see this region, there is a blocking connection. */
};
int movewhere(const struct unit *u, const char *token,
struct region *r, struct region **resultp);
direction_t reldirection(const struct region *from, const struct region *to);

int personcapacity(const struct unit *u);
void movement(void);
int enoughsailors(const struct ship *sh, int sumskill);
bool canswim(struct unit *u);
bool canfly(struct unit *u);
bool is_transporting(const struct unit* ut, const struct unit* u);
void leave_trail(struct ship *sh, struct region *from,
                        struct region_list *route);
void move_ship(struct ship *sh, struct region *from,
struct region *to, struct region_list *route);
void follow_cmds(struct unit *u);
struct unit *owner_buildingtyp(const struct region *r,
    const struct building_type *bt);
bool move_blocked(const struct unit *u, const struct region *src,
    const struct region *dest);
bool can_takeoff(const struct ship * sh, const struct region * from, const struct region * to);
void move_cmd(struct unit * u, struct order * ord);
void move_cmd_ex(struct unit * u, struct order * ord, const char *directions);
int follow_ship(struct unit * u, struct order * ord);

#define SA_HARBOUR_ALLOWED 1
#define SA_ALLOWED 0
#define SA_INSECT_DENIED -1
#define SA_DENIED -2
#define SA_HARBOUR_DENIED -3
#define SA_HARBOUR_DISABLED -4

int check_ship_allowed(struct ship *sh, const struct region * r);
direction_t drift_target(struct ship *sh);
struct order * cycle_route(struct order * ord, const struct locale *lang, int gereist);
struct order * make_movement_order(const struct locale *lang, direction_t steps[], int length);

typedef struct capacities {
    int animals, vehicles;
    int acap, vcap;
} capacities;

void get_transporters(const struct item *itm, struct capacities *cap);
int movement_speed(const struct unit *u, const struct capacities *cap);
int walkingcapacity(const struct unit *u, const struct capacities *cap);
