local modules = {}

function add_module(pkg)
    table.insert(modules, pkg)
end

local pkg = {}

function pkg.init()
    for k, v in ipairs(modules) do
        if v.init then v.init() end
    end
end

function pkg.update()
    for k, v in ipairs(modules) do
        if v.update then v.update() end
    end
end

return pkg
