# Microsoft Developer Studio Project File - Name="askalon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=askalon - Win32 Conversion
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "askalon-6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "askalon-6.mak" CFG="askalon - Win32 Conversion"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "askalon - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "askalon - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "askalon - Win32 Conversion" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "askalon - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "askalon_"
# PROP BASE Intermediate_Dir "askalon_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "askalon - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "askalon0"
# PROP BASE Intermediate_Dir "askalon0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Gm /ZI /Od /I ".." /I "../util" /I "../common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "askalon - Win32 Conversion"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "askalon_"
# PROP BASE Intermediate_Dir "askalon_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "askalon_"
# PROP Intermediate_Dir "askalon_"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Za /W4 /Gm /Zi /Od /I ".." /I "../util" /I "../common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /Za /W4 /Gm /ZI /Od /I ".." /I "../util" /I "../common" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "_DEBUG" /D "CONVERT_TRIGGER" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "askalon - Win32 Release"
# Name "askalon - Win32 Debug"
# Name "askalon - Win32 Conversion"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\modules\arena.h
# End Source File
# Begin Source File

SOURCE=..\modules\museum.h
# End Source File
# Begin Source File

SOURCE=..\modules\score.h
# End Source File
# Begin Source File

SOURCE=.\weapons.h
# End Source File
# Begin Source File

SOURCE=..\modules\xmas2000.h
# End Source File
# End Group
# Begin Group "Conversion"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\old\pointertags.c

!IF  "$(CFG)" == "askalon - Win32 Release"

!ELSEIF  "$(CFG)" == "askalon - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "askalon - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\old\relation.c

!IF  "$(CFG)" == "askalon - Win32 Release"

!ELSEIF  "$(CFG)" == "askalon - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "askalon - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\old\trigger.c

!IF  "$(CFG)" == "askalon - Win32 Release"

!ELSEIF  "$(CFG)" == "askalon - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "askalon - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Attributes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\attributes\attributes.c
# End Source File
# Begin Source File

SOURCE=..\attributes\attributes.h
# End Source File
# Begin Source File

SOURCE=..\attributes\follow.c
# End Source File
# Begin Source File

SOURCE=..\attributes\follow.h
# End Source File
# Begin Source File

SOURCE=..\attributes\hate.c
# End Source File
# Begin Source File

SOURCE=..\attributes\iceberg.c
# End Source File
# Begin Source File

SOURCE=..\attributes\iceberg.h
# End Source File
# Begin Source File

SOURCE=..\attributes\key.c
# End Source File
# Begin Source File

SOURCE=..\attributes\key.h
# End Source File
# Begin Source File

SOURCE=..\attributes\matmod.c
# End Source File
# Begin Source File

SOURCE=..\attributes\matmod.h
# End Source File
# Begin Source File

SOURCE=..\attributes\reduceproduction.c
# End Source File
# Begin Source File

SOURCE=..\attributes\reduceproduction.h
# End Source File
# Begin Source File

SOURCE=..\attributes\targetregion.c
# End Source File
# End Group
# Begin Group "Races"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\races\dragons.c
# End Source File
# Begin Source File

SOURCE=..\races\dragons.h
# End Source File
# Begin Source File

SOURCE=..\races\illusion.c
# End Source File
# Begin Source File

SOURCE=..\races\illusion.h
# End Source File
# Begin Source File

SOURCE=..\races\zombies.c
# End Source File
# Begin Source File

SOURCE=..\races\zombies.h
# End Source File
# End Group
# Begin Group "Items"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\korrektur.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=..\modules\score.c
# End Source File
# Begin Source File

SOURCE=.\triggers.c
# End Source File
# Begin Source File

SOURCE=.\weapons.c
# End Source File
# End Target
# End Project
