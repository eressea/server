local sets = {
    ['fam_lynx'] = {
        ['skills'] = {
            ['espionage'] = 1,
            ['magic'] = 1,
            ['stealth'] = 1,
            ['perception'] = 1,
        }
    },
    ['fam_tunnelworm'] = {
        ['skills'] = {
            ['forestry'] = 1,
            ['magic'] = 1,
            ['mining'] = 1,
            ['stamina'] = 1,
        }
    },
    ['fam_eagle'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['perception'] = 1,
        }
    },
    ['fam_rat'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['espionage'] = 1,
            ['stealth'] = 1,
            ['perception'] = 1,
            ['stamina'] = 6,
        }
    },
    ['fam_owl'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['espionage'] = 1,
            ['stealth'] = 1,
            ['perception'] = 1,
        }
    },
    ['fam_direwolf'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['perception'] = 1,
        }
    },
    ['fam_hellcat'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['perception'] = 1,
        }
    },
    ['fam_tiger'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['perception'] = 1,
        }
    },
    ['fam_songdragon'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['flee'] = 2,
            ['sleep'] = 7,
            ['frighten'] = 8,
        },
    },
    ['fam_ghost'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['steal_aura'] = 6,
            ['summonundead'] = 6,
            ['frighten'] = 8,
        },
    },
    ['fam_fairy'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['appeasement'] = 1,
            ['calm_monster'] = 6,
            ['seduction'] = 6,
        },
    },
    ['fam_unicorn'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['stealth'] = 1,
            ['perception'] = 1,
        },
        ['spells'] = {
            ['resist_magic'] = 3,
            ['song_of_peace'] = 12,
            ['calm_monster'] = 6,
            ['heroic_song'] = 5,
            ['song_of_healing'] = 3,
            ['appeasement'] = 2,
        },
    },
    ['fam_imp'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['stealth'] = 1,
            ['perception'] = 1,
            ['espionage'] = 1,
            ['taxation'] = 1,
        },
        ['spells'] = {
            ['steal_aura'] = 6,
            ['shapeshift'] = 3,
            ['seduction'] = 6,
        },
    },
    ['fam_dreamcat'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['stealth'] = 1,
            ['perception'] = 1,
            ['espionage'] = 1,
            ['taxation'] = 1,
        },
        ['spells'] = {
            ['shapeshift'] = 3,
            ['transferauratraum'] = 3,
        },
    },
    ['fam_nymph'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['bow'] = 1,
            ['herbalism'] = 1,
            ['training'] = 1,
            ['riding'] = 1,
            ['espionage'] = 1,
            ['stealth'] = 1,
            ['entertainment'] = 1,
            ['perception'] = 1,
        },
        ['spells'] = {
            ['seduction'] = 6,
            ['calm_monster'] = 3,
            ['song_of_confusion'] = 4,
            ['appeasement'] = 1,
        },
    },
}

local equipment = require('eressea.equipment')
equipment.add_multiple(sets)
