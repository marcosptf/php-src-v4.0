# Microsoft Developer Studio Project File - Name="dba" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dba - Win32 Debug_TS Berkeley DB3
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dba.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dba.mak" CFG="dba - Win32 Debug_TS Berkeley DB3"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dba - Win32 Release_TS Berkeley DB3" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dba - Win32 Debug_TS Berkeley DB3" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dba - Win32 Release_TS Berkeley DB3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_TS"
# PROP BASE Intermediate_Dir "Release_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_TS"
# PROP Intermediate_Dir "Release_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\.." /I "..\..\Zend" /I "..\..\TSRM" /I "..\..\main" /D ZEND_DEBUG=0 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PHP_EXPORTS" /D "COMPILE_DL_DBA" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_DBA=1 /D DBA_DB3=1 /D DB3_INCLUDE_FILE="db.h" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\.." /I "..\..\Zend" /I "..\..\TSRM" /I "..\..\main" /D ZEND_DEBUG=0 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PHP_EXPORTS" /D "COMPILE_DL_DBA" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_DBA=1 /D DBA_DB3=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 php4ts.lib libdb31s.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\..\Release_TS/php_dba.dll" /libpath:"..\..\Release_TS" /libpath:"..\..\Release_TS_Inline"
# ADD LINK32 php4ts.lib libdb31s.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\..\Release_TS/php_dba.dll" /libpath:"..\..\Release_TS" /libpath:"..\..\Release_TS_Inline"

!ELSEIF  "$(CFG)" == "dba - Win32 Debug_TS Berkeley DB3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_TS"
# PROP BASE Intermediate_Dir "Debug_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_TS"
# PROP Intermediate_Dir "Debug_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\.." /I "..\..\Zend" /I "..\..\TSRM" /I "..\..\main" /D ZEND_DEBUG=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PHP_EXPORTS" /D "COMPILE_DL_DBA" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_DBA=1 /D "DBA_DB3" /D DB3_INCLUDE_FILE="db.h" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\.." /I "..\..\Zend" /I "..\..\TSRM" /I "..\..\main" /D ZEND_DEBUG=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "PHP_EXPORTS" /D "COMPILE_DL_DBA" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_DBA=1 /D "DBA_DB3" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 php4ts_debug.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Debug_TS/php_dba.dll" /pdbtype:sept /libpath:"..\..\Debug_TS"
# ADD LINK32 php4ts_debug.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Debug_TS/php_dba.dll" /pdbtype:sept /libpath:"..\..\Debug_TS"

!ENDIF 

# Begin Target

# Name "dba - Win32 Release_TS Berkeley DB3"
# Name "dba - Win32 Debug_TS Berkeley DB3"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dba.c
# End Source File
# Begin Source File

SOURCE=.\dba_cdb.c
# End Source File
# Begin Source File

SOURCE=.\dba_db2.c
# End Source File
# Begin Source File

SOURCE=.\dba_db3.c
# End Source File
# Begin Source File

SOURCE=.\dba_dbm.c
# End Source File
# Begin Source File

SOURCE=.\dba_gdbm.c
# End Source File
# Begin Source File

SOURCE=.\dba_ndbm.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\php_cdb.h
# End Source File
# Begin Source File

SOURCE=.\php_db2.h
# End Source File
# Begin Source File

SOURCE=.\php_db3.h
# End Source File
# Begin Source File

SOURCE=.\php_dba.h
# End Source File
# Begin Source File

SOURCE=.\php_dbm.h
# End Source File
# Begin Source File

SOURCE=.\php_gdbm.h
# End Source File
# Begin Source File

SOURCE=.\php_ndbm.h
# End Source File
# End Group
# End Target
# End Project
