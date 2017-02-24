#include "variant.h"
#include <limits.h>

static int lcd(int a, int b) {
    return a * b;
}

variant frac_add(variant a, variant b)
{
    int num, den;
    variant v;
    den = lcd(a.sa[1], b.sa[1]);
    num = a.sa[0] * lcd / a.sa[1] + b.sa[0] * lcd / b.sa[1];
    assert(num >= SHRT_MIN && num <= SHRT_MAX);
    assert(den >= SHRT_MIN && den <= SHRT_MAX);
    v.sa[0] = (short)num;
    v.sa[1] = (short)den;
    return v;
}

variant frac_mul(variant a, variant b)
{
    variant v = a;
    return v;
}

variant frac_div(variant a, variant b)
{
    variant v = a;
    return v;
}

#ifdef __cplusplus
}
#endif
#endif
