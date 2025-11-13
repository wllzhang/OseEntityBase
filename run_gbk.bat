@echo off
:: 使用 GBK 编码运行程序（Windows 控制台默认编码，更可靠）

setlocal
pushd "%~dp0"

:: 确定程序路径
set "EXE_PATH="
if exist "release\ScenePlan2.exe" (
    set "EXE_PATH=%~dp0release\ScenePlan2.exe"
) else if exist "debug\ScenePlan2.exe" (
    set "EXE_PATH=%~dp0debug\ScenePlan2.exe"
) else (
    echo [ERROR] 未找到 ScenePlan2.exe，请先编译程序。
    popd
    exit /b 1
)

:: 创建临时批处理文件来运行程序
set "TEMP_BAT=%TEMP%\run_sceneplan2_gbk_%RANDOM%.bat"
(
    echo @echo off
    echo :: 设置控制台代码页为 GBK^(936^)
    echo chcp 936 ^>nul
    echo :: 切换到程序目录
    echo cd /d "%~dp0"
    echo :: 运行程序
    echo call "%EXE_PATH%"
    echo :: 清理临时文件
    echo del "%%~f0" ^>nul 2^>^&1
    echo pause
    echo exit
) > "%TEMP_BAT%"

:: 验证临时文件是否创建成功
if not exist "%TEMP_BAT%" (
    echo [ERROR] 无法创建临时批处理文件
    popd
    exit /b 1
)

:: 尝试使用 Windows Terminal (如果安装了)
where wt.exe >nul 2>&1
if %errorlevel% == 0 (
    echo [INFO] 使用 Windows Terminal 运行程序...
    wt.exe cmd /c "%TEMP_BAT%"
    popd
    exit /b 0
)

:: 使用 start 命令在新窗口中运行
echo [INFO] 在新窗口中运行程序（GBK 编码）...
start "" cmd /c "%TEMP_BAT%"
popd
exit /b 0

