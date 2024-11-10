# @pre: pwd is ${workspaceFolder}

if ($null -eq (Get-Command "msbuild.exe" -ErrorAction SilentlyContinue)) {
    $RealPwd = $pwd
    Import-Module "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
    Enter-VsDevShell fd4e7f29
    Set-Location $RealPwd
}

devenv ADSXContextMenu.sln /Clean
