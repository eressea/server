# Microsoft Developer Studio Project File - Name="triggers" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=triggers - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "triggers.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "triggers.mak" CFG="triggers - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "triggers - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "triggers - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "triggers - Win32 Release"

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
# ADD CPP /nologo /Za /W4 /GX /Z7 /O2 /I "../kernel" /I "../util" /I "../.." /I ".." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "triggers - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "triggers"
# PROP BASE Intermediate_Dir "triggers"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../util" /I "../kernel" /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /FD /c
# SUBTRACT CPP /Fr /YX
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

# Name "triggers - Win32 Release"
# Name "triggers - Win32 Debug"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\spells\alp.h
# End Source File
# Begin Source File

SOURCE=.\changefaction.h
# End Source File
# Begin Source File

SOURCE=.\changerace.h
# End Source File
# Begin Source File

SOURCE=.\clonedied.h
# End Source File
# Begin Source File

SOURCE=.\createcurse.h
# End Source File
# Begin Source File

SOURCE=.\createunit.h
# End Source File
# Begin Source File

SOURCE=.\gate.h
# End Source File
# Begin Source File

SOURCE=.\giveitem.h
# End Source File
# Begin Source File

SOURCE=.\killunit.h
# End Source File
# Begin Source File

SOURCE=.\removecurse.h
# End Source File
# Begin Source File

SOURCE=.\shock.h
# End Source File
# Begin Source File

SOURCE=..\spells\spells.h
# End Source File
# Begin Source File

SOURCE=.\timeout.h
# End Source File
# Begin Source File

SOURCE=.\triggers.h
# End Source File
# Begin Source File

SOURCE=.\unguard.h
# End Source File
# Begin Source File

SOURCE=.\unitmessage.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\changefaction.c
# End Source File
# Begin Source File

SOURCE=.\changerace.c
# End Source File
# Begin Source File

SOURCE=.\clonedied.c
# End Source File
# Begin Source File

SOURCE=.\createcurse.c
# End Source File
# Begin Source File

SOURCE=.\createunit.c
# End Source File
# Begin Source File

SOURCE=.\gate.c
# End Source File
# Begin Source File

SOURCE=.\giveitem.c
# End Source File
# Begin Source File

SOURCE=.\killunit.c
# End Source File
# Begin Source File

SOURCE=.\removecurse.c
# End Source File
# Begin Source File

SOURCE=.\shock.c
# End Source File
# Begin Source File

SOURCE=.\timeout.c
# End Source File
# Begin Source File

SOURCE=.\triggers.c
# End Source File
# Begin Source File

SOURCE=.\unguard.c
# End Source File
# Begin Source File

SOURCE=.\unitmessage.c
# End Source File
# End Target
# End Project
