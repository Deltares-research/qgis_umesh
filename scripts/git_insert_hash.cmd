rem @ECHO off

REM This script is used for adding the GIT (short) hash to the properties of
REM a *version.h and a *version.rc file

setlocal enabledelayedexpansion

SET SEARCHTEXT=VN_BUILD_HASH
SET ORG_DIR=%CD%
SET INTEXTFILE=%CD%\%1\%2.cvs
SET OUTTEXTFILE=%CD%\%1\%2
SET TEMPTEXTFILE=%OUTTEXTFILE%.temp


CD /D %CD%\%1

REM GET THE GIT SHORT HASH
FOR /f %%i in ('git rev-parse --short HEAD') do set GIT_HASH=%%i

REM REMOVE PREVIOUS FILE
IF EXIST %TEMPTEXTFILE% (
    DEL %TEMPTEXTFILE% 
)

REM SUBSTITUTE THE GIT SHORT HASH IN TEMPLATE
FOR /f "tokens=1,* delims=¶" %%A IN ( '"type %INTEXTFILE%"') DO (
    SET string=%%A
    SET modified=!string:%SEARCHTEXT%=%GIT_HASH%!
    ECHO !modified! >> %TEMPTEXTFILE%
)

rem update the version numbers major, minor and revision, these values are taken from the file version_number.ini

for /f "delims=" %%a in ('call ..\..\scripts\read_ini.cmd /i major version_number.ini ') do (
    set vn_major=%%a
)
rem echo %vn_major%
for /f "delims=" %%a in ('call  ..\..\scripts\read_ini.cmd /i minor version_number.ini') do (
    set vn_minor=%%a
)
rem echo %vn_minor%
for /f "delims=" %%a in ('call  ..\..\scripts\read_ini.cmd /i revision version_number.ini') do (
    set vn_revision=%%a
)
rem echo %vn_revision%

rem substitute the major, minor and revision numbers 

set JANM=janm
FOR /f "tokens=1,* delims=¶" %%A IN ( '"type %TEMPTEXTFILE%"') DO (
    SET string=%%A
    SET modified=!string:VN_MAJOR=%vn_major%!
    ECHO !modified! >> %JANM%
)
MOVE /Y %JANM% %TEMPTEXTFILE% > NUL

FOR /f "tokens=1,* delims=¶" %%A IN ( '"type %TEMPTEXTFILE%"') DO (
    SET string=%%A
    SET modified=!string:VN_MINOR=%vn_minor%!
    ECHO !modified! >> %JANM%
)
MOVE /Y %JANM% %TEMPTEXTFILE% > NUL

FOR /f "tokens=1,* delims=¶" %%A IN ( '"type %TEMPTEXTFILE%"') DO (
    SET string=%%A
    SET modified=!string:VN_REVISION=%vn_revision%!
    ECHO !modified! >> %JANM%
)
MOVE /Y %JANM% %TEMPTEXTFILE% > NUL


REM COMPARE TEMP FILE WITH OUTFILE


FC /A /L %TEMPTEXTFILE% %OUTTEXTFILE% > NUL 2>&1

if exist %OUTTEXTFILE% (
    IF %ERRORLEVEL% == 0 (
        REM IF THEY ARE IDENTICAL
        DEL %TEMPTEXTFILE% > NUL
    ) ELSE (
        REM IF DIFFERENT
        MOVE /Y %TEMPTEXTFILE% %OUTTEXTFILE% > NUL
    )
)
if exist %TEMPTEXTFILE% (
    MOVE /Y %TEMPTEXTFILE% %OUTTEXTFILE% > NUL
)

CD %ORG_DIR%

:EOF