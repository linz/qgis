set VERSION=%1

set DEVENV=devenv.exe
set BUILDDIR=%CD%\build

path %SYSTEMROOT%\system32;%SYSTEMROOT%;%SYSTEMROOT%\System32\Wbem;%PROGRAMFILES%\CMake 2.8\bin;c:\perl\bin;%PROGRAMFILES%\Git\bin
set PYTHONPATH=
REM set QGIS_PLUGINPATH=\\\\ad\dfs\sites\lh\group\LOL_Operations\Spatial Parcel Improvement Project\software\QGISPlugins
set QGIS_PLUGINPATH=L:/LOL_Operations/Spatial Parcel Improvement Project/software/QGISPlugins
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

if "%BUILDDIR:~1,1%"==":" %BUILDDIR:~0,2%
cd %BUILDDIR%

%DEVENV% qgis%VERSION%.sln

cd ..

