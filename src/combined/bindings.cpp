#include "common/settings.h"
#include "common/config.h"
#include "stdafx.hpp"

#include "eressea/server.cpp"

#ifdef BINDINGS_LUABIND
#include "eressea/lua/alliance.cpp"
#include "eressea/lua/building.cpp"
#include "eressea/lua/eressea.cpp"
#include "eressea/lua/event.cpp"
#include "eressea/lua/faction.cpp"
#include "eressea/lua/gamecode.cpp"
#include "eressea/lua/gm.cpp"
#include "eressea/lua/item.cpp"
#include "eressea/lua/message.cpp"
#include "eressea/lua/objects.cpp"
#include "eressea/lua/region.cpp"
#include "eressea/lua/script.cpp"
#include "eressea/lua/ship.cpp"
#include "eressea/lua/spell.cpp"
#include "eressea/lua/test.cpp"
#include "eressea/lua/unit.cpp"
#endif
