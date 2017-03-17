require "lunit"

module("tests.e3.morale", package.seeall, lunit.testcase )

function setup()
    eressea.game.reset()
    eressea.settings.set("rules.food.flags", "4") -- food is free
end

function teardown()
    eressea.settings.set("rules.food.flags", "0")
end

function test_when_owner_returns_morale_stays()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("human", "owner_returns@eressea.de", "de")
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
  process_orders()
  assert_equal(5, r.morale) -- old owner returns, no reduction
  assert_false(r.is_mourning)
end

function test_morale_alliance()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("human", "ma1@eressea.de", "de")
  local u1 = unit.create(f1, r, 1)
  u1:add_item("money", 10000)
  local f2 = faction.create("human", "ma2@eressea.de", "de")
  local u2 = unit.create(f2, r, 1)
  u2:add_item("money", 10000)
  local f3 = faction.create("human", "ma3@eressea.de", "de")
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
  assert_false(r.is_mourning)

  
  -- change owner, new owner is in the same alliance
  u1.building = nil
  run_a_turn()
  assert_equal(4, r.morale)
  assert_true(r.is_mourning)

  run_a_turn()
  assert_false(r.is_mourning) -- mourning recovers

  
  -- change owner, new owner is not in the same alliance
  u2.building = nil
  run_a_turn()
  assert_equal(0, r.morale)
  assert_true(r.is_mourning)
  run_a_turn()
  assert_false(r.is_mourning) -- mourning recovers
end

function test_bigger_castle_empty()
    local r = region.create(0, 0, "plain")
    assert_equal(1, r.morale)
    local f1 = faction.create("human", "small1@eressea.de", "de")
    local u1 = unit.create(f1, r, 1)
    local f2 = faction.create("human", "small2@eressea.de", "de")
    local u2 = unit.create(f2, r, 1)
    u1:add_item("money", 10000)
    
    local big = building.create(r, "castle")
    big.size = 20
    u1.building = big

    local small = building.create(r, "castle")
    small.size = 10
    u2.building = small

    local function run_a_turn()
        process_orders()
        f1.lastturn=get_turn()
    end

    update_owners()
    assert_equal(r.owner, u1.faction)
    u1.building = nil
    update_owners()
    assert_equal(r.owner, u2.faction)
    assert_equal(0, r.morale)
    assert_true(r.is_mourning)

    run_a_turn()
    assert_false(r.is_mourning) -- mourning recovers
end

function test_morale_change()
    local r = region.create(0, 0, "plain")
    assert_equal(1, r.morale)
    local f1 = faction.create("human", "mchange@eressea.de", "de")
    local u1 = unit.create(f1, r, 1)
    u1:add_item("money", 10000)
    
    local AVG_STEP = 6
    local b = building.create(r, "castle")
    b.size = 10
    u1.building = b

    local function run_a_turn()
        process_orders()
        f1.lastturn=get_turn()
    end
  
    -- reinhardt-regel: nach 2*AVG_STEP ist moral mindestens einmal gestiegen.
    update_owners()
    assert_not_equal(r.owner, nil)
    assert_false(r.is_mourning)
    for i=1,AVG_STEP*2 do
        run_a_turn()
        assert_not_equal(r.owner, nil)
    end
    assert_not_equal(1, r.morale)
    assert_false(r.is_mourning)

    -- regel: moral ist nie hoeher als 2 punkte ueber burgen-max.
    for i=1,AVG_STEP*4 do
        run_a_turn()
    end
    assert_equal(4, r.morale)
    assert_false(r.is_mourning)
  
    -- auch mit herrscher faellt moral um 1 pro woche, wenn moral > burgstufe
    r.morale = 6
    run_a_turn()
    assert_equal(5, r.morale)
    assert_false(r.is_mourning)
    run_a_turn()
    assert_equal(4, r.morale)
    run_a_turn()
    assert_equal(4, r.morale)
    
    -- regel: ohne herrscher fällt die moral jede woche um 1 punkt, bis sie 1 erreicht
    assert_false(r.is_mourning)
    u1.building = nil
    update_owners()
    assert_false(r.is_mourning)
    run_a_turn()
    assert_equal(3, r.morale)
    assert_false(r.is_mourning)
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

function test_morale_give_command()
  local r = region.create(0, 0, "plain")
  assert_equal(1, r.morale)
  local f1 = faction.create("human", "mold1@eressea.de", "de")
  local u1 = unit.create(f1, r, 1)
  local f2 = faction.create("human", "mold2@eressea.de", "de")
  local u2 = unit.create(f2, r, 1)

  local b = building.create(r, "castle")
  b.size = 10
  u1.building = b
  u2.building = b
  update_owners()
  assert_equal(1, r.morale)
  assert_false(r.is_mourning)
  r.morale = 5
  assert_equal(r.owner, u1.faction)
  u1:clear_orders()
  u1:add_order("GIB " .. itoa36(u2.id) .. " KOMMANDO")

  process_orders()
  assert_equal(u2.faction, r.owner)
  assert_equal(3, r.morale) --  5 - MORALE_TRANSFER
  assert_true(r.is_mourning) 

  u1:clear_orders()

  process_orders()
  assert_false(r.is_mourning) -- mourning recovers
end

