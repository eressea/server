local install = os.getenv("ERESSEA_INSTALL") or config.install
if install then
    eressea.config.add_authority("conf", install .. '/conf')
    eressea.config.add_authority("res", install .. '/res')
else
    print("eressea.ini does not set an install directory")
end
