# Microsoft Developer Studio Project File - Name="mapper" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=mapper - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit\
 NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den\
 Befehl
!MESSAGE 
!MESSAGE NMAKE /f "mapper-5.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "mapper-5.mak" CFG="mapper - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "mapper - Win32 Release" (basierend auf\
  "Win32 (x86) Console Application")
!MESSAGE "mapper - Win32 Debug" (basierend auf\
  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mapper - Win32 Release"

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

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MDd /Za /W4 /Gm /Zi /Od /I ".." /I "../common" /I "../common/util" /I "../common/kernel" /I "../common/gamecode" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /S (*.h ../*.h ../kernel/eressea.h ../util/*.h ../triggers/*.h) ../kernel/Debug/*.sbr ../triggers/Debug/*.sbr ../util/Debug/*.sbr
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 curses.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/mapper.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "mapper - Win32 Release"
# Name "mapper - Win32 Debug"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\mapper.h
# End Source File
# Begin Source File

SOURCE=..\modules\oceannames.h
# End Source File
# Begin Source File

SOURCE=..\modules\score.h
# End Source File
# Begin Source File

SOURCE=..\common\triggers\triggers.h
# End Source File
# Begin Source File

SOURCE=..\modules\weather.h
# End Source File
# End Group
# Begin Group "Spells"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\spells\alp.c

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\spells\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\spells\alp.h

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\spells\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\spells\spells.c

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\spells\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\spells\spells.h

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\spells\Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Races"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\races\dragons.c

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\dragons.h

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\illusion.c

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\illusion.h

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\zombies.c

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\zombies.h

!IF  "$(CFG)" == "mapper - Win32 Release"

!ELSEIF  "$(CFG)" == "mapper - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\modules\arena.c
# End Source File
# Begin Source File

SOURCE=.\map_modify.c
# End Source File
# Begin Source File

SOURCE=.\map_partei.c
# End Source File
# Begin Source File

SOURCE=.\map_region.c
# End Source File
# Begin Source File

SOURCE=.\map_tools.c
# End Source File
# Begin Source File

SOURCE=.\map_units.c
# End Source File
# Begin Source File

SOURCE=.\mapper.c
# End Source File
# Begin Source File

SOURCE=..\modules\museum.c
# End Source File
# Begin Source File

SOURCE=..\modules\oceannames.c
# End Source File
# Begin Source File

SOURCE=..\modules\score.c
# End Source File
# Begin Source File

SOURCE=..\modules\xmas2000.c
# End Source File
# End Target
# End Project
