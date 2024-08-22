# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe
sudo {
    Push-Location Active\
        regsvr32 /u ADSExplorer.dll
        if ($LastExitCode -ne 0) {
            Write-Warning "Failed to unregister the DLL"
        }
    Pop-Location

    Start-Process cmd.exe `
        -ArgumentList @("/c", "explorer.exe") `
        -WindowStyle ([System.Diagnostics.ProcessWindowStyle]::Minimized)

    Copy-Item x64\Debug\* Active\
    Copy-Item ADSExplorer\x64\Debug\ADSExplorer.tlb Active\

    Push-Location Active\
        regsvr32 ADSExplorer.dll
    Pop-Location
}
