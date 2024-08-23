# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

sudo {
	Push-Location Active\
		$ADSX_REG_PATH = `
			"Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{ED383D11-6797-4103-85EF-CBDB8DEB50E2}"
		if (Test-Path $ADSX_REG_PATH) {
			taskkill /f /im explorer.exe
			regsvr32 /u ADSExplorer.dll
			if ($LastExitCode -ne 0) {
				Write-Error "Failed to unregister the DLL"
				exit $LastExitCode
			}
		}

		Start-Process explorer.exe

		Copy-Item ..\x64\Debug\* .
		Copy-Item ..\ADSExplorer\x64\Debug\ADSExplorer.tlb .

		regsvr32 ADSExplorer.dll
	Pop-Location
}
