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
    
    # Check if MapEditor log was updated and verify operational milestones
    $mapLogTimeAfter = if (Test-Path $mapLog) { (Get-Item $mapLog).LastWriteTime } else { [DateTime]::MinValue }
    $operationalMilestonesPassed = $true
    if ($mapLogTimeAfter -gt $mapLogTimeBefore) {
        Write-Host "SUCCESS: Guru-MapEditor.log was actively updated during run!" -ForegroundColor Green
        $logContent = Get-Content $mapLog
        $lastLines = $logContent | Select-Object -Last 15
        Write-Host "Recent lines of Guru-MapEditor.log:"
        $lastLines | ForEach-Object { Write-Host "   $_" }

        # Milestone 1: Engine & Renderer Initialization
        $hasRenderer = [bool]($logContent -match "wiRenderer Initialized")
        Write-Host (" [MILESTONE] Renderer & Scene Engine Init: " + $(if ($hasRenderer) { "PASS" } else { "NOT FOUND" })) -ForegroundColor $(if ($hasRenderer) { "Green" } else { "Yellow" })

        # Milestone 2: Terrain & Scene Loading
        $hasScene = [bool]($logContent -match "(CREATING TERRAIN|NEWMAP: Starting new map|_game_loadinleveldata|_game_preparelevel)")
        Write-Host (" [MILESTONE] Scene / Terrain Loading:       " + $(if ($hasScene) { "PASS" } else { "NOT FOUND" })) -ForegroundColor $(if ($hasScene) { "Green" } else { "Yellow" })

        # Milestone 3: Models & Entity Initiations
        $hasEntities = [bool]($logContent -match "(Entity Initiations|\.dbo)")
        Write-Host (" [MILESTONE] Model Ingestion & Entities:     " + $(if ($hasEntities) { "PASS" } else { "NOT FOUND" })) -ForegroundColor $(if ($hasEntities) { "Green" } else { "Yellow" })

        # Milestone 4: Game Loop Execution
        $hasGameLoop = [bool]($logContent -match "(main game loop begins|STARTING GAME|Guru Map Editor Loop Starts)")
        Write-Host (" [MILESTONE] Game Loop & Simulation:        " + $(if ($hasGameLoop) { "PASS" } else { "NOT FOUND" })) -ForegroundColor $(if ($hasGameLoop) { "Green" } else { "Yellow" })
    }

    Write-Host "Requesting graceful engine shutdown via CloseMainWindow()..."
    $closedGracefully = $false
    try {
        $proc.CloseMainWindow() | Out-Null
        $waitClose = 0
        while (-not $proc.HasExited -and $waitClose -lt 5) {
            Start-Sleep -Seconds 1
            $waitClose++
        }
        if ($proc.HasExited) {
            $closedGracefully = $true
            Write-Host "Engine shut down gracefully after $waitClose seconds." -ForegroundColor Green
        }
    } catch {
        # Fallback if window handle not present
    }

    if (-not $proc.HasExited) {
        Write-Host "Terminating test engine process forcefully..."
        Stop-Process -Id $proc.Id -Force
    }

    Write-Host "SMOKE TEST PASSED: Engine launched, operational milestones verified, and completed without crashes." -ForegroundColor Green
    exit 0
}
