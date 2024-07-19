# @pre: pwd is ${workspaceFolder}

& ((Split-Path $MyInvocation.InvocationName) + "\build.ps1")
& ((Split-Path $MyInvocation.InvocationName) + "\reinstall.ps1")
