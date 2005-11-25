function get_wage(r, f, race)
  return 10
end


function get_maintenance(u)
  local f = u.region.owner
  if f ~= nil then
    if f == u.faction then
      return 2 * u.number
    else if f:get_policy(u.faction, "money") then
      return 5 * u.number
    end
  end
  return 10 * u.number
end

overload("maintenance", get_maintenance)
overload("wage", get_wage)
