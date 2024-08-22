# @pre: pwd is ${workspaceFolder}

$ADSX_REG_PATH = "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{ED383D11-6797-4103-85EF-CBDB8DEB50E2}"

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe
sudo {
    if (Test-Path $ADSX_REG_PATH) {
        Push-Location Active\
            regsvr32 /u ADSExplorer.dll
            if ($LastExitCode -ne 0) {
                Write-Error "Failed to unregister the DLL"
                exit $LastExitCode
            }
        Pop-Location
    }

    Start-Process cmd.exe `
        -ArgumentList @("/c", "explorer.exe") `
        -WindowStyle ([System.Diagnostics.ProcessWindowStyle]::Minimized)

    Copy-Item x64\Debug\* Active\
    Copy-Item ADSExplorer\x64\Debug\ADSExplorer.tlb Active\

    Push-Location Active\
        regsvr32 ADSExplorer.dll
    Pop-Location
}
