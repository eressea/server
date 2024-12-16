local function mallorn_region(r)
    return r:get_flag(1) -- RF_MALLORN
end

function seed_limit(r)
    if mallorn_region(r) then
        return 0
    end
    return r:get_resource("seed")
end

function seed_produce(r, n)
    if not mallorn_region(r) then
        local seeds = r:get_resource("seed")
        if seeds>=n then
            r:set_resource("seed", seeds-n)
        else
            r:set_resource("seed", 0)
        end
    end
end

function mallornseed_limit(r)
    if mallorn_region(r) then
        return r:get_resource("seed")
    end
    return 0
end

function mallornseed_produce(r, n)
    if mallorn_region(r) then
        local seeds = r:get_resource("seed")
        if seeds>=n then
            r:set_resource("seed", seeds-n)
        else
            r:set_resource("seed", 0)
        end
    end
end
function horse_limit(r)
    return r:get_resource("horse")
end

function horse_produce(r, n)
  local horses = r:get_resource("horse")
  if horses>=n then
    r:set_resource("horse", horses-n)
  else
    r:set_resource("horse", 0)
  end
end

function log_limit(r)
  return r:get_resource("tree") + r:get_resource("sapling")
end

function log_produce(r, n)
  local trees = r:get_resource("tree")
  if trees>=n then
    r:set_resource("tree", trees-n)
  else
    r:set_resource("tree", 0)
    n = n - trees
    trees = r:get_resource("sapling")
    if trees>=n then
      r:set_resource("sapling", trees-n)
    else
      r:set_resource("sapling", 0)
    end
  end
end

function mallorn_limit(r)
  if not mallorn_region(r) then
    return 0
  end
  return r:get_resource("tree") + r:get_resource("sapling")
end

function mallorn_produce(r, n)
  local trees = r:get_resource("tree")
  if trees>=n then
    r:set_resource("tree", trees-n)
  else
    r:set_resource("tree", 0)
    n = n - trees
    trees = r:get_resource("sapling")
    if trees>=n then
      r:set_resource("sapling", trees-n)
    else
      r:set_resource("sapling", 0)
    end
  end
end
