#include <platform.h>

/* kernel includes */
#include <kernel/order.h>
#include <util/parser.h>

/* lua includes */
#include <tolua.h>

#include <stdlib.h>

static int tolua_order_get_token(lua_State *L) {
    order *ord = (order *)tolua_tousertype(L, 1, 0);
    int n = (int)tolua_tonumber(L, 2, 0);
    const char * str = 0;
    init_order(ord);
    while (n-->0) {
        str = getstrtoken();
        if (!str) {
            return 0;
        }
    }

    tolua_pushstring(L, str);
    return 1;
}

void tolua_order_open(lua_State * L)
{
    /* register user types */
    tolua_usertype(L, TOLUA_CAST "order");

    tolua_module(L, NULL, 0);
    tolua_beginmodule(L, NULL);
    {
        tolua_cclass(L, TOLUA_CAST "order", TOLUA_CAST "order", TOLUA_CAST "",
            NULL);
        tolua_beginmodule(L, TOLUA_CAST "order");
        {
            tolua_function(L, TOLUA_CAST "token", tolua_order_get_token);
        }
        tolua_endmodule(L);
    }
    tolua_endmodule(L);
}

