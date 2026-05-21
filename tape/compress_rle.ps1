param(
    [string]$InputFile,
    [string]$OutputFile
)

if (-not (Test-Path $InputFile)) {
    Write-Error "Input file not found: $InputFile"
    exit 1
}

$data = [System.IO.File]::ReadAllBytes($InputFile)
$compressed = New-Object System.Collections.ArrayList

$i = 0
while ($i -lt $data.Length) {
    $byte = $data[$i]
    $count = 1
    while (($i + $count -lt $data.Length) -and ($data[$i + $count] -eq $byte) -and ($count -lt 255)) {
        $count++
    }
    if ($count -gt 2) {
        $compressed.Add(0xC0) | Out-Null
        $compressed.Add($byte) | Out-Null
        $compressed.Add($count) | Out-Null
        $i += $count
    } else {
        $compressed.Add($byte) | Out-Null
        $i++
    }
}

[System.IO.File]::WriteAllBytes($OutputFile, $compressed.ToArray())
$original = $data.Length
$final = $compressed.Count
$pct = [math]::Round(($final / $original) * 100, 1)
Write-Host "Compressed $original bytes -> $final bytes ($pct%)"
