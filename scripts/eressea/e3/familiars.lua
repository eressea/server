eressea.log.debug("rules for game E3")

local equipment = require('eressea.equipment')
local sets = {
    ['fam_lynx'] = {
        ['skills'] = {
            ['magic'] = 1,
        }
    },
    ['fam_eagle'] = {
        ['skills'] = {
            ['magic'] = 1,
        }
    },
    ['fam_direwolf'] = {
        ['skills'] = {
            ['magic'] = 1,
        }
    },
    ['fam_owl'] = {
        ['skills'] = {
            ['magic'] = 1,
        }
    },
    ['fam_hellcat'] = {
        ['skills'] = {
            ['magic'] = 1,
        }
    },
    ['fam_tiger'] = {
        ['skills'] = {
            ['magic'] = 1,
        }
    },
    ['fam_rat'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['stamina'] = 6,
        }
    },
    ['fam_tunnelworm'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['mining'] = 1,
            ['forestry'] = 1,
            ['stamina'] = 1,
        }
    },
    ['fam_dreamcat'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['shapeshift'] = 3,
            ['transferauratraum'] = 3,
        }
    },
    ['fam_imp'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['shapeshift'] = 3,
            ['seduction'] = 6,
            ['steal_aura'] = 6,
        }
    },
    ['fam_ghost'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['steal_aura'] = 6,
            ['summonundead'] = 6,
            ['frighten'] = 8,
        }
    },
    ['fam_fairy'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['appeasement'] = 1,
            ['seduction'] = 6,
            ['calm_monster'] = 6,
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
        }
    },
    ['fam_unicorn'] = {
        ['skills'] = {
            ['magic'] = 1,
        },
        ['spells'] = {
            ['appeasement'] = 1,
            ['song_of_healing'] = 2,
            ['resist_magic'] = 3,
            ['heroic_song'] = 5,
            ['calm_monster'] = 6,
            ['song_of_peace'] = 12,
        }
    },
    ['fam_nymph'] = {
        ['skills'] = {
            ['magic'] = 1,
            ['bow'] = 1,
            ['training'] = 1,
            ['riding'] = 1,
        },
        ['spells'] = {
            ['appeasement'] = 1,
            ['song_of_confusion'] = 4,
            ['calm_monster'] = 6,
            ['seduction'] = 6,
        }
    },
}

equipment.add_multiple(sets)
