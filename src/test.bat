@echo off
setlocal
set count=0
for /r %%f in (*) do set /a count+=1
echo Total files: %count%
pause