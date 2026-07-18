local install = config.install
if install then
    eressea.config.add_authority("conf", install .. '/conf')
    eressea.config.add_authority("res", install .. '/res')
else
    print("no install directory set")
end
