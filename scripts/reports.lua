dofile('config.lua')
eressea.read_game(get_turn() .. '.dat')
init_reports()
-- do not use write_reports, since it will change passwords
for f in factions() do
    write_report(f)
end
