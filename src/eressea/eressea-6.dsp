# Microsoft Developer Studio Project File - Name="eressea" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=eressea - Win32 Conversion
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "eressea-6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "eressea-6.mak" CFG="eressea - Win32 Conversion"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "eressea - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "eressea - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "eressea - Win32 Conversion" (based on "Win32 (x86) Console Application")
!MESSAGE "eressea - Win32 Profile" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
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
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /GX /Z7 /O2 /I ".." /I "../common" /I "../common/util" /I "../common/kernel" /I "../common/gamecode" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386

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
# ADD CPP /nologo /Za /W4 /Gm /ZI /Od /I ".." /I "../common" /I "../common/util" /I "../common/kernel" /I "../common/gamecode" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /S (*.h ../*.h ../common/kernel/eressea.h ../common/gamecode/*.h ../common/util/*.h ../common/triggers/*.h) ../common/gamecode/Debug/*.sbr ../common/kernel/Debug/*.sbr ../common/triggers/Debug/*.sbr ../common/util/Debug/*.sbr
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
# PROP Output_Dir "Conversion"
# PROP Intermediate_Dir "Conversion"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Za /W4 /Gm /Zi /Od /I ".." /I "../util" /I "../common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Za /W4 /Gm /ZI /Od /I ".." /I "../common" /I "../common/util" /I "../common/kernel" /I "../common/gamecode" /D "_CONSOLE" /D "_MBCS" /D "CONVERT_TRIGGER" /D "WIN32" /D "_DEBUG" /D "BETA_CODE" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /out:"Conversion/eressea.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "eressea___Win32_Profile"
# PROP BASE Intermediate_Dir "eressea___Win32_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I ".." /I "../common" /I "../common/util" /I "../common/kernel" /I "../common/gamecode" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Za /W4 /Zi /O2 /I ".." /I "../common" /I "../common/util" /I "../common/kernel" /I "../common/gamecode" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /profile /machine:I386

!ENDIF 

# Begin Target

# Name "eressea - Win32 Release"
# Name "eressea - Win32 Debug"
# Name "eressea - Win32 Conversion"
# Name "eressea - Win32 Profile"
# Begin Group "Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\common\modules\arena.h
# End Source File
# Begin Source File

SOURCE=..\common\modules\museum.h
# End Source File
# Begin Source File

SOURCE=..\common\modules\score.h
# End Source File
# Begin Source File

SOURCE=.\weapons.h
# End Source File
# Begin Source File

SOURCE=..\common\modules\xmas2000.h
# End Source File
# End Group
# Begin Group "Conversion"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\old\pointertags.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\old\pointertags.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\old\relation.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\old\relation.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\old\trigger.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\old\trigger.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Races"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\races\dragons.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\races\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\races\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\races\Release"
# PROP Intermediate_Dir "..\common\races\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\dragons.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\races\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\races\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\races\Release"
# PROP Intermediate_Dir "..\common\races\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\illusion.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\races\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\races\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\races\Release"
# PROP Intermediate_Dir "..\common\races\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\illusion.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\races\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\races\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\races\Release"
# PROP Intermediate_Dir "..\common\races\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\zombies.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\races\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\races\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\races\Release"
# PROP Intermediate_Dir "..\common\races\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\races\zombies.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\races\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\races\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\races\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\races\Release"
# PROP Intermediate_Dir "..\common\races\Profile"

!ENDIF 

# End Source File
# End Group
# Begin Group "Items"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\items\demonseye.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\items\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\items\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\items\Release"
# PROP Intermediate_Dir "..\common\items\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\demonseye.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\items\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\items\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\items\Release"
# PROP Intermediate_Dir "..\common\items\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\lmsreward.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\items\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\items\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\items\Release"
# PROP Intermediate_Dir "..\common\items\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\lmsreward.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\items\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\items\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\items\Release"
# PROP Intermediate_Dir "..\common\items\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\weapons.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\items\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\items\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\items\Release"
# PROP Intermediate_Dir "..\common\items\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\items\weapons.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\items\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\items\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\items\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\items\Release"
# PROP Intermediate_Dir "..\common\items\Profile"

!ENDIF 

# End Source File
# End Group
# Begin Group "Modules"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\modules\arena.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\modules\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\modules\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\modules\Release"
# PROP Intermediate_Dir "..\common\modules\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\modules\gmcmd.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\modules\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\modules\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\modules\Release"
# PROP Intermediate_Dir "..\common\modules\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\modules\museum.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\modules\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\modules\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\modules\Release"
# PROP Intermediate_Dir "..\common\modules\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\modules\oceannames.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\modules\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\modules\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\modules\Release"
# PROP Intermediate_Dir "..\common\modules\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\modules\score.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\modules\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\modules\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\modules\Release"
# PROP Intermediate_Dir "..\common\modules\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\modules\xmas2000.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\modules\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\modules\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\modules\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\modules\Release"
# PROP Intermediate_Dir "..\common\modules\Profile"

!ENDIF 

# End Source File
# End Group
# Begin Group "Attributes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\attributes\follow.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\follow.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\giveitem.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\giveitem.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\hate.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\hate.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\iceberg.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\iceberg.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\key.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\key.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\matmod.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\matmod.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\orcification.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\overrideroads.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\overrideroads.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\reduceproduction.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\reduceproduction.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\targetregion.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\attributes\targetregion.h

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\attributes\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\attributes\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\attributes\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\attributes\Release"
# PROP Intermediate_Dir "..\common\attributes\Profile"

!ENDIF 

# End Source File
# End Group
# Begin Group "Triggers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\triggers\changefaction.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\changerace.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\createcurse.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\createunit.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\giveitem.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\killunit.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\removecurse.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\shock.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\timeout.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\triggers\unitmessage.c

!IF  "$(CFG)" == "eressea - Win32 Release"

# PROP Intermediate_Dir "..\common\triggers\Release"

!ELSEIF  "$(CFG)" == "eressea - Win32 Debug"

# PROP Intermediate_Dir "..\common\triggers\Debug"

!ELSEIF  "$(CFG)" == "eressea - Win32 Conversion"

# PROP Intermediate_Dir "..\common\triggers\Conversion"

!ELSEIF  "$(CFG)" == "eressea - Win32 Profile"

# PROP BASE Intermediate_Dir "..\common\triggers\Release"
# PROP Intermediate_Dir "..\common\triggers\Profile"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\attributes.c
# End Source File
# Begin Source File

SOURCE=.\items.c
# End Source File
# Begin Source File

SOURCE=.\korrektur.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\spells.c
# End Source File
# Begin Source File

SOURCE=.\triggers.c
# End Source File
# End Target
# End Project
