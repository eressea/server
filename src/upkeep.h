#ifndef UPKEEP_H
#define UPKEEP_H
#ifdef __cplusplus
extern "C" {
#endif

    struct region;
    struct unit;
    void get_food(struct region * r);
    int lifestyle(const struct unit * u);

    enum {
        FOOD_FROM_PEASANTS = 1,
        FOOD_FROM_OWNER = 2,
        FOOD_IS_FREE = 4
    };


#ifdef __cplusplus
}
#endif
#endif
