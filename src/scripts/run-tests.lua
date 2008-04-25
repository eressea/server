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
io.stdin:read("*line")
