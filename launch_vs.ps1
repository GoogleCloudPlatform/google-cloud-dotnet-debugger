$env:CORECLR_PATH = "C:\Users\quoct\Documents\GitHub\coreclr"

$vsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\IDE\devenv.exe"
#$vsPath = "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\devenv.exe"


$env:CORECLR_ENABLE_PROFILING = 0;
Start-Process $vsPath