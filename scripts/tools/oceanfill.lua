p = plane.get(0)
w, h = p:size()
print(p, w, h)
for x=0,w-1 do
    for y=0,h-1 do
        r = get_region(x,y)
        if r==nil then
            r = region.create(x, y, "ocean")
        end
    end
end
