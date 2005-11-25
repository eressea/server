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

    hintText[0] = "Das Schiff des Elfen hat ein rotes Segel"
    hintText[1] = "Der Zwerg hat eine Nuss dabei"
    hintText[2] = "Die Katze führt eine Hellebarde"
    hintText[3] = "Das Schiff mit dem grünen Segel liegt links neben dem mit einem weissen Segel"
    hintText[4] = "Auf dem Schiff mit grünen Segeln kam der Speerkämpfer" 
    hintText[5] = "Der Krieger mit dem Kreis im Wappen hat einen Keks"
    hintText[6] = "Der Krieger des mittleren Schiffs hat ein Schwert"
    hintText[7] = "Auf dem gelben Segel prankt ein Kreuz als Wappen"
    hintText[8] = "Der Mensch kam mit dem ersten Schiff"
    hintText[9] = "Das Schiff mit dem Stern im Wappen liegt neben dem der einen Mandelkern hat"
    hintText[10] = "Das Schiff des Kriegers, der ein Apfel hat, liegt neben dem, der ein Kreuz als Wappen hat"
    hintText[11] = "Der Krieger mit dem Turm im Wappen trägt eine Axt"
    hintText[12] = "Das Schiff des Menschen liegt neben dem blauen Schiff"
    hintText[13] = "Das Insekt trägt einen Baum als Wappen"
    hintText[14] = "Das Schiff mit dem Stern im Wappen liegt neben dem des Kriegers, der einen Zweihänder führt"

    for i=0,4,1 do
      possibleHint[i] = tonumber(u.faction:get_variable("sphinxhint"..tostring(i)))
    end

    hint = math.random(0,4)

    usphinx:add_order("botschaft einheit " .. itoa36(u.id) .. " \"" .. hintText[possibleHint[hint]] .. "!\"")
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
    u.region:add_notice("Eine Botschaft von Sphinx (si7z): Fünf Krieger kamen auf ihren Schiffen in die Ebene der Helden, wollten um die Ehre ringen. Jedes Schiff trug ein anderes Segel, auf dem das Wappen des Kriegers prankte. Ein jeder trug seine liebste Waffe und das Signum seines Volkes und doch war keiner gleich.")
    u.region:add_notice("Eine Botschaft von Sphinx (si7z): Wer hat den Schneeball?")
  end
  
  local u = get_unit(atoi36("qcph"))
  if u ~= nil and u.faction == f then
    u.region:add_notice("Eine Botschaft von Sphinx (qcph): Mit einer Botschaft \"Hinweis\" gebe ich euch einen Hinweis, mit einer Botschaft \"Antwort <Eure Antwort>\" könnt ihr mir eure Antwort mitteilen. Doch Vorsicht, falsche Antworten werde ich bestrafen!")
    u.region:add_notice("Eine Botschaft von Sphinx (qcph): Fünf Krieger  kamen auf ihren Schiffen in die Ebene der Helden, wollten um die Ehre ringen. Jedes Schiff trug ein anderes Segel, auf dem das Wappen des Kriegers prankte. Ein jeder trug seine liebste Waffe und das Signum seines Volkes und doch war keiner gleich.")
  u.region:add_notice("Eine Botschaft von Sphinx (qcph): Wer hat den Schneeball?")
  end

  for faction in factions() do
    faction:delete_variable("sphinxGotHintsi7z");
    faction:delete_variable("sphinx2GotHintqcph");
  end

end
