@echo off
set today_data=%date:~0,4%%date:~5,2%%date:~8,2%
set now_time=%time:~0,2%%time:~3,2%%time:~6,2%
for %%i in ("%cd%") do set dir_name=%%~ni
7z a "%dir_name%-%today_data%-%now_time%" -r
adb push "%dir_name%-%today_data%-%now_time%.7z" /sdcard/CodeBackup/
pause