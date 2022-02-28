#ifndef H_KRNL_BUILD
#define H_KRNL_BUILD

#include "direction.h"
#include "skill.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* Die enums fuer Gebauede werden nie gebraucht, nur bei der Bestimmung
     * des Schutzes durch eine Burg wird die Reihenfolge und MAXBUILDINGS
     * wichtig
     */

    typedef struct requirement {
        const struct resource_type *rtype;
        int number;
    } requirement;

    typedef struct construction {
        skill_t skill;              /* skill req'd per point of size */
        int minskill;               /* skill req'd per point of size */

        int maxsize;                /* maximum size of this type */
        int reqsize;                /* size of object using up 1 set of requirement. */
        requirement *materials;     /* material req'd to build one object */
    } construction;

    void construction_init(struct construction *con, int minskill, skill_t sk, int reqsize, int maxsize);
    void free_construction(struct construction *cons);
    int destroy_cmd(struct unit *u, struct order *ord);
    int leave_cmd(struct unit *u, struct order *ord);

    void build_road(struct unit *u, int size, direction_t d);
    void create_ship(struct unit *u, const struct ship_type *newtype,
        int size, struct order *ord);
    void continue_ship(struct unit *u, int size);

    struct building *getbuilding(const struct region *r);
    struct ship *getship(const struct region *r);

    void shash(struct ship *sh);
    void sunhash(struct ship *sh);
    int roqf_factor(void);

    int build(struct unit *u, int number, const construction * ctype, int completed, int want, int skill_mod);
    int maxbuild(const struct unit *u, const construction * cons);
    struct message *msg_materials_required(struct unit *u, struct order *ord,
        const struct construction *ctype, int multi);

    /** error messages that build may return: */
#define ELOWSKILL -1
#define ENEEDSKILL -2
#define ECOMPLETE -3
#define ENOMATERIALS -4

#ifdef __cplusplus
}
#endif
#endif
