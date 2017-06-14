```
Subset of files from the Core CLR source repository required to build. The
repository link is https://github.com/dotnet/coreclr.

dbgshim.h, located at /coreclr/src/dlls/dbgshim/dbgshim.h

All files at git commit 86103a4

The header file in this repository only contains a subset of all the functions
in the original header file. Also, we remove __in and __out in the header file
for compilation on Linux.

```
