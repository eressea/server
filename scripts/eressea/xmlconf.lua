local confdir = 'conf/'
if config.install then
    confdir = config.install .. '/' .. confdir
end
rules=''
if config.rules then
    rules = config.rules .. '/'
end
assert(0 == read_xml(confdir .. rules .. 'config.xml', confdir .. rules .. 'catalog.xml'), "could not load XML data, did you compile with LIBXML2 ?")
assert(0 == eressea.config.read(rules .. 'config.json', confdir), "could not read JSON data")
