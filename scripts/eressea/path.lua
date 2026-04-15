root = os.getenv("ERESSEA_ROOT")
if root then
    local path = root .. "/scripts"
    package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path
end

if config.install then
	local path = config.install .. "/scripts"
    package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path
    eressea.config.add_authority("conf", config.install .. '/conf')
    eressea.config.add_authority("res", config.install .. '/res')
end
