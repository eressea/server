project.name = "eressea"
project.configs = { "Debug", "Profile", "Release" }

package = newpackage()
package.name = "util"
package.kind = "lib"
package.language = "c"
package.path = "common"
package.includepaths = { "..", ".", "util" }
package.files = matchfiles("../config.h", "settings.h", "util/*.c", "util/*.h", "iniparser/*.c", "iniparser/*.h")
package.excludes = { "util/strncpy.c" }
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  if (windows) then
    table.insert(package.config[v].defines, "WIN32")
    table.insert(package.config[v].defines, "_CRT_SECURE_NO_DEPRECATE")
  end
  if v=="Release" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end


package = newpackage()
package.name = "kernel"
package.kind = "lib"
package.language = "c"
package.path = "common"
package.includepaths = { "..", ".", "kernel", "util" }
package.files = matchfiles("kernel/*.c", "triggers/*.c", "spells/*.c", "items/*.c", "kernel/*.h", "triggers/*.h", "spells/*.h", "items/*.h", "modules/*.h", "modules/*.c", "attributes/*.h", "attributes/*.c")
package.excludes = { 
  "modules/victoryconditions.c",
  "modules/victoryconditions.h",
  "items/studypotion.c",
  "items/studypotion.h"
}
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  if (windows) then
    table.insert(package.config[v].defines, "WIN32")
    table.insert(package.config[v].defines, "_CRT_SECURE_NO_DEPRECATE")
  end
  if v=="Release" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end


package = newpackage()
package.name = "gamecode"
package.kind = "lib"
package.language = "c"
package.path = "common"
package.includepaths = { "..", ".", "kernel", "util" }
package.files = matchfiles("gamecode/*.c", "races/*.c", "gamecode/*.h", "races/*.h")
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  if (windows) then
    table.insert(package.config[v].defines, "WIN32")
    table.insert(package.config[v].defines, "_CRT_SECURE_NO_DEPRECATE")
  end
  if v=="Release" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end


package = newpackage()
package.name = "bindings"
package.kind = "lib"
package.language = "c++"
package.path = "eressea/lua"
package.includepaths = { "../..", "../../common", "../../common/kernel" }
package.files = matchfiles("*.cpp", "*.h")
package.excludes = { "gm.cpp" }
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  if (windows) then
    table.insert(package.config[v].defines, "WIN32")
    table.insert(package.config[v].defines, "_CRT_SECURE_NO_DEPRECATE")
  end
  if v=="Release" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end

package = newpackage()
package.name = "gmtool"
package.kind = "exe"
package.language = "c++"
package.path = "eressea"
package.includepaths = { "..", "../common", "../common/kernel" }
package.files = { "gmtool.h", "gmtool.c", "editing.c", "editing.h", "gmmain.cpp", "console.c", "curses/listbox.h", "curses/listbox.c", "lua/gm.cpp" }
package.links = { "util", "kernel", "bindings", "libxml2", "curses" }
for k,v in project.configs do
  package.config[v].objdir   = v
  package.config[v].bindir   = v
  table.insert(package.config[v].buildflags, "extra-warnings")
  if (windows) then
    table.insert(package.config[v].defines, "WIN32")
    table.insert(package.config[v].defines, "_CRT_SECURE_NO_DEPRECATE")
  end
  if v=="Debug" then
    package.config[v].links = { "lua5.1_d", "luabind_d" }
  else
    package.config[v].links = { "lua5.1", "luabind" }
    if v=="Release" then
      table.insert(package.config[v].defines, "NDEBUG")
    end
  end
end


package = newpackage()
package.name = "eressea-lua"
package.kind = "exe"
package.language = "c++"
package.path = "eressea"
package.includepaths = { "..", "../common", "../common/kernel" }
package.files = { "korrektur.c", "server.cpp", "console.c", "console.h" }
package.links = { "util", "kernel", "gamecode", "bindings", "libxml2" }
for k,v in project.configs do
  package.config[v].objdir   = v
  package.config[v].bindir   = v
  table.insert(package.config[v].buildflags, "extra-warnings")
  if (windows) then
    table.insert(package.config[v].defines, "WIN32")
    table.insert(package.config[v].defines, "_CRT_SECURE_NO_DEPRECATE")
  end
  if v=="Debug" then
    package.config[v].links = { "lua5.1_d", "luabind_d" }
  else
    package.config[v].links = { "lua5.1", "luabind" }
    if v=="Release" then
      table.insert(package.config[v].defines, "NDEBUG")
    end
  end
--  package.config["Debug"].linkoptions = { "/NODEFAULTLIB:MSVCRT /NODEFAULTLIB:LIBCMT" }
end

-- kernel: util, triggers, attributes, spells, items
-- gamecode: races
