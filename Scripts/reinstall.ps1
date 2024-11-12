# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe | Out-Null

sudo {
	Push-Location Active\
		regsvr32 /s /u ADSExplorer.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Warning "Failed to unregister ADSExplorer.dll (status $LastExitCode)"
		}
		regsvr32 /s /u ADSXContextMenu.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Warning "Failed to unregister ADSXContextMenu.dll (status $LastExitCode)"
		}

		Start-Process explorer.exe

		Copy-Item ..\x64\Debug\* .
		Copy-Item ..\ADSExplorer\x64\Debug\ADSExplorer.tlb .
		Copy-Item ..\ADSXContextMenu\x64\Debug\ADSXContextMenu.tlb .

		regsvr32 /s ADSExplorer.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Error "Failed to register ADSExplorer.dll (status $LastExitCode)"
			exit $LastExitCode
		}
		regsvr32 /s ADSXContextMenu.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Error "Failed to register ADSXContextMenu.dll (status $LastExitCode)"
			exit $LastExitCode
		}
	Pop-Location
}
