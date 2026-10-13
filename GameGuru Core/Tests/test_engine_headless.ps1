# Automated Headless / Smoke Test for GameGuru MAX
param(
    [string]$ProjectName = "",
    [int]$TimeoutSeconds = 15
)

$buildDir = "D:\GitHub-repo\GameGuruMAXRepo\BUILD\GameGuru Wicked MAX Build Area\Max"
$exePath = Join-Path $buildDir "GameGuruMAX.exe"
$crashLog = Join-Path $buildDir "Guru-Crash.log"
$mapLog = Join-Path $buildDir "Guru-MapEditor.log"

if (-not (Test-Path $exePath)) {
    Write-Error "GameGuruMAX.exe not found at $exePath"
    exit 1
}

$crashTimeBefore = if (Test-Path $crashLog) { (Get-Item $crashLog).LastWriteTime } else { [DateTime]::MinValue }
$mapLogTimeBefore = if (Test-Path $mapLog) { (Get-Item $mapLog).LastWriteTime } else { [DateTime]::MinValue }

$argsList = @()
if ($ProjectName -ne "") {
    $argsList += "editorfromstandalone=$ProjectName"
}

Write-Host "======================================================"
Write-Host " [SMOKE TEST] Launching GameGuruMAX.exe"
Write-Host " Working Dir: $buildDir"
Write-Host " Arguments:   $($argsList -join ' ')"
Write-Host " Timeout:     $TimeoutSeconds seconds"
Write-Host "======================================================"

$proc = if ($argsList.Count -gt 0) {
    Start-Process -FilePath $exePath -ArgumentList $argsList -WorkingDirectory $buildDir -PassThru
} else {
    Start-Process -FilePath $exePath -WorkingDirectory $buildDir -PassThru
}

if (-not $proc) {
    Write-Host "FAILED: Could not start process." -ForegroundColor Red
    exit 1
}

$timer = 0
while (-not $proc.HasExited -and $timer -lt $TimeoutSeconds) {
    Start-Sleep -Seconds 1
    $timer++
}

$crashTimeAfter = if (Test-Path $crashLog) { (Get-Item $crashLog).LastWriteTime } else { [DateTime]::MinValue }
$crashed = ($crashTimeAfter -gt $crashTimeBefore)

if ($proc.HasExited) {
    $exitCode = $proc.ExitCode
    Write-Host "Process exited early after $timer seconds with exit code: $exitCode"
    if ($crashed) {
        Write-Host "FAILED: Engine CRASHED!" -ForegroundColor Red
        if (Test-Path $crashLog) {
            Get-Content $crashLog | Select-Object -Last 25
        }
        exit 1
    } elseif ($exitCode -ne 0) {
        Write-Host "FAILED: Engine exited with non-zero exit code: $exitCode" -ForegroundColor Red
        exit 1
    } else {
        Write-Host "Engine exited cleanly with exit code 0."
        exit 0
    }
} else {
    Write-Host "Engine survived startup and is actively running after $timer seconds!" -ForegroundColor Green
    if ($crashed) {
        Write-Host "WARNING: Crash log was touched during execution." -ForegroundColor Yellow
        Get-Content $crashLog | Select-Object -Last 25
        Stop-Process -Id $proc.Id -Force
        exit 1
    }
    
    # Check if MapEditor log was updated
    $mapLogTimeAfter = if (Test-Path $mapLog) { (Get-Item $mapLog).LastWriteTime } else { [DateTime]::MinValue }
    if ($mapLogTimeAfter -gt $mapLogTimeBefore) {
        Write-Host "SUCCESS: Guru-MapEditor.log was actively updated during run!" -ForegroundColor Green
        $lastLines = Get-Content $mapLog | Select-Object -Last 10
        Write-Host "Last lines of Guru-MapEditor.log:"
        $lastLines | ForEach-Object { Write-Host "   $_" }
    }

    Write-Host "Terminating test engine process cleanly..."
    Stop-Process -Id $proc.Id -Force
    Write-Host "SMOKE TEST PASSED: Engine launched and initialized without crashes." -ForegroundColor Green
    exit 0
}
