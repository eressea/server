# Microsoft Developer Studio Project File - Name="attributes" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=attributes - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "attributes.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "attributes.mak" CFG="attributes - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "attributes - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "attributes - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "attributes - Win32 Release"

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
# ADD CPP /nologo /Za /W4 /GX /Z7 /O2 /I "../util" /I "../kernel" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "attributes - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../util" /I "../kernel" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /YX"stdafx.h" /FD /c
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

# Name "attributes - Win32 Release"
# Name "attributes - Win32 Debug"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\attributes.h
# End Source File
# Begin Source File

SOURCE=.\fleechance.h
# End Source File
# Begin Source File

SOURCE=.\follow.h
# End Source File
# Begin Source File

SOURCE=.\giveitem.h
# End Source File
# Begin Source File

SOURCE=.\gm.h
# End Source File
# Begin Source File

SOURCE=.\hate.h
# End Source File
# Begin Source File

SOURCE=.\iceberg.h
# End Source File
# Begin Source File

SOURCE=.\key.h
# End Source File
# Begin Source File

SOURCE=.\matmod.h
# End Source File
# Begin Source File

SOURCE=.\moved.h
# End Source File
# Begin Source File

SOURCE=.\option.h
# End Source File
# Begin Source File

SOURCE=.\orcification.h
# End Source File
# Begin Source File

SOURCE=.\otherfaction.h
# End Source File
# Begin Source File

SOURCE=.\overrideroads.h
# End Source File
# Begin Source File

SOURCE=.\racename.h
# End Source File
# Begin Source File

SOURCE=.\raceprefix.h
# End Source File
# Begin Source File

SOURCE=.\reduceproduction.h
# End Source File
# Begin Source File

SOURCE=.\roadsoverride.h
# End Source File
# Begin Source File

SOURCE=.\synonym.h
# End Source File
# Begin Source File

SOURCE=.\targetregion.h
# End Source File
# Begin Source File

SOURCE=.\ugroup.h
# End Source File
# Begin Source File

SOURCE=.\viewrange.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\attributes.c
# End Source File
# Begin Source File

SOURCE=.\fleechance.c
# End Source File
# Begin Source File

SOURCE=.\follow.c
# End Source File
# Begin Source File

SOURCE=.\giveitem.c
# End Source File
# Begin Source File

SOURCE=.\gm.c
# End Source File
# Begin Source File

SOURCE=.\hate.c
# End Source File
# Begin Source File

SOURCE=.\iceberg.c
# End Source File
# Begin Source File

SOURCE=.\key.c
# End Source File
# Begin Source File

SOURCE=.\matmod.c
# End Source File
# Begin Source File

SOURCE=.\moved.c
# End Source File
# Begin Source File

SOURCE=.\option.c
# End Source File
# Begin Source File

SOURCE=.\orcification.c
# End Source File
# Begin Source File

SOURCE=.\otherfaction.c
# End Source File
# Begin Source File

SOURCE=.\overrideroads.c
# End Source File
# Begin Source File

SOURCE=.\racename.c
# End Source File
# Begin Source File

SOURCE=.\raceprefix.c
# End Source File
# Begin Source File

SOURCE=.\reduceproduction.c
# End Source File
# Begin Source File

SOURCE=.\synonym.c
# End Source File
# Begin Source File

SOURCE=.\targetregion.c
# End Source File
# Begin Source File

SOURCE=.\ugroup.c
# End Source File
# Begin Source File

SOURCE=.\viewrange.c
# End Source File
# End Target
# End Project
