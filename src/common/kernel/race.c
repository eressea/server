/* vi: set ts=2:
 *
 *	$Id: race.c,v 1.5 2001/02/14 08:35:12 katze Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */
#include <config.h>
#include "eressea.h"
#include "race.h"

#include <races/zombies.h>
#include <races/dragons.h>
#include <races/illusion.h>

#include "alchemy.h"
#include "build.h"
#include "building.h"
#include "faction.h"
#include "item.h"
#include "magic.h"
#include "region.h"
#include "spell.h"
#include "unit.h"
#include "names.h"
#include "pathfinder.h"
#include "ship.h"
#include "skill.h"

/* util includes */
#include <attrib.h>

/* libc includes */
#include <stdio.h>
#include <math.h>

/* TODO: Tragkraft in die Struktur */

/* struct racedata{
 * magres, maxaura, regaura,
 * {4 namen},
 * heimat, rekr.kost, unterhalt, splitsize, weight, speed
 * hitpoints, damage, armor, at_default, df_default, at_bonus, df_bonus,
 * attack[6]
 * Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
 * Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK
 * nonplayer,use_armor,
 * flags,
 * battle_flags,
 * ec(onomic)_flags,
 * generate_name,
 * familiars
 * */

/* at_default, df_default
 * Angriffs- bzw. Verteidigungswert von Unbewaffneten dieser Rasse, die
 * nicht waffenlosen Kampf können. */
/* at_bonus, df_bonus
 * Angriffs- bzw. Verteidigungsbonus auf den Kampfskill, der zusätzlich
 * zum Talentwert gilt. */

/** dragon movement **/
boolean
allowed_dragon(const region * src, const region * target)
{
	if (src->terrain==T_GLACIER && target->terrain == T_OCEAN) return false;
	return allowed_fly(src, target);
}

struct racedata race[MAXRACES] =
{
	{
		/* const char *name[4] */
		{"Zwerg", "Zwerge", "Zwergen", "Zwergen"},

		/* Magieresistenz (0=Normal) */
		0.05,

		/* Maximale Aura (1=Durchschnitt) */
		1.00,

		/* Auraregeneration (1=Durchschnitt) */
		0.50,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		90, 10,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		24,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d5",

		/* Natürliche Rüstung */
		0,

		/* Angriff, Verteidigung unbewaffnet */
		-2, -2,

		/* Bonus Angriff, Verteidigung */
		0, 0,

		/* Angriffe */
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 0, 2,-1,   2, 1,-1, 2,  -2,-2,-2,-2,   2,-1, 1,-2,
			0, 0, 2, 2,   0,-1,-1, 2,   0, 0, 1, 0,   -99
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		false,

		/* Flags */
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,

		/* Battle_flags */
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,

		/* Economic-Flags */
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{RC_TUNNELWORM, RC_EAGLE, RC_OWL, RC_HOUSECAT, RC_WARG, RC_RAT}
	},

	{
		{"Elf", "Elfen", "Elfen", "Elfen"},
		0.10, 1.00, 1.25,
		130, 10, 10000, 1000, 1.0,
		18, "1d5", 0, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-1, 0,-2, 2,  -1, 0, 0,-2,   2, 1, 1, 0,  -1,-1, 0,-1,
			 0, 0,-1,-1,   0, 1, 0, 0,   0, 1, 0, 0,  -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_HOUSECAT, RC_FEY, RC_OWL, RC_NYMPH, RC_UNICORN, RC_IMP}
	},
	{
		{"Ork", "Orks", "Orks", "Ork"},
		-0.05, 1.00, 1.00,
		50, 10, 10000, 1000, 1.0,
		24, "1d5", 0, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			1, 0, 1, 0,   1,-3, 1, 0,  -2,-1,-1, 0,   1,-1, 0,-1,
			0,-1, 1, 0,   1, 0,-2, 2,  -1, 0, 1, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_GOBLIN, RC_WRAITH, RC_IMP, RC_RAT, RC_WARG, RC_DAEMON}
	},
	{
		{"Goblin", "Goblins", "Goblins", "Goblin"},
		-0.05, 1.00, 1.00,
		40, 10, 10000, 1000, 1.0,
		16, "1d5", 0, -2, 0, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			1, 0, 1, 0,   1,-1, 0, 1,   0,-1, 0, 0,   0,-2, 0,-2,
			0, 0, 0,-2,  -2, 1,-1, 0,  -1, 0, 0, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_RAT, RC_PSEUDODRAGON, RC_IMP, RC_RAT, RC_RAT, RC_IMP}
	},
	{
		{"Mensch", "Menschen", "Menschen", "Menschen"},
		0.00, 1.00, 1.00,
		75, 10, 10000, 1000, 1.0,
		20, "1d5", 0, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 1, 0, 0,  -1, 0, 0, 0,   0, 1, 0, 1,
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_WARG, RC_DREAMCAT, RC_OWL, RC_OWL, RC_EAGLE, RC_IMP}
	},
	{
		{"Troll", "Trolle", "Trollen", "Troll"},
		0.10, 1.00, 1.00,
		90, 10, 10000, 2000, 1.0,
		30, "1d5+3", 1, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 2,-2,   2, 0, 0, 2,  -1, 0,-1,-2,   2,-1, 1,-1,
			0,-3, 2, 2,  -1,-3,-1, 0,   0,-1, 1, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_TUNNELWORM, RC_RAT, RC_RAT, RC_EAGLE, RC_TUNNELWORM, RC_WARG}
	},
	{
		{"Dämon", "Dämonen", "Dämonen", "Dämonen"},
		0.15, 1.00, 1.25,
		150, 10, 10000, 1000, 1.0,
		50, "1d5", 2, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_DAZZLE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			2, 0, 0, 0,   0,-3, 1, 0,  -3, 1,-3,-1,   0,-1, 1,-1,
			1, 0, 0, 0,  -1, 1,-3, 1,  -2, 1, 1, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_IMP, RC_IMP, RC_WRAITH, RC_RAT, RC_WARG, RC_IMP}
	},
	{
		{"Insekt", "Insekten", "Insekten", "Insekten"},
		0.05, 1.00, 1.00,
		80, 10, 10000, 1000, 1.0,
		24, "1d5", 2, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 1, 1,-2,   2,-1, 1, 0,   1, 0,-3,-3,   2, 0,-1, 0,
			1, 0, 0,-1,  -1,-1,-2, 0,   0, 1, 0, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_WRAITH, RC_RAT, RC_OWL, RC_RAT, RC_TUNNELWORM, RC_IMP}
	},
	{
		{"Halbling", "Halblinge", "Halblingen", "Halblings"},
		0.05, 1.00, 1.00,
		80, 10, 10000, 1000, 1.0,
		18, "1d5", 0, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 1, 1,-1,   1, 2, 0,-1,   2, 0,-1,-1,   0,-1,-1,-2,
			-1,1, 0, 1,   0, 1, 1, 0,   2, 1,-1, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_OWL, RC_RAT, RC_EAGLE, RC_PSEUDODRAGON, RC_EAGLE, RC_RAT}
	},
	{
		{"Katze", "Katzen", "Katzen", "Katzen"},
		0.00, 1.00, 1.00,
		90, 10, 10000, 1000, 1.0,
		20, "1d5", 0, -2, -2, 0, 1,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-1,0,-2, 0,  -1, 0, 0,-1,   1, 0, 0, 0,  -1,-1, 0,-2,
			0, 2,-1,-1,   0, 1, 0, 0,   0, 2, 1, 0,   -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_HOUSECAT, RC_DREAMCAT, RC_HOUSECAT, RC_PSEUDODRAGON,
			RC_TIGER, RC_HELLCAT}
	},
	{
		{"Meermensch", "Meermenschen", "Meermenschen", "Meermenschen"},
		0.00, 1.00, 1.00,
		80, 10, 10000, 1000, 1.0,
		20, "1d5", 0, -2, -2, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0,-2, 0,  -1, 2, 0, 0,   0, 0, 0, 0,  -1, 3, 0, 3,
			0, 0, 0,-1,   0, 0, 0, 0,   0, 0, 0, 0,  -99
		},
		false,
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{RC_OCEANTURTLE, RC_DOLPHIN, RC_OCEANTURTLE, RC_DOLPHIN,
			RC_DOLPHIN, RC_DOLPHIN}
	},
	{
		{"Untoter", "Untote", "Untoten", "Untoten"},
		0.00, 1.00, 1.00,
		2, 0, 20000, 1000, 1.0,
		20, "1d7", 0, 0, 0, 1, 1,
		{
			{AT_NATURAL, {"1d7"}, 0}, {AT_DAZZLE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0
		},
		true,
		(RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),
		(BF_EQUIPMENT | BF_MAGIC_EQUIPMENT),
		CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&untoten_name, &age_undead
	},
	{
		{"Illusion", "Illusionen", "Illusions", "Illusions"},
		0.00, 0.00, 0.00,
		50, 0, 999999, 0, 1.0,
		1, "1d1", 0, 0, 0, 0, 0,
		{
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0
		},
		true, RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOWEAPONS, 0, 0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		NULL, &age_illusion
	},
	{
		{"Jungdrache", "Jungdrachen", "Jungdrachen", "Jungdrachen"},
		0.50, 1.00, 1.00,
		10000, 0, 6, 20000, 1.0,
		300, "2d15", 4, 0, 0, 4, 4,
		{
			{AT_NATURAL, {"1d30"}, 0}, {AT_NATURAL, {"1d30"}, 0}, {AT_NATURAL, {"1d30"}, 0},
			{AT_SPELL, {(void *)SPL_FIREDRAGONODEM}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   4, 0, 0, 0,   0, 0, 0, 0, 0
		},
		true,
		RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_LEARN|RCF_FLY|RCF_WALK|RCF_NOTEACH,
		BF_MAGIC_EQUIPMENT,
		GETITEM | HOARDMONEY | CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&drachen_name, &age_firedragon, &allowed_dragon
	},
	{
		{"Drache", "Drachen", "Drachen", "Drachen"},
		0.70, 1.00, 2.00,
		50000, 0, 2, 60000, 1.5,
		900, "2d30", 6, 0, 0, 7, 7,
		{
			{AT_NATURAL, {"2d20"}, 0}, {AT_NATURAL, {"2d20"}, 0}, {AT_NATURAL, {"3d30"}, 0},
			{AT_SPELL, {(void *)SPL_DRAGONODEM}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0, 	0, 0, 0, 0, 	0, 0, 0, 0, 	0, 0, 0, 0,
			0, 0, 0, 0, 	8, 0, 0, 0, 	0, 5, 0, 0, 0
		},
		true,
		( RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_LEARN|RCF_FLY|RCF_WALK|RCF_NOTEACH ),
		( BF_MAGIC_EQUIPMENT ),
		GETITEM | HOARDMONEY | CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&drachen_name, &age_dragon, &allowed_dragon
	},
	{
		{"Wyrm", "Wyrme", "Wyrmen", "Wyrm"},
		0.90, 1.00, 3.00,
		250000, 0, 1, 180000, 1.0,
		2700, "2d60", 8, 0, 0, 10, 10,
		{
			{AT_NATURAL, {"3d20"}, 0}, {AT_NATURAL, {"3d20"}, 0}, {AT_NATURAL, {"5d30"}, 0},
			{AT_SPELL, {(void *)SPL_WYRMODEM}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0, 	0, 0, 0, 0, 	0, 0, 0, 0,
			0, 0, 0, 0,  12, 0, 0, 0,   0,10, 0, 0, 0
		},
		true,
		( RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_LEARN|RCF_FLY|RCF_WALK|RCF_NOTEACH ),
		( BF_MAGIC_EQUIPMENT ),
		GETITEM | HOARDMONEY | CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&drachen_name, NULL, &allowed_dragon
	},
	{
		{"Ent", "Ents", "Ents", "Ent"},
		0.25, 1.00, 0.50,
		5000, 0, 1000, 5000, 1.0,
		50, "2d4+12", 4, 0, 0, 9, 7,
		{
			{AT_NATURAL, {"2d12"}, 0}, {AT_NATURAL, {"2d12"}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},
		true,
		( RCF_SCAREPEASANTS | RCF_MOVERANDOM | RCF_LEARN | RCF_WALK | RCF_NOTEACH ),
		( BF_MAGIC_EQUIPMENT ),
		CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Katzendrache", "Katzendrachen", "Katzendrachen", "Katzendrachen"},
		0.90, 1.00, 1.00,
		500000, 0, 1, 20000, 1.0,
		20, "2d40", 0, 0, 0, 0, 50,
		{
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},
		true,
		( RCF_FLY | RCF_WALK | RCF_NOTEACH),
		0,
		GIVEITEM | GIVEPERSON | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Dracoid", "Dracoide", "Dracoiden", "Dracoiden"},
		0.00, 1.00, 1.00,
		50, 0, 10000, 1000, 1.0,
		24, "1d5", 0, -2, -2, 0, 0,
		{
			{AT_NATURAL, {"1d6"}, 0}, {AT_NATURAL, {"1d6"}, 0}, {AT_STANDARD, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true,
		RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_LEARN|RCF_WALK|RCF_NOTEACH,
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,
		GIVEITEM | GIVEPERSON | GETITEM | CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&dracoid_name, NULL
	},
	/* nur fuer den mapper: */
	{
		{"Spezial", "Spezial", "Spezial", "Spezial"},
		0.00, 0.00, 0.00,
		0, 0, 1, 0, 0.0,
		1, "1d4", 0, -2, -2, 0, 0,
		{
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true, 0, 0, 0,
	{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Zauber", "Zauber", "Zauber", NULL},
		0.00, 1.00, 0.10,
		0, 0, 1, 0, 0.0,
		1, "1d4", 0, -2, -2, 0, 0,
		{
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true, 0, 0, 0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},

	/* ende Spezialunits */

	{
		{"Eisengolem", "Eisengolems", "Eisengolems", NULL},
		0.25, 1.00, 0.10,
		5000, 0, 50, 10000, 1.0,
		50, "2d10+4", 2, 0, 0, 4, 2,
		{
			{AT_NATURAL, {"2d8+4"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0},
		true,
		RCF_WALK | RCF_NOLEARN | RCF_NOTEACH, 0,
		GIVEITEM | GIVEPERSON,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Steingolem", "Steingolems", "Steingolems", NULL},
		0.25, 1.00, 0.10,
		5000, 0, 50, 10000, 1.0,
		60, "2d12+6", 4, 0, 0, 4, 2,
		{
			{AT_NATURAL, {"2d10+4"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true,
		RCF_WALK | RCF_NOLEARN | RCF_NOTEACH, 0,
		GIVEITEM | GIVEPERSON,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Schattendämon", "Schattendämonen", "Schattendämonen", NULL},
		0.75, 1.00, 1.00,
		5000, 0, 1000, 0, 1.0,
		50, "2d4", 3, 0, 0, 8, 11,
		{
			{AT_NATURAL, {"2d3"}, 0}, {AT_DRAIN_ST, {"1d1"}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true,
		RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_LEARN|RCF_WALK|RCF_NOTEACH|RCF_DESERT,
		BF_MAGIC_EQUIPMENT,
		0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&shadow_name, NULL,
	},
	{
		{"Schattenmeister", "Schattenmeister", "Schattenmeistern", NULL},
		0.75, 1.00, 2.00,
		50000, 0, 50, 0, 1.0,
		150, "2d5", 4, 0, 0, 11, 13,
		{
			{AT_NATURAL, {"2d4"}, 0}, {AT_DRAIN_EXP, {"2d30"}, 0}, {AT_DRAIN_ST, {"1d2"}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true,
		RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_LEARN|RCF_WALK|RCF_NOTEACH|RCF_DESERT,
		BF_MAGIC_EQUIPMENT,
		0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&shadow_name, NULL,
	},
	{
		{"Bergwächter", "Bergwächter", "Bergwächtern", NULL},
		0.50, 1.00, 0.50,
		50000, 0, 1, 10000, 0.0,
		1000, "2d40", 12, 0, 0, 6, 8,
		{
			{AT_NATURAL, {"2d40"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true, RCF_CANNOTMOVE|RCF_NOLEARN|RCF_NOTEACH, 0,
		0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Alp", "Alps", "Alps", NULL},
		0.95, 1.00, 1.00,
		50000, 0, 1, 0, 1.5,
		20, "1d4", 2, 0, 0, 2, 20,
		{
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		true,
		(RCF_SEEKTARGET|RCF_FLY|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH),
		0,
		0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Kröte", "Kröten", "Kröten", NULL},
		0.20, 1.00, 1.00,
		50, 0, 1, 1, 1.0,
		10, "1d2", 0, -2, -2, 0, 0,
		{
			{AT_NATURAL, {"1d2"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-10,-10,-10,-10,  -10,-10,-10,-10,  -10,-10,-10,-10, -10,-10,-10,-10,
			-10,-10,-10,-10,  -10, +2,-10,-10,  -10,-10,-10,-10, 0
		},
		true,
		RCF_LEARN|RCF_ATTACKRANDOM|RCF_CANNOTMOVE,
		0,
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Hirntöter", "Hirntöter", "Hirntöter", NULL},
		0.90, 1.00, 1.00,
		50000, 0, 500, 1, 1.0,
		20, "0d0", 1, 0, 0, 6, 10,
		{
			{AT_DRAIN_EXP, {"3d15"}, 0}, {AT_DRAIN_ST, {"1d1"}, 0}, {AT_NATURAL, {"1d1"}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},
		true,
		( RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_LEARN|RCF_FLY|RCF_WALK|RCF_NOTEACH),
		( BF_INV_NONMAGIC),
		CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Bauer", "Bauern", "Bauern", NULL},
		0.00, 1.00, 1.00,
		50, 0, 10000, 10, 1.0,
		20, "1d6", 0, 0, 0, 0, 0,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},
		true,
		RCF_CANNOTMOVE|RCF_NOTEACH,
		( BF_EQUIPMENT | BF_MAGIC_EQUIPMENT ),
		CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		{"Wolf", "Wölfe", "Wölfen", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 1.0,
		20, "2d4", 0, 0, 0, 3, 1,
		{
			{AT_NATURAL, {"2d4"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0
		},
		true,
		(RCF_WALK|RCF_NOTEACH), (0), GIVEPERSON,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},

	/*************************************************************/
	/* Vertraute !                                               */
	/*************************************************************/
	/* RC_HOUSECAT */
	{
		{"Luchs", "Luchse", "Luchsen", NULL},
		0.00, 0.00, 0.00,
		50, 0, 99999, 5, 1.0,
		20, "2d3", 0, 0, 0, 4, 5,
		{
			{AT_NATURAL, {"2d3"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,     1, 0,  1,-99,   -99,-99,-99,-99,
			-99,  3,-99,-99,   -99,  3,-99,-99,   -99, 4,-99,  0,    0
		},
		true,
		(RCF_WALK|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_TUNNELWORM */
	{
		{"Tunnelwurm", "Tunnelwürmer", "Tunnelwürmern", NULL},
		0.80, 0.00, 0.00,
		50, 0, 99999, 30000, 1.0,
		300, "3d20", 6, 0, 0, 6, 1,
		{
			{AT_NATURAL, {"3d20"}, 0}, {AT_STRUCTURAL, {"1d10"}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99, 50,-99,   -99,-99, 50,-99,   -99, 0,-99,-99,   -99,-99,-99,-99,
			-99,-99, 50,-99,   -99,-99,-99,-99,   -99,-99,-99, 2, 0
		},
		true,
		(RCF_WALK|RCF_SCAREPEASANTS|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_EAGLE */
	{
		{"Adler", "Adler", "Adlern", NULL},
		0.00, 0.00, 0.00,
		50, 0, 9999, 5, 1.5,
		15, "2d3", 0, 0, 0, 6, 2,
		{
			{AT_NATURAL, {"2d3"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,   -99,-99,-99,-99,
			-99,  0,-99,-99,   -99,-99,-99,-99,   -99, 2,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_FLY|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_RAT */
	{
		{"Ratte", "Ratten", "Ratten", NULL},
		0.00, 0.00, 0.00,
		50, 0, 9999, 1, 1.0,
		5, "1d2", 0, 0, 0, 1, 1,
		{
			{AT_NATURAL, {"1d2"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,   -99,-99,-99,-99,
			-99,  5,-99,-99,   -99,  4,-99,-99,   -99, 2,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_PSEUDODRAGON */
	{
		{"Singdrache", "Singdrachen", "Singdrachen", NULL},
		0.99, 1.00, 1.00,
		50, 0, 9999, 10, 1.5,
		40, "2d4", 1, 0, 0, 3, 1,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NATURAL, {"2d4"}, 0}, {AT_NATURAL, {"2d4"}, 0},
			{AT_SPELL, {(void*)SPL_FIREDRAGONODEM}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 1,-99,-99,   -99,-99,-99,-99,
			-99,  0,-99,-99,   -99,  0,-99,-99,   -99, 0,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_FLY|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_NYMPH */
	{
		{"Nymphe", "Nymphen", "Nymphen", NULL},
		0.90, 1.00, 1.50,
		50, 0, 9999, 10, 1.0,
		15, "1d4", 0, 0, 0, 3, 1,
		{
			{AT_STANDARD, {NULL}, 0}, {AT_DRAIN_EXP, {"2d20"}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			 0, 0,-99, 2,   -99,-2,-99,-99,   4, 1, 5, 5,  -2,-99, 0,-2,
			 0, 2,-99,-99,   -2, 3, 10, -2,  -2, 2,-2,-1, 0
		},
		true,
		(RCF_WALK|RCF_NOTEACH),
		(BF_EQUIPMENT|BF_MAGIC_EQUIPMENT),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_UNICORN */
	{
		{"Einhorn", "Einhörner", "Einhörnern", NULL},
		0.90, 1.50, 1.50,
		50, 0, 9999, 50, 2.0,
		40, "2d4", 0, 0, 0, 6, 4,
		{
			{AT_NATURAL, {"3d12"}, 0}, {AT_NATURAL, {"2d4"}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 2,-99,-99,   -99,-99,-99,-99,
			-99,  0,-99,-99,     0,  4,-99,-99,   -99, 5,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_WARG */
	{
		{"Warg", "Warge", "Wargen", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 1.0,
		25, "2d6", 0, 0, 0, 6, 3,
		{
			{AT_NATURAL, {"2d6"}, 0}, {AT_NATURAL, {"1d4"}, 0}, {AT_NATURAL, {"1d4"}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  0,-99,-99,     0,  0,-99,-99,   -99, 2,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_SCAREPEASANTS|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_WRAITH */
	{
		{"Geist", "Geister", "Geistern", NULL},
		0.80, 0.50, 0.10,
		50, 0, 5000, 5, 1.0,
		30, "2d6", 5, 0, 0, 5, 8,
		{
			{AT_NATURAL, {"1d5"}, 0}, {AT_NATURAL, {"1d5"}, 0}, {AT_STANDARD, {NULL}, 0},
			{AT_DRAIN_EXP, {"2d30"}, 0}, {AT_DRAIN_ST, {"1d1"}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 1,-99, -2,  -99,-99, 0,-99,
			  0,  0,-99,-99,   -99,  0,-99,-99,   -99, 0,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_FLY|RCF_SCAREPEASANTS|RCF_NOTEACH),
		(BF_EQUIPMENT|BF_MAGIC_EQUIPMENT|BF_INV_NONMAGIC),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_IMP */
	{
		{"Teufelchen", "Teufelchen", "Teufelchen", NULL},
		0.50, 1.00, 0.00,
		50, 0, 5000, 5, 1.0,
		10, "1d4", 1, 0, 0, 5, 4,
		{
			{AT_NATURAL, {"1d4"}, 0}, {AT_NATURAL, {"1d4"}, 0}, {AT_STANDARD, {NULL}, 0},
			{AT_SPELL, {(void*)SPL_FIREDRAGONODEM}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 1,-99, -1,  -99,-99, 0,-99,
			  0,  1,-99,-99,   -99,  1,-99,-99,   -99, 1,  1,  0, 0
		},
		true, (RCF_FLY|RCF_WALK|RCF_NOTEACH), (BF_EQUIPMENT|BF_MAGIC_EQUIPMENT),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_DREAMCAT */
	{
		{"Traumkatze", "Traumkatzen", "Traumkatzen", NULL},
		0.50, 1.00, 1.00,
		50, 0, 5000, 5, 1.0,
		10, "1d5", 0, 0, 0, 5, 6,
		{
			{AT_NATURAL, {"1d5"}, 0}, {AT_NATURAL, {"1d5"}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 1,-99,-99,  -99,-99, 0,-99,
			  0,  1,-99,-99,   -99,  1,-99,-99,   -99, 1,  1,  0, 0
		},
		true,
		(RCF_FLY|RCF_WALK|RCF_NOTEACH), (BF_INV_NONMAGIC),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_FEY */
	{
		{"Fee", "Feen", "Feen", NULL},
		0.80, 1.00, 1.50,
		50, 0, 5000, 5, 1.0,
		6, "1d3", 0, 0, 0, 6, 14,
		{
			{AT_NATURAL, {"1d3"}, 0}, {AT_NATURAL, {"1d3"}, 0}, {AT_NATURAL, {"1d3"}, 0},
			{AT_NATURAL, {"1d3"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 1,-99, -1,  -99,-99, -1,-99,
			 -1,  2,-99,-99,   -99,  5,-99,-99,   -99, 2,-99,  0, 0
		},
		true,
		(RCF_FLY|RCF_WALK|RCF_NOTEACH), (BF_EQUIPMENT|BF_MAGIC_EQUIPMENT),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_OWL */
	{
		{"Eule", "Eulen", "Eulen", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 1.0,
		9, "1d4", 0, 0, 0, 2, 4,
		{
			{AT_NATURAL, {"1d4"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  1,-99,-99,   -99,  1,-99,-99,   -99, 5,-99,  0, 0
		},
		true,
		(RCF_FLY|RCF_WALK|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_HELLCAT */
	{
		{"Höllenkatze", "Höllenkatzen", "Höllenkatzen", NULL},
		0.50, 0.00, 0.00,
		50, 0, 5000, 5, 1.0,
		40, "2d6", 0, 0, 0, 6, 4,
		{
			{AT_NATURAL, {"2d6"}, 0}, {AT_NATURAL, {"1d6"}, 0}, {AT_NATURAL, {"1d6"}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  0,-99,-99,     0,  0,-99,-99,   -99, 1,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_NOTEACH), (RCF_SCAREPEASANTS),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_TIGER */
	{
		{"Tiger", "Tiger", "Tigern", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 1.0,
		30, "2d6", 0, 0, 0, 6, 3,
		{
			{AT_NATURAL, {"2d6"}, 0}, {AT_NATURAL, {"1d6"}, 0}, {AT_NATURAL, {"1d6"}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  0,-99,-99,     0,  0,-99,-99,   -99, 1,-99,  0, 0
		},
		true,
		(RCF_WALK|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_DOLPHIN */
	{
		{"Delphin", "Delphine", "Delphinen", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 2.0,
		24, "1d6", 0, 0, 0, 5, 5,
		{
			{AT_NATURAL, {"1d6"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  0,-99,-99,     0,  0,-99,-99,   -99, 1,-99,  0, 0
		},
		true,
		(RCF_SWIM|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_OCEANTURTLE */
	{
		{"Riesenschildkröte", "Riesenschildkröten", "Riesenschildkröten", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 1.0,
		900, "2d50", 7, 0, 0, 10, 5,
		{
			{AT_NATURAL, {"2d50"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  0,-99,-99,     0,-99,-99,-99,   -99, 1,-99,  0, 0
		},
		true,
		(RCF_SWIM|RCF_WALK|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}, /* RC_KRAKEN */
	{
		{"Krake", "Kraken", "Kraken", NULL},
		0.00, 0.00, 0.00,
		50, 0, 5000, 5, 2.0,
		300, "2d10", 0, 0, 0, 7, 7,
		{
			{AT_NATURAL, {"2d10"}, 0}, {AT_NATURAL, {"1d10"}, 0}, {AT_NATURAL, {"1d10"}, 0},
			{AT_NATURAL, {"1d10"}, 0}, {AT_NATURAL, {"1d10"}, 0}, {AT_NATURAL, {"1d10"}, 0},
		},
		{
			-99,-99,-99,-99,   -99,-99,-99,-99,   -99, 0,-99,-99,  -99,-99,-99,-99,
			-99,  0,-99,-99,     0,-99,-99,-99,   -99, 1,-99,  0, 0
		},
		true,
		(RCF_SWIM|RCF_NOTEACH), (0),
		GIVEITEM | GETITEM,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	/*************************************************************/
	/* Ende Vertraute !                                          */
	/*************************************************************/

	/* RC_SEASERPENT */
	{
		{"Seeschlange", "Seeschlangen", "Seeschlangen", NULL},
		0.50, 1.00, 1.00,
		5000, 0, 6, 20000, 1.0,
		600, "2d15", 3, 0, 0, 4, 4,
		{
			{AT_NATURAL, {"1d30"}, 0}, {AT_NATURAL, {"1d30"}, 0}, {AT_NATURAL, {"1d30"}, 0},
			{AT_SPELL, {(void*)SPL_FIREDRAGONODEM}, 0}, {AT_STRUCTURAL, {"1d10"}, 0},
			{AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   4, 0, 0, 0,   0, 0, 0, 0, 0
		},
		true,
		RCF_KILLPEASANTS|RCF_SCAREPEASANTS|RCF_ATTACKRANDOM|RCF_LEARN|RCF_NOTEACH|RCF_SWIM|RCF_MOVERANDOM,
		(0),
		GETITEM | HOARDMONEY | CANGUARD,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},
		&drachen_name, NULL, &allowed_swim
	},

	/* Schattenritter, nur temporär für einen Kampf. 1 Hitpoint, keine
	 * Rüstung, kein Angriff. */
	{
		{"Schattenritter", "Schattenritter", "Schattenrittern", NULL},
		0.00, 0.00, 0.00,
		5, 0, 20000, 1000, 1.0,
		1, "1d1", 0, 0, 0, 1, 1,
		{
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0
		},
		true,
		(RCF_SCAREPEASANTS|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH),
		(BF_NOBLOCK),
		0,
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	{
		/* const char *name[4] */
		{"Zentaur", "Zentauren", "Zentauren", "Zentauren"},

		/* Magieresistenz (0=Normal) */
		0.00,

		/* Maximale Aura (1=Durchschnitt) */
		0.75,

		/* Auraregeneration (1=Durchschnitt) */
		0.75,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		100, 10,

		/* Splitsize */
		10000,

		/* Gewicht */
		5000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		30,

		/* Schaden AT_STANDARD unbewaffnet */
		"2d5",

		/* Natürliche Rüstung */
		0,

		/* Angriff, Verteidigung unbewaffnet */
		0, 0,

		/* Bonus Angriff, Verteidigung */
		0, 0,

		/* Angriffe */
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Stei,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1,-3, 1,  -2, 1, 0,-2,   1, 0, 4, 0,  -1,-3, 0,-3,
			1,-1,-1, 0,   0,-1, 0, 1,   1, 1, 1, 0,   0
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		RCF_WALK | RCF_LEARN | RCF_MOVERANDOM | RCF_ATTACKRANDOM | RCF_HORSE,

		/* Battle_flags */
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,

		/* Economic-Flags */
		GIVEITEM | GIVEPERSON | GIVEUNIT | GETITEM | REC_HORSES,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{RC_EAGLE, RC_FEY, RC_OWL, RC_UNICORN, RC_NYMPH, RC_IMP}
	},

	/* Skelette, einfachste Untotenart */
	{
		/* const char *name[4] */
		{"Skelett", "Skelette", "Skeletten", NULL},

		/* Magieresistenz (0=Normal) */
		0.10,

		/* Maximale Aura (1=Durchschnitt) */
		1.0,

		/* Auraregeneration (1=Durchschnitt) */
		1.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		0, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		500,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		20,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d7",

		/* Natürliche Rüstung */
		1,

		/* Angriff, Verteidigung unbewaffnet */
		1, 1,

		/* Bonus Angriff, Verteidigung */
		6, 6,

		/* Angriffe */
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 1, 0,
			1, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_SCAREPEASANTS|RCF_KILLPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),

		/* Battle_flags */
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT | BF_RES_PIERCE | BF_RES_CUT,

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},

		/* char *(*generate_name) (unit *) */
		&skeleton_name,

		/* Altern, wenn u->age%age == 0 */
		&age_skeleton,
	},

	/* Skelette, einfachste Untotenart */
	{
		/* const char *name[4] */
		{"Skelettherr", "Skelettherren", "Skelettherren", NULL},

		/* Magieresistenz (0=Normal) */
		0.30,

		/* Maximale Aura (1=Durchschnitt) */
		1.0,

		/* Auraregeneration (1=Durchschnitt) */
		1.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		2, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		60,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d7",

		/* Natürliche Rüstung */
		4,

		/* Angriff, Verteidigung unbewaffnet */
		6, 6,

		/* Bonus Angriff, Verteidigung */
		8, 8,

		/* Angriffe */
		{
			{AT_STANDARD, {NULL}, 0}, {AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 1, 0,
			1, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_SCAREPEASANTS|RCF_ABSORBPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),

		/* Battle_flags */
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT | BF_RES_PIERCE | BF_RES_CUT,

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},

		/* char *(*generate_name) (unit *) */
		&skeleton_name,

	},

	/* Zombies, Untotenart */
	{
		/* const char *name[4] */
		{"Zombie", "Zombies", "Zombies", NULL},

		/* Magieresistenz (0=Normal) */
		0.20,

		/* Maximale Aura (1=Durchschnitt) */
		1.0,

		/* Auraregeneration (1=Durchschnitt) */
		1.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		4, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		40,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d7",

		/* Natürliche Rüstung */
		1,

		/* Angriff, Verteidigung unbewaffnet */
		2, 2,

		/* Bonus Angriff, Verteidigung */
		5, 5,

		/* Angriffe */
		{
			{AT_STANDARD, {"1d7"}, 0},  {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 1, 0,
			1, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_SCAREPEASANTS|RCF_KILLPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),

		/* Battle_flags */
		BF_EQUIPMENT|BF_MAGIC_EQUIPMENT|BF_RES_PIERCE|BF_RES_CUT,

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},

		/* char *(*generate_name) (unit *) */
		&zombie_name,

		/* Altern, wenn u->age%age == 0 */
		&age_zombie,
	},

	/* Juju-Zombies, Untotenart */
	{
		/* const char *name[4] */
		{"Juju-Zombie", "Juju-Zombies", "Juju-Zombies", NULL},

		/* Magieresistenz (0=Normal) */
		0.50,

		/* Maximale Aura (1=Durchschnitt) */
		1.0,

		/* Auraregeneration (1=Durchschnitt) */
		1.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		8, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		80,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d7",

		/* Natürliche Rüstung */
		2,

		/* Angriff, Verteidigung unbewaffnet */
		6, 6,

		/* Bonus Angriff, Verteidigung */
		8, 8,

		/* Angriffe */
		{
			{AT_STANDARD, {"1d7"}, 0}, {AT_DRAIN_ST, {"1d1"}, 0}, {AT_DRAIN_ST, {"1d1"}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 1, 0,
			1, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_SCAREPEASANTS|RCF_ABSORBPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),

		/* Battle_flags */
		BF_EQUIPMENT|BF_MAGIC_EQUIPMENT|BF_RES_PIERCE|BF_RES_CUT|BF_RES_BASH,

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},

		/* char *(*generate_name) (unit *) */
		&zombie_name,

		/* Altern, wenn u->age%age == 0 */
		NULL,
	},


	/* Ghoule, Untotenart */
	{
		/* const char *name[4] */
		{"Ghoul", "Ghoule", "Ghoulen", NULL},

		/* Magieresistenz (0=Normal) */
		0.30,

		/* Maximale Aura (1=Durchschnitt) */
		1.0,

		/* Auraregeneration (1=Durchschnitt) */
		1.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		5, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		30,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d7",

		/* Natürliche Rüstung */
		1,

		/* Angriff, Verteidigung unbewaffnet */
		3, 3,

		/* Bonus Angriff, Verteidigung */
		3, 3,

		/* Angriffe */
		{
			{AT_NATURAL, {"2d6"}, 0}, {AT_DRAIN_ST, {"1d2"}, 0}, {AT_DRAIN_ST, {"1d2"}, 0},
			{AT_DRAIN_EXP, {"1d30"}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 1, 0,
			1, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_SCAREPEASANTS|RCF_ABSORBPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),

		/* Battle_flags */
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},

		/* char *(*generate_name) (unit *) */
		&ghoul_name,

		/* Altern, wenn u->age%age == 0 */
		&age_ghoul,
	},

	/* Ghasts, Untotenart */
	{
		/* const char *name[4] */
		{"Ghast", "Ghaste", "Ghasten", NULL},

		/* Magieresistenz (0=Normal) */
		0.60,

		/* Maximale Aura (1=Durchschnitt) */
		1.0,

		/* Auraregeneration (1=Durchschnitt) */
		1.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		5, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		60,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d7",

		/* Natürliche Rüstung */
		2,

		/* Angriff, Verteidigung unbewaffnet */
		6, 6,

		/* Bonus Angriff, Verteidigung */
		6, 6,

		/* Angriffe */
		{
			{AT_NATURAL, {"2d8"}, 0}, {AT_DRAIN_ST, {"1d5"}, 0}, {AT_DRAIN_ST, {"1d5"}, 0},
			{AT_DRAIN_EXP, {"1d30"}, 0}, {AT_DRAIN_EXP, {"1d30"}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 1, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 1, 0,
			1, 0, 0, 0,   1, 0, 0, 0,   0, 0, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_SCAREPEASANTS|RCF_ABSORBPEASANTS|RCF_ATTACKRANDOM|RCF_MOVERANDOM|RCF_WALK|RCF_NOLEARN|RCF_NOTEACH|RCF_NOHEAL),

		/* Battle_flags */
		BF_EQUIPMENT | BF_MAGIC_EQUIPMENT,

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE},

		/* char *(*generate_name) (unit *) */
		&ghoul_name,

		/* Altern, wenn u->age%age == 0 */
		NULL,
	},

	/* Museumsgeist */
	{
		/* const char *name[4] */
		{"Museumsgeist", "Museumsgeister", "Museumsgeistern", NULL},

		/* Magieresistenz (0=Normal) */
		1.0,

		/* Maximale Aura (1=Durchschnitt) */
		0.0,

		/* Auraregeneration (1=Durchschnitt) */
		0.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		5, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		50,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d4",

		/* Natürliche Rüstung */
		0,

		/* Angriff, Verteidigung unbewaffnet */
		10, 10,

		/* Bonus Angriff, Verteidigung */
		10, 10,

		/* Angriffe */
		{
			{AT_DRAIN_EXP, {"5d600"}, 0}, {AT_DRAIN_ST, {"5d5"}, 0}, {AT_DRAIN_EXP, {"5d600"}, 0},
			{AT_DRAIN_EXP, {"5d600"}, 0}, {AT_DRAIN_ST, {"5d5"}, 0}, {AT_DRAIN_EXP, {"5d600"}, 0}
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   1, 0, 0, 0,   0, 1, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_WALK|RCF_NOTEACH),

		/* Battle_flags */
		(0),

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	},
	/* Weihnachtsgnom */
	{
		/* const char *name[4] */
		{"Gnom", "Gnome", "Gnomen", NULL},

		/* Magieresistenz (0=Normal) */
		1.0,

		/* Maximale Aura (1=Durchschnitt) */
		0.0,

		/* Auraregeneration (1=Durchschnitt) */
		0.0,

		/* Rekrutierungskosten, Unterhalt pro Runde */
		5, 0,

		/* Splitsize */
		10000,

		/* Gewicht */
		1000,

		/* Multiplikator Geschwindigkeit */
		1.0,

		/* Trefferpunkte */
		50,

		/* Schaden AT_STANDARD unbewaffnet */
		"1d4",

		/* Natürliche Rüstung */
		0,

		/* Angriff, Verteidigung unbewaffnet */
		10, 10,

		/* Bonus Angriff, Verteidigung */
		10, 10,

		/* Angriffe */
		{
			{AT_STANDARD, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
			{AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0}, {AT_NONE, {NULL}, 0},
		},

		/* Talentboni
 		  Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
		  Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK */
		{
			0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
			0, 0, 0, 0,   1, 0, 0, 0,   0, 1, 0, 1,   1
		},

		/* Nonplayer (bei Gelegenheit entfernen) */
		true,

		/* Flags */
		(RCF_WALK|RCF_NOTEACH),

		/* Battle_flags */
		(0),

		/* Economic-Flags */
		CANGUARD,

		/* Vertraute für den Zauber
		   (Generisch, Illaun, Tybied, Cerddor, Gwyrrd, Draig) */
		{NORACE,NORACE,NORACE,NORACE,NORACE,NORACE}
	}

};

/* magres, {3 namen},
 * heimat, rekr.kost, unterhalt, splitsize, weight,
 * hitpoints, damage, armor, at_default, df_default, at_bonus, df_bonus
 * Alc,Arm,Ber,Bog, Bur,Han,Hol,Kat, Kräu,Mag,Pfer,Rei, Rüs,Sbau,Hie,Seg,
 * Sta,Spi,Ste,Str, Tak,Tar,Unt,Waf, Wag,Wahr,Steu,Aus, WlK
 * nonplayer,use_armor,
 * flags,
 * battle_flags,
 * generate_name
 * */

void
set_show_item(faction *f, item_t i)
{
	attrib *a = a_add(&f->attribs, a_new(&at_showitem));
	a->data.v = olditemtype[i];
}

void
give_starting_equipment(struct region *r, struct unit *u)

{
	switch(u->race) {
	case RC_DWARF:
		set_skill(u, SK_SWORD, 30);
		set_item(u, I_AXE, 1);
		set_item(u, I_CHAIN_MAIL, 1);
		break;
	case RC_ELF:
		set_item(u, I_FEENSTIEFEL, 1);
		set_show_item(u->faction, I_FEENSTIEFEL);
		break;
	case RC_ORC:
		set_skill(u, SK_SPEAR, 300);
		set_skill(u, SK_SWORD, 300);
		set_skill(u, SK_CROSSBOW, 300);
		set_skill(u, SK_LONGBOW, 300);
		set_skill(u, SK_CATAPULT, 300);
		break;
	case RC_GOBLIN:
		set_item(u, I_RING_OF_INVISIBILITY, 1);
		set_show_item(u->faction, I_RING_OF_INVISIBILITY);
		scale_number(u, 10);
		break;
	case RC_HUMAN:
		{
			building *b = new_building(&bt_castle, r, u->faction->locale);
			b->size = 2;
			u->building = b;
			fset(u, FL_OWNER);
		}
		break;
	case RC_TROLL:
		set_skill(u, SK_BUILDING, 30);
		set_skill(u, SK_OBSERVATION, 180);
		set_item(u, I_STONE, 10);
		break;
	case RC_DAEMON:
		set_skill(u, SK_AUSDAUER, 3600);
		u->hp = unit_max_hp(u);
		break;
	case RC_INSECT:
	  /* TODO: Potion-Beschreibung ausgeben */
		i_change(&u->items, oldpotiontype[P_WARMTH]->itype, 9);
		break;
	case RC_HALFLING:
		set_skill(u, SK_TRADE, 30);
		set_skill(u, SK_RIDING, 90);
		set_item(u, I_HORSE, 2);
		set_item(u, I_WAGON, 1);
		set_item(u, I_BALM, 5);
		set_item(u, I_SPICES, 5);
		set_item(u, I_JEWELERY, 5);
		set_item(u, I_MYRRH, 5);
		set_item(u, I_OIL, 5);
		set_item(u, I_SILK, 5);
		set_item(u, I_INCENSE, 5);
		break;
	case RC_CAT:
		set_item(u, I_RING_OF_INVISIBILITY, 1);
		set_show_item(u->faction, I_RING_OF_INVISIBILITY);
		break;
	case RC_AQUARIAN:
		{
			ship *sh = new_ship(&st_boat);
			sh->size = sh->type->construction->maxsize;
			addlist(&r->ships, sh);
			u->ship = sh;
			fset(u, FL_OWNER);
		}
		set_skill(u, SK_SAILING, 30);
		break;
	}

	set_money(u, 2000 + turn * 10);
}

void
give_latestart_bonus(region *r, unit *u, int b)
{
	change_skill(u, SK_OBSERVATION, b*30*u->number);
	change_money(u, 200*b);

	{
		unit *u2 = createunit(r, u->faction, 1, u->race);
		change_skill(u2, SK_TACTICS, ((b*30)/2) * u2->number);
		u2->irace = u->irace;
		fset(u2, FL_PARTEITARNUNG);
	}

	{
		unit *u2 = createunit(r, u->faction, 2*b, u->race);
		change_skill(u2, SK_SPEAR, 180 * u2->number);
		change_skill(u2, SK_TAXING, 180 * u2->number);
		change_item(u2, I_SPEAR, u2->number);
		u2->irace = u->irace;
		fset(u2, FL_PARTEITARNUNG);
	}
}

int
unit_max_hp(const unit * u)
{
	int h;
	double p;
	h = race[u->race].hitpoints;

	p = pow(effskill(u, SK_AUSDAUER) / 2.0, 1.5) * 0.2;
	h += (int) (h * p + 0.5);

	return h;
}

int
unit_old_max_hp(unit * u)
{
	int h;
	double p;
	h = race[u->race].hitpoints / 2;

	p = pow(effskill(u, SK_AUSDAUER) / 2.0, 1.5) * 0.2;
	h += (int) (h * p + 0.5);

	return h;
}

boolean is_undead(const unit *u)
{
	return u->race == RC_UNDEAD || u->race == RC_SKELETON
		|| u->race == RC_SKELETON_LORD || u->race == RC_ZOMBIE
		|| u->race == RC_ZOMBIE_LORD || u->race == RC_GHOUL
		|| u->race == RC_GHOUL_LORD;
}

extern void 
init_races(void)
{
#ifdef BETA_CODE
	a_add(&race[RC_TROLL].attribs, make_skillmod(NOSKILL, SMF_RIDING, NULL, 0.0, -1));
#endif
}
