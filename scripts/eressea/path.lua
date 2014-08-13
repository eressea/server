if config.paths ~= nil then
    for path in string.gmatch(config.paths, "([^:]+)") do
        package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'
    end
end
-- print(package.path)
