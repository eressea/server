local path = os.getenv("ERESSEA_DIR")
if not path then
    if config.install then
	    path = config.install
    else
        path = '.'
    end
else
    config.install = path
end
path = path .. "/scripts"
package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path
if config.paths ~= nil then
    for path in string.gmatch(config.paths, "([^:]+)") do
        package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'
    end
end
