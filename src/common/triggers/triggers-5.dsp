# Microsoft Developer Studio Project File - Name="triggers" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=triggers - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit\
 NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den\
 Befehl
!MESSAGE 
!MESSAGE NMAKE /f "triggers-5.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "triggers-5.mak" CFG="triggers - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "triggers - Win32 Release" (basierend auf\
  "Win32 (x86) Static Library")
!MESSAGE "triggers - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

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
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
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
# ADD CPP /nologo /MDd /Za /W4 /Z7 /Od /I "../util" /I "../kernel" /I "../.." /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX"stdafx.h" /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\triggers.lib"

!ENDIF 

# Begin Target

# Name "triggers - Win32 Release"
# Name "triggers - Win32 Debug"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\changefaction.h
# End Source File
# Begin Source File

SOURCE=.\changerace.h
# End Source File
# Begin Source File

SOURCE=.\createcurse.h
# End Source File
# Begin Source File

SOURCE=.\createunit.h
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

SOURCE=.\timeout.h
# End Source File
# Begin Source File

SOURCE=.\triggers.h
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

SOURCE=.\createcurse.c
# End Source File
# Begin Source File

SOURCE=.\createunit.c
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

SOURCE=.\unitmessage.c
# End Source File
# End Target
# End Project
