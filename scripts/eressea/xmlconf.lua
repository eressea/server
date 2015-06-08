local confdir = 'conf/'
if config.install then
    confdir = config.install .. '/' .. confdir
end
rules=''
if config.rules then
    rules = config.rules .. '/'
    read_xml(confdir .. rules .. 'config.xml', confdir .. rules .. 'catalog.xml')
    eressea.config.read(rules .. 'config.json', confdir)
end
