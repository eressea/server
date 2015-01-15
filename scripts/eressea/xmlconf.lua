local confdir = 'conf/'
if config.install then
    confdir = config.install .. '/' .. confdir
    else
    confdir = '../' .. confdir
end
rules=''
if config.rules then
    rules = config.rules .. '/'
end
read_xml(confdir .. rules .. 'config.xml', confdir .. rules .. 'catalog.xml')
eressea.config.read(rules .. 'config.json', confdir)
