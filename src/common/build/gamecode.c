#include <settings.h>
#include <platform.h>
#include "stdafx.h"

#ifdef BINDINGS_TOLUA
#include <bindings/bindings.c>
#include <bindings/bind_attrib.c>
#include <bindings/bind_sqlite.c>
#include <bindings/bind_unit.c>
#include <bindings/bind_ship.c>
#include <bindings/bind_building.c>
#include <bindings/bind_region.c>
#include <bindings/bind_faction.c>
#include <bindings/bind_message.c>
#include <bindings/bind_hashtable.c>
#include <bindings/bind_gmtool.c>
#include <bindings/bind_storage.c>
#include <bindings/helpers.c>
#endif

#include <gamecode/archetype.c>
#include <gamecode/creation.c>
#include <gamecode/creport.c>
#include <gamecode/economy.c>
#include <gamecode/give.c>
#include <gamecode/items.c>
#include <gamecode/laws.c>
#include <gamecode/luck.c>
#include <gamecode/market.c>
#include <gamecode/monster.c>
#include <gamecode/randenc.c>
#include <gamecode/report.c>
// #include <gamecode/spells.c>
#include <gamecode/spy.c>
#include <gamecode/study.c>
#include <gamecode/summary.c>
#include <gamecode/xmlreport.c>

#include <races/dragons.c>
#include <races/illusion.c>
#include <races/races.c>
#include <races/zombies.c>

#include <attributes/alliance.c>
#include <attributes/attributes.c>
#include <attributes/fleechance.c>
#include <attributes/follow.c>
#include <attributes/giveitem.c>
#include <attributes/gm.c>
#include <attributes/hate.c>
#include <attributes/iceberg.c>
#include <attributes/key.c>
#include <attributes/matmod.c>
#include <attributes/movement.c>
#include <attributes/moved.c>
#include <attributes/object.c>
#include <attributes/option.c>
#include <attributes/orcification.c>
#include <attributes/otherfaction.c>
#include <attributes/overrideroads.c>
#include <attributes/racename.c>
#include <attributes/raceprefix.c>
#include <attributes/reduceproduction.c>
#include <attributes/targetregion.c>
#include <attributes/viewrange.c>
#include <attributes/variable.c>

#include <items/artrewards.c>
#include <items/demonseye.c>
#include <items/itemtypes.c>
#include <items/phoenixcompass.c>
#include <items/seed.c>
#include <items/weapons.c>
#include <items/xerewards.c>

#include <spells/alp.c>
#include <spells/buildingcurse.c>
// #include <spells/combatspells.c>
#include <spells/regioncurse.c>
#include <spells/shipcurse.c>
// #include <spells/spells.c>
#include <spells/unitcurse.c>

#include <triggers/changefaction.c>
#include <triggers/changerace.c>
#include <triggers/clonedied.c>
#include <triggers/createcurse.c>
#include <triggers/createunit.c>
#include <triggers/gate.c>
#include <triggers/giveitem.c>
#include <triggers/killunit.c>
#include <triggers/removecurse.c>
#include <triggers/shock.c>
#include <triggers/timeout.c>
#include <triggers/triggers.c>
#include <triggers/unguard.c>
#include <triggers/unitmessage.c>
