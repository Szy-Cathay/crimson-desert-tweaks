function Read-HookInput {
    $raw = [Console]::In.ReadToEnd()
    if ([string]::IsNullOrWhiteSpace($raw)) {
        return $null
    }

    try {
        return $raw | ConvertFrom-Json -ErrorAction Stop
    } catch {
        return $null
    }
}

function Get-ToolName($input) {
    if ($null -eq $input) {
        return ""
    }

    if ($input.tool_name) {
        return [string]$input.tool_name
    }

    if ($input.toolName) {
        return [string]$input.toolName
    }

    return ""
}

function Get-ToolInput($input) {
    if ($null -eq $input) {
        return $null
    }

    if ($input.tool_input) {
        return $input.tool_input
    }

    if ($input.toolInput) {
        return $input.toolInput
    }

    return $null
}

function Get-InputPathCandidates($toolInput) {
    $paths = New-Object System.Collections.Generic.List[string]
    if ($null -eq $toolInput) {
        return $paths
    }

    foreach ($name in @("file_path", "filePath", "path")) {
        if ($toolInput.PSObject.Properties.Name -contains $name) {
            $value = $toolInput.$name
            if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
                $paths.Add([string]$value)
            }
        }
    }

    if ($toolInput.PSObject.Properties.Name -contains "edits") {
        foreach ($edit in $toolInput.edits) {
            foreach ($name in @("file_path", "filePath", "path")) {
                if ($edit.PSObject.Properties.Name -contains $name) {
                    $value = $edit.$name
                    if (-not [string]::IsNullOrWhiteSpace([string]$value)) {
                        $paths.Add([string]$value)
                    }
                }
            }
        }
    }

    return $paths
}

function Resolve-WorkspacePath([string]$path) {
    if ([string]::IsNullOrWhiteSpace($path)) {
        return ""
    }

    if ([System.IO.Path]::IsPathRooted($path)) {
        return [System.IO.Path]::GetFullPath($path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $path))
}

function Test-PathUnder([string]$path, [string]$root) {
    if ([string]::IsNullOrWhiteSpace($path) -or [string]::IsNullOrWhiteSpace($root)) {
        return $false
    }

    $fullPath = [System.IO.Path]::GetFullPath($path).TrimEnd('\')
    $fullRoot = [System.IO.Path]::GetFullPath($root).TrimEnd('\')
    return $fullPath.Equals($fullRoot, [System.StringComparison]::OrdinalIgnoreCase) -or
        $fullPath.StartsWith($fullRoot + '\', [System.StringComparison]::OrdinalIgnoreCase)
}

function Block-Hook([string]$message) {
    [Console]::Error.WriteLine($message)
    exit 2
}

function Allow-Hook {
    exit 0
}
