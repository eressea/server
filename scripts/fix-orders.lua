require 'config'
local fixme = {
"1",
"3uk8",
"6xv9",
"821n",
"8q",
"bsg3",
"cast",
"cere",
"chgg",
"evt",
"faLn",
"hsv9",
"j0L5",
"kLak",
"L12a",
"mdn",
"nxtz",
"p6it",
"trok",
"vb",
"yido"
}

function output(options)
    init_reports()
    for _, fno in ipairs(fixme) do
        f = get_faction(fno)
        if f then
            write_report(f, options)
        end
    end
end

function stage1()
    eressea.read_game('1267.dat')
    output(4)
end

function stage2()
    eressea.read_game('1268.dat')
    eressea.read_orders('noorders.txt')
    output(6)
    eressea.write_game('1268_fixed.dat')
end

function stage3()
    local file = io.open("sendreports.sh", "w")
    eressea.read_game('1268_fixed.dat')
    for _, id in ipairs(fixme) do
        f = get_faction(id)
        if f then
            file:write("#", tostring(f), '\n')
            file:write("zip " .. id .. ".zip 1268-" .. id .. ".*\n")
            file:write("cat mailbody | mutt " .. f.email .. " -s 'Eressea Ersatzvorlage' -a " .. id .. ".zip\n\n")
        end
    end
    file:close()
end

-- stage1()
-- stage2()
stage3()
