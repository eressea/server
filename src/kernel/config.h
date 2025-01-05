#pragma once
#ifndef ERESSEA_CONF_H
#define ERESSEA_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

    struct locale;
    struct _dictionary_;

    /* alle vierstelligen zahlen: */
#define MAX_CONTAINER_NR (36*36*36*36-1)

#define DISPLAYSIZE         4096 /* max. Laenge einer Beschreibung, incl trailing 0 */
#define ORDERSIZE           4096 /* max. length of an order */
#define NAMESIZE            128 /* max. Laenge eines Namens, incl trailing 0 */
#define IDSIZE              16  /* max. Laenge einer no (als String), incl trailing 0 */
#define OBJECTIDSIZE        (NAMESIZE+5+IDSIZE) /* max. Laenge der Strings, die
     * von struct unitname, etc. zurueckgegeben werden. ohne die 0 */

#define fval(u, i) ((u)->flags & (i))
#define fset(u, i) ((u)->flags |= (i))
#define freset(u, i) ((u)->flags &= ~(i))

    const char * game_name(void);
    const char * game_mailcmd(void);
    int game_id(void);
    /* returns a value between [0..xpct_2], generated with two dice */

    bool forbiddenid(int id);
    int newcontainerid(void);

    bool rule_region_owners(void);
    int rule_alliance_limit(void);
    int rule_faction_limit(void);
#define HARVEST_WORK  0x02
#define HARVEST_TAXES 0x01
    int rule_blessed_harvest(void);
#define GIVE_SELF 1
#define GIVE_PEASANTS 2
#define GIVE_LUXURIES 4
#define GIVE_HERBS 8
#define GIVE_GOODS 16
#define GIVE_ONDEATH 32
#define GIVE_ALLITEMS (GIVE_GOODS|GIVE_HERBS|GIVE_LUXURIES)
#define GIVE_DEFAULT (GIVE_SELF|GIVE_PEASANTS|GIVE_LUXURIES|GIVE_HERBS|GIVE_GOODS)
    int rule_give(void);

    /* grammatik-flags: */
#define GF_NONE 0
    /* singular, ohne was dran */
#define GF_PLURAL 1
    /* Angaben in Mehrzahl */
#define GF_ARTICLE 8
    /* der, die, eine */
#define GF_SPECIFIC 16
    /* der, die, das vs. ein, eine */
#define GF_DETAILED 32
    /* mehr Informationen. z.b. strasse zu 50% */
#define GF_PURE 64
    /* untranslated */

    const char *datapath(void);
    void set_datapath(const char *path);

    const char *basepath(void);
    void set_basepath(const char *);

    const char *reportpath(void);
    void set_reportpath(const char *);

    int create_directories(void);

    void kernel_init(void);
    void kernel_done(void);

    /* globale settings des Spieles */
    typedef struct settings {
        void *vm_state;
    } settings;

    extern settings global;

    void config_set(const char *key, const char *value);
    void config_set_int(const char *key, int value);
    void config_set_from(const struct _dictionary_ *d, const char *keys[]);
    const char *config_get(const char *key);
    int config_get_int(const char *key, int def);
    double config_get_flt(const char *key, double def);
    bool config_token(const char *key, const char *tok);
    bool config_changed(int *cache_key);

    char * join_path(const char *p1, const char *p2, char *dst, size_t len);

    void free_config(void);
    void free_ids(void);

    struct params;

    void params_set(struct params** p, const char* key, const char* value);
    const char* params_get(const struct params* p, const char* key);
    int params_get_int(const struct params* p, const char* key, int def);
    int params_check(const struct params* p, const char* key, const char* searchvalue);
    double params_get_flt(const struct params* p, const char* key, double def);
    void params_free(struct params** pp);


#ifdef __cplusplus
}
#endif
#endif
