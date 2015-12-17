local function get_markets(r, result)
  local n = 0
  result = result or {}

  for b in r.buildings do
    if b.type == "market" and b.working and b.size == b.maxsize then
      u = b.owner
      if u~=nil then
        table.insert(result, u)
        n = n + 1
      end
    end
  end
  return n, result
end

local function collect_markets(r, result)
  local result = result or {}
  local n = 0
  n, result = get_markets(r, result)
  for i, r in ipairs(r.adj) do
    if r then
      local x, result = get_markets(r, result)
      n = n + x
    end
  end
  return n, result
end

local function market_action(r)
  local f = r.owner
  local trade = 1000
  if f~=nil and f.race=="halfling" then
    trade = 600
  end
  
  local p = r:get_resource("peasant")
  if p > 500 then
    local n, markets = collect_markets(r)

    if n>0 then
      local give
      if r.luxury~=nil then
        give = {}
        local numlux = p / trade
        for x = 1, numlux do
          local m = 1+math.fmod(rng_int(), n)
          u = markets[m]
          if give[u] then
            give[u] = give[u] + 1
          else
            give[u] = 1
          end
        end

        for u, v in pairs(give) do
          u:add_item(r.luxury, v)
        end
      end
      
      if r.herb~=nil then
        give = {}
        local numherb = p / 500
        for x = 1, numherb do
          local m = 1+math.fmod(rng_int(), n)
          u = markets[m]
          if give[u] then
            give[u] = give[u] + 1
          else
            give[u] = 1
          end
        end

        for u, v in pairs(give) do
          u:add_item(r.herb, v)
        end
      end
    end
  end
end

local markets = {}

function markets.update()
  local r
  for r in regions() do
    market_action(r)
  end
end

return markets
