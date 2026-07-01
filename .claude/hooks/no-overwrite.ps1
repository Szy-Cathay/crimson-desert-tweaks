. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolName = Get-ToolName $inputJson
$toolInput = Get-ToolInput $inputJson

if ($toolName -ne "Write") {
    Allow-Hook
}

$protectedPatterns = @(
    '\\PROGRESS\.md$',
    '\\CLAUDE\.md$',
    '\\CMakeLists\.txt$',
    '\\src\\.*\.(cpp|h)$'
)

foreach ($path in (Get-InputPathCandidates $toolInput)) {
    $resolved = Resolve-WorkspacePath $path
    if (-not (Test-Path -LiteralPath $resolved)) {
        continue
    }

    foreach ($pattern in $protectedPatterns) {
        if ($resolved -match $pattern) {
            Block-Hook "Blocked: do not overwrite existing protected file with Write. Read it first, then edit it in place: $resolved"
        }
    }
}

Allow-Hook
