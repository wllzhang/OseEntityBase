@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: =============================
:: 参数解析
:: =============================
set "CLEAN_BUILD=0"
set "REBUILD=0"
set "SHOW_HELP=0"

:parse_args
if "%~1"=="" goto :end_parse
if /i "%~1"=="--clean" set "CLEAN_BUILD=1"
if /i "%~1"=="-c" set "CLEAN_BUILD=1"
if /i "%~1"=="--rebuild" set "REBUILD=1"
if /i "%~1"=="-r" set "REBUILD=1"
if /i "%~1"=="--help" set "SHOW_HELP=1"
if /i "%~1"=="-h" set "SHOW_HELP=1"
shift
goto :parse_args

:end_parse
if "%SHOW_HELP%"=="1" (
    echo.
    echo 用法: build_release.bat [选项]
    echo.
    echo 选项:
    echo   --clean, -c        清理旧的编译文件（.obj, .o 等）
    echo   --rebuild, -r      强制重新编译（先清理再编译）
    echo   --help, -h         显示此帮助信息
    echo.
    echo 示例:
    echo   build_release.bat          增量编译（默认，只编译修改的文件）
    echo   build_release.bat --clean   清理编译文件
    echo   build_release.bat --rebuild 强制重新编译所有文件
    echo.
    exit /b 0
)

:: =============================
:: User configurable parameters
:: =============================
:: Qt 安装目录（需要包含对应的 qmake/nmake 工具）
set "QT_DIR=D:\Qt5.14.2\5.14.2\msvc2017_64"
:: Visual Studio 编译环境（根据本地 VS 版本调整）
set "VS_VCVARS=D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
:: 目标构建类型：release 或 debug
set "BUILD_CONFIG=release"

:: =============================
:: 环境检测与初始化
:: =============================
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [ERROR] qmake 未在 %QT_DIR% 中找到，请修改 QT_DIR.
    exit /b 1
)

:: 获取 VS 路径的短路径名（8.3格式）以避免括号解析问题
setlocal DisableDelayedExpansion
for %%I in ("%VS_VCVARS%") do set "VS_SHORT=%%~sI"
endlocal & set "VS_SHORT=%VS_SHORT%"

:: 检查路径是否存在
if not exist "%VS_SHORT%" (
    echo [ERROR] 未找到 vcvarsall.bat
    echo 请检查 VS 安装目录并修改 VS_VCVARS.
    exit /b 1
)

:: 调用 VS 编译环境脚本（使用短路径名）
call "%VS_SHORT%" x64 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] 初始化 VS 编译环境失败.
    exit /b 1
)

set "PATH=%QT_DIR%\bin;%PATH%"

:: =============================
:: 切换到工程目录（脚本所在目录）
:: =============================
pushd "%~dp0"
if errorlevel 1 (
    echo [ERROR] 无法进入脚本目录.
    exit /b 1
)

::: =============================
::: 杀死可能正在运行的 ScenePlan2 进程
::: =============================
echo [INFO] 检查并终止正在运行的 ScenePlan2.exe...
taskkill /F /IM ScenePlan2.exe >nul 2>&1

echo [INFO] 继续执行构建流程...

:: =============================
:: 清理编译文件（如果指定了 --rebuild 或 --clean）
:: =============================
if "%REBUILD%"=="1" set "CLEAN_BUILD=1"

if "%CLEAN_BUILD%"=="1" (
    echo [INFO] 清理旧的编译文件...
    
    :: 清理 Makefile
    if exist Makefile del /q Makefile >nul 2>&1
    if exist Makefile.Debug del /q Makefile.Debug >nul 2>&1
    if exist Makefile.Release del /q Makefile.Release >nul 2>&1
    if exist Makefile.nmake del /q Makefile.nmake >nul 2>&1
    
    :: 清理编译输出目录中的 .obj 文件
    if exist "%BUILD_CONFIG%\*.obj" (
        del /q "%BUILD_CONFIG%\*.obj" >nul 2>&1
    )
    
    :: 清理临时文件
    if exist "%BUILD_CONFIG%\moc_*.cpp" del /q "%BUILD_CONFIG%\moc_*.cpp" >nul 2>&1
    if exist "%BUILD_CONFIG%\ui_*.h" del /q "%BUILD_CONFIG%\ui_*.h" >nul 2>&1
    
    echo [INFO] 清理完成.
    if "%REBUILD%"=="1" (
        echo [INFO] 将执行完整重新编译...
    ) else (
        echo [INFO] 清理完成，退出。使用 --rebuild 参数可执行完整重新编译。
        popd
        exit /b 0
    )
)

:: =============================
:: 生成 Makefile
:: =============================
if exist Makefile.nmake (
    echo [INFO] 检测到旧 Makefile，将重新生成...
    del /q Makefile.nmake >nul 2>&1
)

echo [INFO] 运行 qmake 生成项目文件...
qmake ScenePlan2.pro CONFIG+=!BUILD_CONFIG!
if errorlevel 1 (
    echo [ERROR] qmake 生成 Makefile 失败.
    popd
    exit /b 1
)

:: =============================
:: 编译（nmake 默认支持增量编译）
:: =============================
if "%REBUILD%"=="1" (
    echo [INFO] 开始完整重新编译...
    nmake /nologo
) else (
    echo [INFO] 开始增量编译（只编译修改的文件）...
    echo [INFO] nmake 会自动检测文件变更，只编译需要更新的文件...
    nmake /nologo
)
if errorlevel 1 (
    echo [ERROR] 编译失败.
    popd
    exit /b 1
)

:: =============================
:: 输出结果提示
:: =============================
echo [INFO] 编译完成。目标可执行文件位于 ^
      %CD%\%BUILD_CONFIG%\ScenePlan2.exe

popd
echo [DONE]
exit /b 0
