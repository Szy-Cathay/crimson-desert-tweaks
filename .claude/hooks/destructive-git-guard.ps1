. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolInput = Get-ToolInput $inputJson

if ($toolInput -and ($toolInput.PSObject.Properties.Name -contains "command")) {
    $command = [string]$toolInput.command
    $dangerous = @(
        '(?i)\bgit\s+reset\s+--hard\b',
        '(?i)\bgit\s+checkout\s+--\s+\.',
        '(?i)\bgit\s+restore\s+\.',
        '(?i)\bgit\s+clean\s+-[^\s]*[fd][^\s]*\b'
    )

    foreach ($pattern in $dangerous) {
        if ($command -match $pattern) {
            Block-Hook "Blocked: destructive git restore/clean command. Ask the user and explain the risk before changing this guard."
        }
    }
}

Allow-Hook
