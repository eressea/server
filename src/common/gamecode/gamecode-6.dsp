# Microsoft Developer Studio Project File - Name="gamecode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=gamecode - Win32 Conversion
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gamecode-6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gamecode-6.mak" CFG="gamecode - Win32 Conversion"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gamecode - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "gamecode - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "gamecode - Win32 Conversion" (based on "Win32 (x86) Static Library")
!MESSAGE "gamecode - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gamecode - Win32 Release"

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

!ELSEIF  "$(CFG)" == "gamecode - Win32 Debug"

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
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../kernel" /I "../util" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\gamecode.lib"

!ELSEIF  "$(CFG)" == "gamecode - Win32 Conversion"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gamecode__"
# PROP BASE Intermediate_Dir "gamecode__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Za /W4 /Z7 /Od /I ".." /I "../util" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../util" /I "../kernel" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "CONVERT_TRIGGER" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "gamecode - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "gamecode___Win32_Profile"
# PROP BASE Intermediate_Dir "gamecode___Win32_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../util" /I "../kernel" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /O2 /I "../util" /I "../kernel" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
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

# Name "gamecode - Win32 Release"
# Name "gamecode - Win32 Debug"
# Name "gamecode - Win32 Conversion"
# Name "gamecode - Win32 Profile"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\creation.h
# End Source File
# Begin Source File

SOURCE=.\creport.h
# End Source File
# Begin Source File

SOURCE=.\economy.h
# End Source File
# Begin Source File

SOURCE=.\laws.h
# End Source File
# Begin Source File

SOURCE=.\randenc.h
# End Source File
# Begin Source File

SOURCE=.\study.h
# End Source File
# End Group
# Begin Group "Spells"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\spells\alp.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\creation.c
# End Source File
# Begin Source File

SOURCE=.\creport.c
# End Source File
# Begin Source File

SOURCE=.\economy.c
# End Source File
# Begin Source File

SOURCE=.\laws.c
# End Source File
# Begin Source File

SOURCE=.\monster.c
# End Source File
# Begin Source File

SOURCE=.\randenc.c
# End Source File
# Begin Source File

SOURCE=.\report.c
# End Source File
# Begin Source File

SOURCE=.\spy.c
# End Source File
# Begin Source File

SOURCE=.\study.c
# End Source File
# End Target
# End Project
