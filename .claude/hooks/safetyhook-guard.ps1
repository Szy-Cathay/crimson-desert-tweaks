. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolInput = Get-ToolInput $inputJson
$workspace = [System.IO.Path]::GetFullPath((Get-Location).Path)
$safetyhookRoot = Join-Path $workspace "deps\safetyhook"

foreach ($path in (Get-InputPathCandidates $toolInput)) {
    $resolved = Resolve-WorkspacePath $path
    if (Test-PathUnder $resolved $safetyhookRoot) {
        Block-Hook "Blocked: deps/safetyhook is required dependency source and must not be modified or removed."
    }
}

if ($toolInput -and ($toolInput.PSObject.Properties.Name -contains "command")) {
    $command = [string]$toolInput.command
    if ($command -match '(?i)deps[\\/]+safetyhook') {
        Block-Hook "Blocked: command touches deps/safetyhook, which must not be modified or removed."
    }
}

Allow-Hook
