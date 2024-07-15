# @pre: pwd is ${workspaceFolder}

. "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
    ADSExplorer.sln `
    /noLogo /p:Configuration=Debug /p:Platform=x64 `
    | Tee-Object -Variable Output
$Output = $Output.Replace("`r", "") -join "`n"
if ($Output.Contains("Build FAILED.")) {
    exit 1
}
if (
    $Output.Contains("Link:`n  All outputs are up-to-date.") -or
    $Output.Contains("Link:`n  Todas las salidas")
) {
    exit 0
}
if (
    !$Output.Contains("Build succeeded.") -and
    !$Output.Contains(" correcta.")
) {
    Write-Error "Unexpected output from msbuild"
    exit 2
}
