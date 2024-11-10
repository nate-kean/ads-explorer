# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe | Out-Null

sudo {
	Push-Location Active\
		regsvr32 /s /u ADSXContextMenu.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Warning "Failed to unregister ADSXContextMenu.dll (status $LastExitCode)"
		}

		Start-Process explorer.exe

		Copy-Item ..\x64\Debug\* .
		Copy-Item ..\ADSXContextMenu\x64\Debug\ADSXContextMenu.tlb .

		regsvr32 /s ADSXContextMenu.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Error "Failed to register ADSXContextMenu.dll (status $LastExitCode)"
			exit $LastExitCode
		}
	Pop-Location
}
