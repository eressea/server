callbacks = {}

callbacks["attrib_init"] = function(attr)
    if attr.name ~= nil then
        local init = callbacks["init_" .. attr.name]
        if init ~=nil then
            init(attr)
        end
    end
end

