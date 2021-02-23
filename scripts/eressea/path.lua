local path = '.'
if config.install then
	path = config.install
else
    path = os.getenv("ERESSEA_DIR") or path
    config.install = path
end
path = path .. "/scripts"
package.path = path .. '/?.lua;' .. path .. '/?/init.lua;' .. package.path
