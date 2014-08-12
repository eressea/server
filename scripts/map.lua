require 'eressea'
require 'eressea.xmlconf'
eressea.read_game(get_turn() .. ".dat")
gmtool.editor()
