. "$PSScriptRoot/common.ps1"

$inputJson = Read-HookInput
$toolName = Get-ToolName $inputJson
$toolInput = Get-ToolInput $inputJson

if ($toolName -ne "Bash") {
    Allow-Hook
}

if ($null -eq $toolInput -or -not ($toolInput.PSObject.Properties.Name -contains "command")) {
    Allow-Hook
}

$command = [string]$toolInput.command
$networkPattern = '(?i)\b(git\s+(push|pull|fetch|clone)|curl|wget|cmake\s+(-S|--build|--fresh|--workflow|--install))\b'
$hasProxy = $command -match '127\.0\.0\.1:7890' -or $command -match 'http\.proxy'

if ($command -match $networkPattern -and -not $hasProxy) {
    Block-Hook "Blocked: network-related command must use local VPN proxy http://127.0.0.1:7890."
}

Allow-Hook
