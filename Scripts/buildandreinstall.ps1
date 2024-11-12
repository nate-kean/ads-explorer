# @pre: pwd is ${workspaceFolder}

& ((Split-Path $MyInvocation.InvocationName) + "\build.ps1")
if ($LastExitCode -ne 0) {
    exit $LastExitCode
}
& ((Split-Path $MyInvocation.InvocationName) + "\reinstall.ps1")

explorer.exe
