# Microsoft Developer Studio Project File - Name="kernel" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=kernel - Win32 Conversion
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kernel-6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kernel-6.mak" CFG="kernel - Win32 Conversion"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kernel - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "kernel - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "kernel - Win32 Conversion" (based on "Win32 (x86) Static Library")
!MESSAGE "kernel - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kernel - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /GX /Z7 /O2 /I "../util" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "kernel - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "kernel___W"
# PROP BASE Intermediate_Dir "kernel___W"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../util" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\kernel.lib"

!ELSEIF  "$(CFG)" == "kernel - Win32 Conversion"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "kernel___Win32_Conversion"
# PROP BASE Intermediate_Dir "kernel___Win32_Conversion"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Conversion"
# PROP Intermediate_Dir "Conversion"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Za /W4 /Z7 /Od /I "../util" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /YX"stdafx.h" /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../util" /I "../.." /I ".." /D "_WINDOWS" /D "BETA_CODE" /D "CONVERT_TRIGGER" /D "WIN32" /D "_DEBUG" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Debug\kernel.lib"
# ADD LIB32 /nologo /out:"Debug\kernel.lib"

!ELSEIF  "$(CFG)" == "kernel - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kernel___Win32_Profile"
# PROP BASE Intermediate_Dir "kernel___Win32_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../util" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /GX- /Z7 /O2 /I "../util" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "kernel - Win32 Release"
# Name "kernel - Win32 Debug"
# Name "kernel - Win32 Conversion"
# Name "kernel - Win32 Profile"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\alchemy.h
# End Source File
# Begin Source File

SOURCE=.\battle.h
# End Source File
# Begin Source File

SOURCE=.\border.h
# End Source File
# Begin Source File

SOURCE=.\build.h
# End Source File
# Begin Source File

SOURCE=.\building.h
# End Source File
# Begin Source File

SOURCE=.\creation.h
# End Source File
# Begin Source File

SOURCE=.\curse.h
# End Source File
# Begin Source File

SOURCE=.\economy.h
# End Source File
# Begin Source File

SOURCE=.\eressea.h
# End Source File
# Begin Source File

SOURCE=.\faction.h
# End Source File
# Begin Source File

SOURCE=.\group.h
# End Source File
# Begin Source File

SOURCE=.\item.h
# End Source File
# Begin Source File

SOURCE=.\karma.h
# End Source File
# Begin Source File

SOURCE=.\laws.h
# End Source File
# Begin Source File

SOURCE=.\magic.h
# End Source File
# Begin Source File

SOURCE=.\message.h
# End Source File
# Begin Source File

SOURCE=.\monster.h
# End Source File
# Begin Source File

SOURCE=.\movement.h
# End Source File
# Begin Source File

SOURCE=.\names.h
# End Source File
# Begin Source File

SOURCE=.\objtypes.h
# End Source File
# Begin Source File

SOURCE=.\orders.h
# End Source File
# Begin Source File

SOURCE=.\pathfinder.h
# End Source File
# Begin Source File

SOURCE=.\plane.h
# End Source File
# Begin Source File

SOURCE=.\pool.h
# End Source File
# Begin Source File

SOURCE=.\race.h
# End Source File
# Begin Source File

SOURCE=.\randenc.h
# End Source File
# Begin Source File

SOURCE=.\region.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\reports.h
# End Source File
# Begin Source File

SOURCE=.\save.h
# End Source File
# Begin Source File

SOURCE=.\ship.h
# End Source File
# Begin Source File

SOURCE=.\skill.h
# End Source File
# Begin Source File

SOURCE=.\spell.h
# End Source File
# Begin Source File

SOURCE=.\spells.h
# End Source File
# Begin Source File

SOURCE=.\spy.h
# End Source File
# Begin Source File

SOURCE=.\study.h
# End Source File
# Begin Source File

SOURCE=.\teleport.h
# End Source File
# Begin Source File

SOURCE=.\terrain.h
# End Source File
# Begin Source File

SOURCE=.\unit.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\alchemy.c
# End Source File
# Begin Source File

SOURCE=.\battle.c
# End Source File
# Begin Source File

SOURCE=.\border.c
# End Source File
# Begin Source File

SOURCE=.\build.c
# End Source File
# Begin Source File

SOURCE=.\building.c
# End Source File
# Begin Source File

SOURCE=.\combatspells.c
# End Source File
# Begin Source File

SOURCE=.\curse.c
# End Source File
# Begin Source File

SOURCE=.\eressea.c
# End Source File
# Begin Source File

SOURCE=.\faction.c
# End Source File
# Begin Source File

SOURCE=.\group.c
# End Source File
# Begin Source File

SOURCE=.\item.c
# End Source File
# Begin Source File

SOURCE=.\karma.c
# End Source File
# Begin Source File

SOURCE=.\magic.c
# End Source File
# Begin Source File

SOURCE=.\message.c
# End Source File
# Begin Source File

SOURCE=.\movement.c
# End Source File
# Begin Source File

SOURCE=.\names.c
# End Source File
# Begin Source File

SOURCE=.\objtypes.c
# End Source File
# Begin Source File

SOURCE=.\orders.c
# End Source File
# Begin Source File

SOURCE=.\pathfinder.c
# End Source File
# Begin Source File

SOURCE=.\plane.c
# End Source File
# Begin Source File

SOURCE=.\pool.c
# End Source File
# Begin Source File

SOURCE=.\race.c
# End Source File
# Begin Source File

SOURCE=.\region.c
# End Source File
# Begin Source File

SOURCE=.\render.c
# End Source File
# Begin Source File

SOURCE=.\reports.c
# End Source File
# Begin Source File

SOURCE=.\save.c
# End Source File
# Begin Source File

SOURCE=.\ship.c
# End Source File
# Begin Source File

SOURCE=.\skill.c
# End Source File
# Begin Source File

SOURCE=.\spell.c
# End Source File
# Begin Source File

SOURCE=.\teleport.c
# End Source File
# Begin Source File

SOURCE=.\terrain.c
# End Source File
# Begin Source File

SOURCE=.\unit.c
# End Source File
# End Target
# End Project
