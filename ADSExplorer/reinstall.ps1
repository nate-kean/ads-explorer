# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe
Push-Location Active\
    sudo regsvr32 /u ADSExplorer.dll
    if ($LastExitCode -ne 0) {
        Start-Process "explorer.exe"
        Write-Error "Failed to unregister the DLL"
        exit $LastExitCode
    }
Pop-Location

Start-Process cmd.exe `
    -ArgumentList @("/c", "explorer.exe") `
    -WindowStyle ([System.Diagnostics.ProcessWindowStyle]::Minimized)

Copy-Item x64\Debug\* Active\
Copy-Item ADSExplorer\x64\Debug\ADSExplorer.tlb Active\

Push-Location Active\
    sudo regsvr32 ADSExplorer.dll
Pop-Location
