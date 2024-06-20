# @pre: pwd is ${workspaceFolder}

if (-not(Get-Command "msbuild.exe" -ErrorAction SilentlyContinue)) {
    $RealPwd = $pwd
    Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
    Enter-VsDevShell fd4e7f29
    Set-Location $RealPwd
}

msbuild ADSExplorer.sln /noLogo `
    /p:Configuration=Debug `
    /p:Platform=x64 `
    | Tee-Object -Variable Output
$Output = $Output.Replace("`r", "") -join "`n"
if ($Output.Contains("Build FAILED.")) {
    exit 1
}
if ($Output.Contains("Link:`n  All outputs are up-to-date.")) {
    exit 0
}
if (!$Output.Contains("Build succeeded.")) {
    Write-Error "Unexpected output from msbuild"
    exit 2
}

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

exit 0
