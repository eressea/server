function size()
  return 16
end

function island(pl, x, y, r)
  gmtool.make_block(pl, x, y, r)
  gmtool.make_island(pl, x+r/2+2, y+r/2, size() * 3)
  gmtool.make_island(pl, x-r-2, y+r/2, size() * 3)
  gmtool.make_island(pl, x-r/2-2, y-r/2, size() * 3)
  gmtool.make_island(pl, x+r+2, y-r/2, size() * 3)
  gmtool.make_island(pl, x+r/2+2, y-r-2, size() * 3)
  gmtool.make_island(pl, x-r/2-2, y+r+2, size() * 3)
end

function cross(pl, x, y, r)
  gmtool.make_block(pl, x-r, y+r*2, r)
  gmtool.make_block(pl, x+r*4/3, y, r)
  gmtool.make_block(pl, x-r*4/3, y, r)
  gmtool.make_block(pl, x+r, y-r*2, r)

  gmtool.make_island(pl, x, y, size() * 3)
  gmtool.make_island(pl, x, y-r*4/3, size() * 3)
  gmtool.make_island(pl, x, y+r*4/3, size() * 3)
  gmtool.make_island(pl, x+r*4/3, y-r*4/3, size() * 3)
  gmtool.make_island(pl, x-r*4/3, y+r*4/3, size() * 3)
end

function count()
  local i = 0
  for f in factions() do i = i + 1 end
  print(i)
end

function line(pl)
  local m = 0
  local x, y
  local i = 0
  x = 0
  y = i
  local r = get_region(x, y, pl)
  while true do
    if r==nil then
      if m==0 and (i>=0 or i<-10) then
        local s = size()
        gmtool.make_island(pl, x, y, s*3, s)
      else
        gmtool.make_block(pl, x, y, 6)
      end
      r = get_region(x, y, pl)
      if r==nil then
        r = region.create(x, y, "ocean", pl)
      end
      m = 1 - m
    end
    i = r.y + 1
    x = 0
    y = i
    r = get_region(x, y, pl)
    if r~=nil and r.y==0 then break end
  end
end

function build(pl)
  cross(pl, -28, -20, 6)
  island(pl, 28, -20, 11)
  island(pl, -28, 20, 11)
  island(pl, 28, 20, 11)
  -- line(pl)
end

eressea.free_game()
pl = plane.create(0, -50, -40, 90, 80)

build(pl)
