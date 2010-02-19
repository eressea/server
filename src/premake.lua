project.name = "eressea"
project.configs = { "Debug", "Profile", "Release" }

-- the name of the curses library on this system:
c_defines = { }
lib_curses = "curses"
if (windows) then
  lib_curses = "pdcurses"
  c_defines = { "WIN32", "_CRT_SECURE_NO_DEPRECATE" }
end

package = newpackage()
package.name = "util"
package.kind = "lib"
package.language = "c"
package.path = "common"
package.includepaths = { "..", "." }
package.files = matchfiles("../config.h", "settings.h", "util/*.c", "util/*.h", "iniparser/*.c", "iniparser/*.h")
package.excludes = { "util/strncpy.c" }
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  table.insert(package.config[v].buildflags, "no-debug-runtime")
  table.insert(package.config[v].buildflags, "singlethread-runtime")
  for index, name in c_defines do
    table.insert(package.config[v].defines, name)
  end
  if v~="Debug" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end


package = newpackage()
package.name = "kernel"
package.kind = "lib"
package.language = "c"
package.path = "common"
package.includepaths = { "..", "." }
package.files = matchfiles("kernel/*.c", "triggers/*.c", "spells/*.c", "items/*.c", "kernel/*.h", "triggers/*.h", "spells/*.h", "items/*.h", "modules/*.h", "modules/*.c", "attributes/*.h", "attributes/*.c")
package.excludes = { 
  "modules/victoryconditions.c",
  "modules/victoryconditions.h",
  "items/studypotion.c",
  "items/studypotion.h",
  "kernel/sqlstore.c",
  "kernel/sqlstore.h"
}
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  table.insert(package.config[v].buildflags, "no-debug-runtime")
  table.insert(package.config[v].buildflags, "singlethread-runtime")
  for index, name in c_defines do
    table.insert(package.config[v].defines, name)
  end
  if v~="Debug" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end


package = newpackage()
package.name = "gamecode"
package.kind = "lib"
package.language = "c"
package.path = "common"
package.includepaths = { "..", "." }
package.files = matchfiles("gamecode/*.c", "races/*.c", "gamecode/*.h", "races/*.h")
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  table.insert(package.config[v].buildflags, "no-debug-runtime")
  table.insert(package.config[v].buildflags, "singlethread-runtime")
  for index, name in c_defines do
    table.insert(package.config[v].defines, name)
  end
  if v~="Debug" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end


package = newpackage()
package.name = "bindings"
package.kind = "lib"
package.language = "c++"
package.path = "eressea/lua"
package.includepaths = { "../..", "../../common" }
package.files = matchfiles("*.cpp", "*.h")
package.excludes = { "gm.cpp" }
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  table.insert(package.config[v].buildflags, "no-debug-runtime")
  table.insert(package.config[v].buildflags, "singlethread-runtime")
  for index, name in c_defines do
    table.insert(package.config[v].defines, name)
  end
  if v~="Debug" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end

package = newpackage()
package.name = "editor"
package.kind = "lib"
package.language = "c++"
package.path = "eressea"
package.includepaths = { "..", "../common" }
package.files = { "gmtool.h", "gmtool.c", "editing.c", "editing.h", "curses/listbox.h", "curses/listbox.c", "lua/gm.cpp" }
for k,v in project.configs do
  package.config[v].objdir   = v .. "/" .. package.name
  package.config[v].libdir   = v .. "/" .. package.name
  table.insert(package.config[v].buildflags, "extra-warnings")
  table.insert(package.config[v].buildflags, "no-debug-runtime")
  table.insert(package.config[v].buildflags, "singlethread-runtime")
  for index, name in c_defines do
    table.insert(package.config[v].defines, name)
  end
  if v~="Debug" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end

package = newpackage()
package.name = "eressea-lua"
package.kind = "exe"
package.language = "c++"
package.path = "eressea"
package.includepaths = { "..", "../common" }
package.files = { "korrektur.c", "server.cpp", "console.c", "console.h" }
package.links = { "util", "kernel", "gamecode", "bindings", "editor", "libxml2", lib_curses}
for k,v in project.configs do
  package.config[v].objdir   = v
  package.config[v].bindir   = v
  table.insert(package.config[v].buildflags, "extra-warnings")
  table.insert(package.config[v].buildflags, "no-debug-runtime")
  table.insert(package.config[v].buildflags, "singlethread-runtime")
  for index, name in c_defines do
    table.insert(package.config[v].defines, name)
  end
  if (windows) then
    if target=="vs2005" then
      if v=="Debug" then
          package.config[v].links = { "lua5.1_d", "luabind_d" }
      else
        package.config[v].links = { "lua5.1", "luabind" }
      end
    else
      package.config[v].links = { "lua50", "luabind" }
    end
  end
  if v~="Debug" then
    table.insert(package.config[v].defines, "NDEBUG")
  end
end

-- kernel: util, triggers, attributes, spells, items
-- gamecode: races
