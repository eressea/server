swapx = 0
swapy = 0

function swap_region(r, tr)
  local sr = get_region(swapx, swapy)
  while sr~=nil do
     swapx = math.random(1000)
     swapy = math.random(1000)
     sr = get_region(swapx, swapy)
   end
   local tx = tr.x
   local ty = tr.y
   local x = r.x
   local y = r.y
   tr:move(swapx, swapy)
   r:move(tx, ty)
   tr:move(x, y)
end

function move_selection(x, y)
   for r in gmtool.get_selection() do
     local tx = r.x+x
     local ty = r.y+y
     local tr = get_region(tx, ty)
     if tr~=nil then
       swap_region(r, tr)
     else
       r:move(tx, ty)
     end
   end
end

