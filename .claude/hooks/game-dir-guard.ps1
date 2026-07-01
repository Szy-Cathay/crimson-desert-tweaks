. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolInput = Get-ToolInput $inputJson
$gameDir = "D:\steam\steamapps\common\Crimson Desert\bin64"

foreach ($path in (Get-InputPathCandidates $toolInput)) {
    $resolved = Resolve-WorkspacePath $path
    if (Test-PathUnder $resolved $gameDir) {
        Block-Hook "Blocked: writing to the Crimson Desert game directory is not allowed."
    }
}

if ($toolInput -and ($toolInput.PSObject.Properties.Name -contains "command")) {
    $command = [string]$toolInput.command
    if ($command -match '(?i)D:[\\/]+steam[\\/]+steamapps[\\/]+common[\\/]+Crimson Desert[\\/]+bin64') {
        Block-Hook "Blocked: command targets the Crimson Desert game directory."
    }
}

Allow-Hook
