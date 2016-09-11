function size()
  return 16
end

function make_island(pl, x, y, a, b)
  if b==nil then b = a/3 end
  local nx, ny = plane.normalize(pl, x, y)
  gmtool.make_island(nx, ny, a, b)
end

function make_block(pl, x, y, r)
  local nx, ny = plane.normalize(pl, x, y)
  gmtool.make_block(nx, ny, r)
end

function find(email)
  for f in factions() do if f.email==email then return f end end
  return nil
end

function give_item(email, id, uname, iname)
  f = find(email)
  for u in f.units do
    u.id=atoi36(id)
    u.name=uname
    u:add_item(iname, 1)
    break
  end
end

function give_items()
  give_item("hongeldongel@web.de", "boss", "Drollitz", "rpg_item_1")
  give_item("zangerl.helmut@chello.at", "holb", "Holbard", "rpg_item_2")
  give_item("r.lang@chello.at", "brtL", "Bertl", "rpg_item_2")
  give_item("schlaustauch@gmx.de", "bert", "Bertram", "rpg_item_3")
end

function island(pl, x, y, r)
  make_block(pl, x, y, r)
  make_island(pl, x+r/2+2, y+r/2, size() * 3)
  make_island(pl, x-r-2, y+r/2, size() * 3)
  make_island(pl, x-r/2-2, y-r/2, size() * 3)
  make_island(pl, x+r+2, y-r/2, size() * 3)
  make_island(pl, x+r/2+2, y-r-2, size() * 3)
  make_island(pl, x-r/2-2, y+r+2, size() * 3)
end

function cross(pl, x, y, r)
  make_block(pl, x-r, y+r*2, r)
  make_block(pl, x+r*4/3, y, r)
  make_block(pl, x-r*4/3, y, r)
  make_block(pl, x+r, y-r*2, r)

  make_island(pl, x, y, size() * 3)
  make_island(pl, x, y-r*4/3, size() * 3)
  make_island(pl, x, y+r*4/3, size() * 3)
  make_island(pl, x+r*4/3, y-r*4/3, size() * 3)
  make_island(pl, x-r*4/3, y+r*4/3, size() * 3)
end

function clean()
  for r in regions() do
    if r.terrain=="ocean" then
      -- print(r)
      region.destroy(r)
    end
  end
end

function count()
  local i = 0
  for f in factions() do i = i + 1 end
  print(i)
end

function line(pl)
  local m = 0
  local i = 0
  local x, y = plane.normalize(pl, 0, i)
  local r = get_region(x, y)
  while true do
    if r==nil then
      if m==0 and (i>=0 or i<-10) then
        local s = size()
        gmtool.make_island(x, y, s*3, s)
      else
        gmtool.make_block(x, y, 6)
      end
      r = get_region(x, y)
      if r==nil then
        r = region.create(x, y, "ocean")
      end
      m = 1 - m
    end
    i = r.y + 1
    x, y = plane.normalize(pl, 0, i)
    r = get_region(x, y)
    if r~=nil and r.y==0 then break end
  end
end

function build(pl)
  local d = 28
  local h = 20
  line(pl)
  island(pl, d+15, -6, 11)
  island(pl, -d, -h-10, 11)
  cross(pl, -d, h-10, 6)
  island(pl, d, 2*h, 11)
end

function fill(pl, w, h)
  local x, y
  for x=0,w do
    for y=0,h do
      local nx, ny = plane.normalize(pl, x, y)
      local r = get_region(nx, ny)
      if r==nil then
        r = region.create(nx, ny, "ocean")
      end
    end
  end
end

function seed()
  local input = io.open(config.basepath .. "/parteien.txt")
  if input then
      for f in factions() do
        if f.race=="vampunicorn" then
          local str = input:read("*line")
          if str==nil then break end
          local race, lang, email = str:match("([^ ]*) ([^ ]*) ([^ ]*)")
          f.race = race:lower()
          f.options = f.options + 4096
          f.email = email
          f.locale = lang
          for u in f.units do
            u.race = race:lower()
            u.hp = u.hp_max
            local b = building.create(u.region, "castle")
            if lang=="de" then
              u.name = "Entdecker"
              b.name = "Heimat"
            else
              u.name = "Explorer"
              b.name = "Home"
            end
            b.size = 10
            u.building = b
          end
        end
      end
      input:close()
  end
  for r in regions() do
    r:set_resource("sapling", r:get_resource("tree")/4)
    r:set_resource("seed", 0)
  end
  update_owners()
end

function select()
  for f in factions() do
    if f.email=="enno@eressea.de" then
      for u in f.units do
        gmtool.select(u.region, true)
        u.number = 0
      end
    end
  end
end

function justWords(str)
  local t = {}
  local function helper(word) table.insert(t, word) return "" end
  if not str:gsub("%w+", helper):find"%S" then return t end
end

function rebuild()
  free_game()
  local w = 110
  local h = 80
  local pl = plane.create(0, -w/2, -h/2, w+1, h+1)
  build(pl)
  fill(pl, w, h)
  write_map("export.cr")
end

function testwelt()
  free_game()
  local w = 10
  local h = 10
  local pl = plane.create(0, -w/2, -h/2, w+1, h+1)
  gmtool.make_island(0, 0, 30, 3)
  fill(pl, w, h)
  write_map("export.cr")
end
