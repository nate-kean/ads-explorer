# @pre: pwd is ${workspaceFolder}

New-Item -ItemType Directory -Force -Path Active\ | Out-Null

taskkill /f /im explorer.exe | Out-Null

sudo {
	Push-Location Active\
		regsvr32 /s /u ADSExplorer.dll | Out-Null
		if ($LastExitCode -ne 0) {
			Write-Warning "Failed to unregister ADSExplorer.dll (status $LastExitCode)"
		} else {
			Write-Output "Successfully unregistered ADSExplorer.dll"
		}

		Start-Process explorer.exe
	Pop-Location
}
