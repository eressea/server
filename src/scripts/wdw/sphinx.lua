--
-- www.websudoku.com
-- Evil Puzzle 9,558,581,844
--

function init_sphinxhints()

  for f in factions() do
    hints = {}

    for i=0,14,1 do
      hints[i] = 0
    end
    for i=0,4,1 do
      if f:get_variable("sphinxhint"..tostring(i)) == nil then
        repeat
          hint = math.random(0,14)
        until hints[hint] == 0
        hints[hint] = 1
        f:set_variable("sphinxhint"..tostring(i),tostring(hint))
      end
    end
  end
end

function sphinx_handler()

  local function send_gotHint(u, usphinx)
    usphinx:add_order("botschaft einheit " .. itoa36(u.id) .. " \"Du hast diese Woche bereits einen Hinweis erhalten!\"")
  end
  
  local function send_hint(u, usphinx)
    hintText = {}
    possibleHint = {}

    hintText[0] = "Eins! Neun! Vier"
    hintText[1] = "Sechs! Eins! Zwei"
    hintText[2] = "Vier! Neun! Sieben"
    hintText[3] = "Sieben! Eins! Neun"
    hintText[4] = "Nenne mir die Summe der beiden Diagonalen"
    hintText[5] = "Fünf! Fünf! Sechs"
    hintText[6] = "Sieben! Zwei! Fünf"
    hintText[7] = "Sechs! Zwei! Sieben"
    hintText[8] = "Zwei! Zwei! Zwei"
    hintText[9] = "Acht! Sechs! Eins"
    hintText[10] = "Fünf! Acht! Vier"
    hintText[11] = "Acht! Sieben! Drei"
    hintText[12] = "Sechs! Fünf! Vier"
    hintText[13] = "Eins! Zwei! Drei"
    hintText[14] = "Vier! Fünf! Neun"
    hintText[15] = "Acht! Acht! Neun"
    hintText[16] = "Zwei! Drei! Eins" 
    hintText[17] = "Fünf! Zwei! Neun"
    hintText[18] = "Zwei! Vier! Sieben" 
    hintText[19] = "Neun! Eins! Eins"
    hintText[20] = "Eins! Vier! Fünf"
    hintText[21] = "Neun! Sechs! Acht"
    hintText[22] = "Drei! Neun! Neun"
    hintText[23] = "Neun! Acht! Fünf"
    hintText[24] = "Vier! Acht! Sechs"
    hintText[25] = "Drei! Acht! Drei"

    numhints = 26
    turn = get_turn()
    -- hint = turn%26
    hint = turn - (math.floor(turn/numhints)*numhints)

    print("sphinx turn=" .. turn .. ", hint=" .. hint .. "\n")

    usphinx:add_order("botschaft einheit " .. itoa36(u.id) .. " \"" .. hintText[hint] .. "!\"")
  end

  local function msg_handler(u, evt)
    str = evt:get_string(0)
    u2 = evt:get_unit(1)
    if string.lower(str) == "hinweis" then
      if u2.faction:get_variable("sphinxGotHint"..itoa36(u.id)) ~= nil then
        send_gotHint(u2)
      else
        send_hint(u2, u)
      end
    end  
    tokens = {}
    for token in string.gfind(str, "%a+") do
      table.insert(tokens, token)
    end
    -- index starts with 1 in lua
    if table.getn(tokens) == 2 and string.lower(tokens[1]) == "antwort" then
      if string.lower(tokens[2]) == "999999" then
        -- Botschaft in alle Regionen
        local m = message("msg_event")
        m:set_string("string", "Das Rätsel der Sphinx ist gelöst! Die Sphinx wird sich eine neue Heimat und ein neues Rätsel suchen.")
        for r in regions() do
          m:send_region(r)
        end
        -- Region terraformen
        terraform(u2.region.x, u.region.y, "plain")
        u2.region.set_resource(u2.region, "tree", 721)
        u2.region.set_resource(u2.region, "peasant", 2312)
        u2.add_item(u2, "trappedairelemental", 5)
        -- Neues Raetsel fuer beide Sphinxe!
        -- Sphinx neu platzieren
        -- Hint-Attribute von allen Allianzen loeschen
      else

      end
    end
  end
  
  local f = get_faction(atoi36("ycx9"))
  local u = get_unit(atoi36("si7z"))
  if u ~= nil and u.faction == f then
    u:add_handler("message", msg_handler) 
  end
  
  local f = get_faction(atoi36("ycx9"))
  local u = get_unit(atoi36("qcph"))
  if u ~= nil and u.faction == f then
    u:add_handler("message", msg_handler) 
  end
end

function sphinx_weekly()
  local f = get_faction(atoi36("ycx9"))
  local u = get_unit(atoi36("si7z"))
  if u ~= nil and u.faction == f then
    u.region:add_notice("Eine Botschaft von Sphinx (si7z): Mit einer Botschaft \"Hinweis\" gebe ich euch einen Hinweis, mit einer Botschaft \"Antwort <Eure Antwort>\" könnt ihr mir eure Antwort mitteilen. Doch Vorsicht, falsche Antworten werde ich bestrafen!")
  end
  
  local u = get_unit(atoi36("qcph"))
  if u ~= nil and u.faction == f then
    u.region:add_notice("Eine Botschaft von Sphinx (qcph): Mit einer Botschaft \"Hinweis\" gebe ich euch einen Hinweis, mit einer Botschaft \"Antwort <Eure Antwort>\" könnt ihr mir eure Antwort mitteilen. Doch Vorsicht, falsche Antworten werde ich bestrafen!")
  end

  for faction in factions() do
    faction:delete_variable("sphinxGotHintsi7z");
    faction:delete_variable("sphinx2GotHintqcph");
  end

end
