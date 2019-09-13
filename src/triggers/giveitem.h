#ifndef GIVEITEM_H
#define GIVEITEM_H
#ifdef __cplusplus
extern "C" {
#endif

    /* all types we use are defined here to reduce dependencies */
    struct trigger_type;
    struct trigger;
    struct unit;
    struct item_type;

    extern struct trigger_type tt_giveitem;

    struct trigger *trigger_giveitem(struct unit *mage,
        const struct item_type *itype, int number);

#ifdef __cplusplus
}
#endif
#endif
