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
