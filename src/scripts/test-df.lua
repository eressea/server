-- start eressea-server with -s file=<filename>
-- where <filename> is relative to datapath()

if read_game(file)~=0 then
  print("could not read game")
end

