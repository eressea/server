#ifndef H_GC_RANDENC
#define H_GC_RANDENC
#ifdef __cplusplus
extern "C" {
#endif

    struct region;

    /** Plagues **/
#define PLAGUE_CHANCE      0.1 /* Seuchenwahrscheinlichkeit (siehe plagues()) */
#define PLAGUE_VICTIMS     0.2 /* % Betroffene */
#define PLAGUE_HEALCHANCE  0.25        /* Wahrscheinlichkeit Heilung */
#define PLAGUE_HEALCOST    30   /* Heilkosten */
    void plagues(struct region *r);
    void randomevents(void);

#ifdef __cplusplus
}
#endif
#endif
