. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolInput = Get-ToolInput $inputJson

$gameDirPattern = '(?i)D:[\\/]+steam[\\/]+steamapps[\\/]+common[\\/]+Crimson Desert[\\/]+bin64'
$modDirPattern = '(?i)D:[\\/]+红色沙漠工具[\\/]+模组[\\/]+Crimson Desert Tweaks'

if ($toolInput -and ($toolInput.PSObject.Properties.Name -contains "command")) {
    $command = [string]$toolInput.command
    $mentionsBuildOutput = $command -match '(?i)build[\\/]+Release'
    $mentionsInstallTarget = $command -match $gameDirPattern -or $command -match $modDirPattern
    $copyLike = $command -match '(?i)\b(copy|cp|xcopy|robocopy|move|mv|Copy-Item|Move-Item)\b'

    if ($mentionsBuildOutput -and $mentionsInstallTarget -and $copyLike) {
        Block-Hook "Blocked: build outputs must stay in build/Release; user installs manually via DMM."
    }
}

Allow-Hook
