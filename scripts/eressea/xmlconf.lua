local confdir = 'conf/'
if config.install then
    confdir = config.install .. '/' .. confdir
    else
    confdir = '../' .. confdir
end
if config.rules then
    local rules = config.rules .. '/'
    assert(0 == eressea.config.read(rules .. 'config.json', confdir), "could not read JSON data")
    assert(0 == read_xml(confdir .. rules .. 'rules.xml', confdir .. rules .. 'catalog.xml'), "could not load XML data, did you compile with LIBXML2 ?")
    assert(0 == read_xml(confdir .. rules .. 'locales.xml', confdir .. rules .. 'catalog.xml'), "could not load XML data, did you compile with LIBXML2 ?")
end
eressea.game.reset()
