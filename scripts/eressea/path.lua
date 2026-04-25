local install = os.getenv("ERESSEA_INSTALL") or config.install
if install then
	local path = install .. "/scripts"
    package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path
    eressea.config.add_authority("conf", install .. '/conf')
    eressea.config.add_authority("res", install .. '/res')
else
    print("eressea.ini does not set an install directory")
end
