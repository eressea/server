# Microsoft Developer Studio Project File - Name="eressea" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=eressea - Win32 Conversion
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit\
 NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den\
 Befehl
!MESSAGE 
!MESSAGE NMAKE /f "eressea-5.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "eressea-5.mak" CFG="eressea - Win32 Conversion"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "eressea - Win32 Release" (basierend auf\
  "Win32 (x86) Console Application")
!MESSAGE "eressea - Win32 Debug" (basierend auf\
  "Win32 (x86) Console Application")
!MESSAGE "eressea - Win32 Conversion" (basierend auf\
  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "eressea_"
# PROP BASE Intermediate_Dir "eressea_"
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

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "eressea0"
# PROP BASE Intermediate_Dir "eressea0"
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/eressea.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "eressea_"
# PROP BASE Intermediate_Dir "eressea_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "eressea_"
# PROP Intermediate_Dir "eressea_"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Za /W4 /Gm /Zi /Od /I ".." /I "../util" /I "../common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /Za /W4 /Gm /Zi /Od /I ".." /I "../util" /I "../common" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "_DEBUG" /D "CONVERT_TRIGGER" /FR /FD /c
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

# Name "eressea - Win32 Release"
# Name "eressea - Win32 Debug"
# Name "eressea - Win32 Conversion"
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

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\old\relation.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\old\trigger.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Races"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\races\dragons.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\dragons.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\illusion.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\illusion.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\zombies.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\zombies.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Items"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\items\demonseye.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\demonseye.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\weapons.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\weapons.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Modules"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\modules\arena.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\modules\museum.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\modules\oceannames.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\modules\score.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\modules\xmas2000.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Attributes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\attributes\follow.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\follow.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\giveitem.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\giveitem.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\hate.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\hate.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\iceberg.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\iceberg.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\key.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\key.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\matmod.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\matmod.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\reduceproduction.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\reduceproduction.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\targetregion.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\targetregion.h

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Group "Triggers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\triggers\changefaction.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\changerace.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\createcurse.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\createunit.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\giveitem.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\killunit.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\removecurse.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\shock.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\timeout.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\unitmessage.c

!IF  "$(CFG)" == "eressea - Win32 Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\korrektur.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\triggers.c
# End Source File
# End Target
# End Project
