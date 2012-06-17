require "lunit"

module("tests.e3.morale", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
end

function test_when_owner_returns_morale_drops_only_2()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  u1:add_item("money", 10000)
  local b = building.create(r, "castle")
  b.size = 50
  
  set_turn(get_turn()+10)
  f1.lastturn=get_turn()
  u1.building = b
  update_owners()
  r.morale = 6
  u1.building = nil
  process_orders()
  assert_equal(5, r.morale) -- no owner, fall by 1
  u1.building = b
  update_owners()
  set_key("test", 42)
  process_orders()
  assert_equal(3, r.morale) -- new owner, fall by 2
end

function test_morale_alliance()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("noreply@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  u1:add_item("money", 10000)
  local f2 = faction.create("noreply@eressea.de", "human", "de")
  local u2 = unit.create(f2, r, 1)
  u2:add_item("money", 10000)
  local f3 = faction.create("noreply@eressea.de", "human", "de")
  local u3 = unit.create(f3, r, 1)
  u3:add_item("money", 10000)

  local al = alliance.create(42, "Die Antwoord")
  f1.alliance = al;
  f2.alliance = al;

  local b = building.create(r, "castle")
  b.size = 50
  u1.building = b
  u2.building = b
  u3.building = b
  update_owners()
  r.morale = 6

  local function run_a_turn()
    process_orders()
    f1.lastturn=get_turn()
    f2.lastturn=get_turn()
    f3.lastturn=get_turn()
  end
  
  -- just checking everything's okay after setup.
  run_a_turn()
  assert_equal(6, r.morale)
  
  -- change owner, new owner is in the same alliance
  u1.building = nil
  run_a_turn()
  assert_equal(4, r.morale)
  
  -- change owner, new owner is not in the same alliance
  u2.building = nil
  run_a_turn()
  assert_equal(0, r.morale)
end

function test_morale_change()
    local r = region.create(0, 0, "plain")
    assert_equal(1, r.morale)
    local f1 = faction.create("noreply@eressea.de", "human", "de")
    local u1 = unit.create(f1, r, 1)
    u1:add_item("money", 10000)
    local f2 = faction.create("noreply@eressea.de", "human", "de")
    local u2 = unit.create(f2, r, 1)
    u2:add_item("money", 10000)
    
    local AVG_STEP = 6
    local b = building.create(r, "castle")
    b.size = 10
    u1.building = b

    local function run_a_turn()
        process_orders()
        f1.lastturn=get_turn()
        f2.lastturn=get_turn()
    end
  
    -- reinhardt-regel: nach 2*AVG_STEP ist moral mindestens einmal gestiegen.
    update_owners()
    assert_not_equal(r.owner, nil)
    for i=1,AVG_STEP*2 do
        run_a_turn()
        assert_not_equal(r.owner, nil)
    end
    assert_not_equal(1, r.morale)

    -- regel: moral ist nie hoeher als 2 punkte ueber burgen-max.
    for i=1,AVG_STEP*4 do
        run_a_turn()
    end
    assert_equal(4, r.morale)
  
    -- auch mit herrscher faellt moral um 1 pro woche, wenn moral > burgstufe
    r.morale = 6
    run_a_turn()
    assert_equal(5, r.morale)
    run_a_turn()
    assert_equal(4, r.morale)
    run_a_turn()
    assert_equal(4, r.morale)
    
    -- regel: ohne herrscher fällt die moral jede woche um 1 punkt, bis sie 1 erreicht
    u1.building = nil
    update_owners()
    run_a_turn()
    assert_equal(3, r.morale)
    run_a_turn()
    assert_equal(2, r.morale)
    run_a_turn()
    assert_equal(1, r.morale)
    run_a_turn()
    assert_equal(1, r.morale)
    
    -- ohne herrscher ändert sich auch beschissene Moral nicht:
    r.morale = 0
    run_a_turn()
    assert_equal(0, r.morale)
end

function test_morale_old()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("first@eressea.de", "human", "de")
  local u1 = unit.create(f1, r, 1)
  local f2 = faction.create("second@eressea.de", "human", "de")
  local u2 = unit.create(f2, r, 1)

  local b = building.create(r, "castle")
  b.size = 10
  u1.building = b
  u2.building = b
  update_owners()
  assert_equal(1, r.morale)
  r.morale = 5
  assert_equal(r.owner, u1.faction)
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " KOMMANDO")
  process_orders()
  u1:clear_orders()
  assert_equal(u2.faction, r.owner)
  assert_equal(3, r.morale) --  5-MORALE_TRANSFER
  for u in r.units do
    if u.faction.id==u2.faction.id then
      u.building = nil
    end
  end
  update_owners()
  assert_equal(r.owner, u1.faction)
  assert_equal(0, r.morale)
end

function test_no_uruk()
  local f1 = faction.create("noreply@eressea.de", "uruk", "de")
  assert_equal(f1.race, "orc")
end
