# Microsoft Developer Studio Project File - Name="util" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=util - Win32 Conversion
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "util-6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "util-6.mak" CFG="util - Win32 Conversion"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "util - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "util - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "util - Win32 Conversion" (based on "Win32 (x86) Static Library")
!MESSAGE "util - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "util - Win32 Release"

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
# ADD CPP /nologo /Za /W4 /GX /Z7 /O2 /I "../.." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "util - Win32 Debug"

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
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\util.lib"

!ELSEIF  "$(CFG)" == "util - Win32 Conversion"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "util___Win32_Conversion"
# PROP BASE Intermediate_Dir "util___Win32_Conversion"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Za /W4 /Z7 /Od /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /YX"stdafx.h" /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /Od /I "../.." /I ".." /D "_WINDOWS" /D "WIN32" /D "_DEBUG" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407
# ADD RSC /l 0x407
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Debug\util.lib"
# ADD LIB32 /nologo /out:"Debug\util.lib"

!ELSEIF  "$(CFG)" == "util - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "util___Win32_Profile"
# PROP BASE Intermediate_Dir "util___Win32_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../.." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Z7 /O2 /I "../.." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
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

# Name "util - Win32 Release"
# Name "util - Win32 Debug"
# Name "util - Win32 Conversion"
# Name "util - Win32 Profile"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\attrib.h
# End Source File
# Begin Source File

SOURCE=.\base36.h
# End Source File
# Begin Source File

SOURCE=.\crmessage.h
# End Source File
# Begin Source File

SOURCE=.\cvector.h
# End Source File
# Begin Source File

SOURCE=.\event.h
# End Source File
# Begin Source File

SOURCE=.\functions.h
# End Source File
# Begin Source File

SOURCE=.\goodies.h
# End Source File
# Begin Source File

SOURCE=.\language.h
# End Source File
# Begin Source File

SOURCE=.\lists.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\message.h
# End Source File
# Begin Source File

SOURCE=.\nrmessage.h
# End Source File
# Begin Source File

SOURCE=.\rand.h
# End Source File
# Begin Source File

SOURCE=.\resolve.h
# End Source File
# Begin Source File

SOURCE=.\translation.h
# End Source File
# Begin Source File

SOURCE=.\umlaut.h
# End Source File
# Begin Source File

SOURCE=.\vmap.h
# End Source File
# Begin Source File

SOURCE=.\vset.h
# End Source File
# Begin Source File

SOURCE=.\windir.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\attrib.c
# End Source File
# Begin Source File

SOURCE=.\base36.c
# End Source File
# Begin Source File

SOURCE=.\cvector.c
# End Source File
# Begin Source File

SOURCE=.\dice.c
# End Source File
# Begin Source File

SOURCE=.\event.c
# End Source File
# Begin Source File

SOURCE=.\functions.c
# End Source File
# Begin Source File

SOURCE=.\goodies.c
# End Source File
# Begin Source File

SOURCE=.\language.c
# End Source File
# Begin Source File

SOURCE=.\lists.c
# End Source File
# Begin Source File

SOURCE=.\log.c
# End Source File
# Begin Source File

SOURCE=.\rand.c
# End Source File
# Begin Source File

SOURCE=.\resolve.c
# End Source File
# Begin Source File

SOURCE=.\umlaut.c
# End Source File
# Begin Source File

SOURCE=.\vmap.c
# End Source File
# Begin Source File

SOURCE=.\vset.c
# End Source File
# Begin Source File

SOURCE=.\windir.c
# End Source File
# End Target
# End Project
