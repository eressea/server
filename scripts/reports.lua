dofile('config.lua')
eressea.read_game(get_turn() .. '.dat')
-- generate a new password and a message for new factions:
init_reports()
-- do not use write_reports, since it will change passwords
for f in factions() do
    write_report(f)
end
-- because passwords were changed, save the new state:
eressea.write_game(get_turn() .. '.dat')
