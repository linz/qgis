@echo off
set BUILDCONF=RelWithDebInfo

set ERROR_MSG=""
set DEVENV=devenv.exe

set BUILDDIR=%CD%\build
set LOG=%BUILDDIR%\build.log
if not exist "%BUILDDIR%" mkdir %BUILDDIR%
if not exist "%BUILDDIR%" (
    SET ERROR_MSG=Build directory %BUILDDIR% does not exist
    goto error
)

set VERSION=%1
set PACKAGE=%2
set PACKAGENAME=%3
if "%VERSION%"=="" (
    SET ERROR_MSG=version has not been defined
    goto error
)
if "%PACKAGE%"=="" (
    SET ERROR_MSG=package has not been defined
    goto error
)
if "%PACKAGENAME%"=="" set PACKAGENAME=qgis-linz


path %SYSTEMROOT%\system32;%SYSTEMROOT%;%SYSTEMROOT%\System32\Wbem;%PROGRAMFILES%\CMake 2.8\bin;c:\perl\bin;%PROGRAMFILES%\Git\bin
set PYTHONPATH=


if "%OSGEO4W_ROOT%"=="" set OSGEO4W_ROOT=C:\OSGeo4W-LINZ
if not exist "%OSGEO4W_ROOT%\bin\o4w_env.bat" (
    SET ERROR_MSG=Could not set OSGEO4W environment using %OSGEO4W_ROOT%\bin\o4w_env.bat
    goto error
)

set VS90COMNTOOLS=%PROGRAMFILES%\Microsoft Visual Studio 9.0\Common7\Tools\
call "%PROGRAMFILES%\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86

call "%OSGEO4W_ROOT%\bin\o4w_env.bat"
set O4W_ROOT=%OSGEO4W_ROOT:\=/%
set LIB_DIR=%O4W_ROOT%
set LIB=%LIB%;%OSGEO4W_ROOT%\lib
set INCLUDE=%INCLUDE%;%OSGEO4W_ROOT%\include;%OSGEO4W_ROOT%\include\qt4\Qt;%OSGEO4W_ROOT%\include\qt4\QtCore;%OSGEO4W_ROOT%\include\qt4\QtGui;%OSGEO4W_ROOT%\include\qt4\QtNetwork;%OSGEO4W_ROOT%\include\qt4\QtSvg;%OSGEO4W_ROOT%\include\qt4\QtXml;%OSGEO4W_ROOT%\include\qt4\Qt3Support

sed -e "s/@package@/%PACKAGENAME%/g" -e "s/@version@/%VERSION%/g" postinstall.bat >%OSGEO4W_ROOT%\etc\postinstall\%PACKAGENAME%.bat
sed -e "s/@package@/%PACKAGENAME%/g" -e "s/@version@/%VERSION%/g" preremove.bat >%OSGEO4W_ROOT%\etc\preremove\%PACKAGENAME%.bat
sed -e "s/@package@/%PACKAGENAME%/g" -e "s/@version@/%VERSION%/g" qgis.bat.tmpl >%OSGEO4W_ROOT%\bin\%PACKAGENAME%.bat.tmpl
sed -e "s/@package@/%PACKAGENAME%/g" -e "s/@version@/%VERSION%/g" qgis.reg.tmpl >%OSGEO4W_ROOT%\apps\%PACKAGENAME%\bin\qgis.reg.tmpl
sed -e "s/@package@/%PACKAGENAME%/g" -e "s/@version@/%VERSION%/g" browser.bat.tmpl >%OSGEO4W_ROOT%\bin\%PACKAGENAME%-browser.bat.tmpl

tar -C %OSGEO4W_ROOT% -cf %PACKAGENAME%-%VERSION%-%PACKAGE%.tar ^
	apps/%PACKAGENAME% ^
	bin/%PACKAGENAME%.bat.tmpl ^
	bin/%PACKAGENAME%-browser.bat.tmpl ^
	etc/postinstall/%PACKAGENAME%.bat ^
	etc/preremove/%PACKAGENAME%.bat
if errorlevel 1 (
    SET ERROR_MSG=Creating tarball %PACKAGENAME%-%VERSION%-%PACKAGE%.tar has failed
    goto error
)

bzip2 %PACKAGENAME%-%VERSION%-%PACKAGE%.tar
if errorlevel 1 (
    SET ERROR_MSG=Creating bzip2 %PACKAGENAME%-%VERSION%-%PACKAGE%.tar.bz2 has failed
    goto error
)

goto end

:error
echo BUILD ERROR %ERROR_MSG%: %DATE% %TIME%
echo BUILD ERROR %ERROR_MSG%: %DATE% %TIME%>>%LOG% 2>&1
if exist %PACKAGENAME%-%VERSION%-%PACKAGE%.tar.bz2 del %PACKAGENAME%-%VERSION%-%PACKAGE%.tar.bz2

:end
echo FINISHED: %DATE% %TIME% >>%LOG% 2>&1
