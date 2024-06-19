# @pre: pwd is ${workspaceFolder}
Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell fd4e7f29

taskkill /f /im explorer.exe

# Push-Location x64\Debug\
#     $exitCode = sudo regsvr32 /u ADSExplorer.dll
#     if ($exitCode -ne 0) {
#         Start-Process "explorer.exe"
#         Write-Error "Failed to unregister the DLL"
#         exit $exitCode
#     }
# Pop-Location

# Start-Process cmd.exe `
#     -ArgumentList @("/c", "explorer.exe") `
#     -WindowStyle ([System.Diagnostics.ProcessWindowStyle]::Minimized)

$exitCode = msbuild ADSExplorer.sln /p:Configuration=Debug
if ($exitCode -ne 0) {
    Write-Error "Failed to build the solution"
    exit $exitCode
}

Copy-Item ADSExplorer\x64\Debug\ADSExplorer.tlb x64\Debug\

Push-Location x64\Debug\
    sudo regsvr32 ADSExplorer.dll
Pop-Location
