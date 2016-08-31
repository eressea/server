local path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'
require 'eressea'
require 'eressea.xmlconf' -- read xml data

require 'config'
auto = require 'eressea.autoseed'

local function dump_selection(sel)
    local best = { score = 0, r = nil }
    local r, score
    for _, r in ipairs(sel) do
        score = p.score(r)
        if score > best.score then
            best.r = r
            best.score = score
        end
        print(score, r, r.terrain)
    end
    return best
end

print("read game")
eressea.read_game(get_turn() .. ".dat")
print("auto-seed")
auto.init()
print("editor")
gmtool.editor()
