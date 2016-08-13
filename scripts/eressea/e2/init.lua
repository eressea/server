require 'eressea.spells'
eressea.log.debug("rules for game E2")

return {
    require('eressea'),
--    require('eressea.autoseed'),
    require('eressea.xmas'),
    require('eressea.xmasitems'),
    require('eressea.wedding'),
    require('eressea.embassy'),
    require('eressea.tunnels'),
    require('eressea.ponnuki'),
    require('eressea.astral'),
    require('eressea.locales'),
    require('eressea.jsreport'),
    require('eressea.ents'),
    require('eressea.cursed')
}
