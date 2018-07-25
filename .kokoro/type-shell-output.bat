:: This takes the name of a shell script located in this directory
:: runs the script, output the results to a file and then
:: types the file back to the user.
::
:: This is needed to be able to see shell output on the
:: Windows Kokoro machines.

set TEMP_FILE=%~dp0/../%1

del %TEMP_FILE%.txt

%~dp0write-output-to-file.sh %TEMP_FILE%

type %TEMP_FILE%.txt
del %TEMP_FILE%.txt