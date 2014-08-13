local confdir = 'conf/'
if config.rules then
    confdir = confdir .. config.rules .. '/'
end
if config.install then
    confdir = config.install .. '/' .. confdir
end
read_xml(confdir .. 'config.xml', confdir .. 'catalog.xml')
