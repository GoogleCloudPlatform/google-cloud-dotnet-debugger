:: Builds all source in the repo.

:: The only optional parameter is the configuration which defaults to Debug.

SET ROOT_DIR=%~dp0
SET CONFIG=%1

SET AGENT_DIR=%ROOT_DIR%\src\Google.Cloud.Diagnostics.Debug
SET DEBUGGER_DIR=%ROOT_DIR%\src\google_cloud_debugger

if "%1" == "" (
  SET CONFIG=Debug
)

dotnet build %AGENT_DIR% --configuration %CONFIG%
msbuild %DEBUGGER_DIR%\google_cloud_debugger.sln /p:Configuration=%CONFIG% /p:Platform=x64