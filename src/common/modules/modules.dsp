# Microsoft Developer Studio Project File - Name="modules" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=modules - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "modules.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "modules.mak" CFG="modules - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "modules - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "modules - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "modules - Win32 Release"

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
# ADD CPP /nologo /Za /W4 /GX /Z7 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "modules - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "modules"
# PROP BASE Intermediate_Dir "modules"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../kernel" /I "../util" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /YX"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
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

# Name "modules - Win32 Release"
# Name "modules - Win32 Debug"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\arena.h
# End Source File
# Begin Source File

SOURCE=.\dungeon.h
# End Source File
# Begin Source File

SOURCE=.\gmcmd.h
# End Source File
# Begin Source File

SOURCE=.\infocmd.h
# End Source File
# Begin Source File

SOURCE=.\museum.h
# End Source File
# Begin Source File

SOURCE=.\oceannames.h
# End Source File
# Begin Source File

SOURCE=.\score.h
# End Source File
# Begin Source File

SOURCE=.\weather.h
# End Source File
# Begin Source File

SOURCE=.\xecmd.h
# End Source File
# Begin Source File

SOURCE=.\xmas.h
# End Source File
# Begin Source File

SOURCE=.\xmas2000.h
# End Source File
# Begin Source File

SOURCE=.\xmas2001.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\arena.c
# End Source File
# Begin Source File

SOURCE=.\dungeon.c
# End Source File
# Begin Source File

SOURCE=.\gmcmd.c
# End Source File
# Begin Source File

SOURCE=.\infocmd.c
# End Source File
# Begin Source File

SOURCE=.\museum.c
# End Source File
# Begin Source File

SOURCE=.\oceannames.c
# End Source File
# Begin Source File

SOURCE=.\score.c
# End Source File
# Begin Source File

SOURCE=.\weather.c
# End Source File
# Begin Source File

SOURCE=.\xecmd.c
# End Source File
# Begin Source File

SOURCE=.\xmas.c
# End Source File
# Begin Source File

SOURCE=.\xmas2000.c
# End Source File
# Begin Source File

SOURCE=.\xmas2001.c
# End Source File
# End Target
# End Project
