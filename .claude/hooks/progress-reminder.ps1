. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolInput = Get-ToolInput $inputJson

foreach ($path in (Get-InputPathCandidates $toolInput)) {
    $resolved = Resolve-WorkspacePath $path
    if ($resolved -match '\\src\\.*\.(cpp|h)$') {
        [Console]::Error.WriteLine("Reminder: source files changed; update PROGRESS.md with commands, findings, validation, and next step.")
        Allow-Hook
    }
}

Allow-Hook
