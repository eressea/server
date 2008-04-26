-- -*- coding: utf-8 -*-

function test_locales()
	local skills = { "", "herb", "kraut", "Kräute", "Kraeut", "k", "kra", "MAGIE" }
	for k,v in pairs(skills) do
		str = test.loc_skill("de", v)
		io.stdout:write(v, "\t", tostring(str), "  ", tostring(get_string("de", "skill::" .. tostring(str))), "\n")
	end
	return 0
end

test_locales()

local now = os.clock()
--read_game("566", "text")
--write_game("566.dat", "binary")
read_game("566.dat", "binary")
local elapsed = os.clock() - now
-- text: 51.203
-- binary: 20.859
print(elapsed)

io.stdin:read("*line")
