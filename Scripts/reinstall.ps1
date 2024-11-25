# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe | Out-Null

sudo {
	Push-Location Active\
		regsvr32 /s /u ADSExplorer.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Warning "Failed to unregister ADSExplorer.dll (status $LastExitCode)"
		}

		Start-Process explorer.exe

		Copy-Item ..\x64\Debug\* .
		Copy-Item ..\ADSExplorer\x64\Debug\ADSExplorer.tlb .

		regsvr32 /s ADSExplorer.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Error "Failed to register ADSExplorer.dll (status $LastExitCode)"
			exit $LastExitCode
		}
	Pop-Location
}
