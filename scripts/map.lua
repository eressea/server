require 'eressea.path'
require 'eressea'
require 'eressea.xmlconf'

function gmtool_on_keypressed(keycode)
    local warp = require('eressea.warpgate')
    if keycode == gmtool.KEY_F1 then
        warp.gm_edit()
    end
end

eressea.read_game(get_turn() .. ".dat")
gmtool.editor()
