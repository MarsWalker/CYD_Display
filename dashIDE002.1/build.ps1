# Windows build entry point. Select one complete hardware profile before compiling.
# No credentials are created here: secrets.h must be a local copy of the template.
# A COM port explicitly requests upload; compile failure prevents the upload path.
param([string]$Board, [string]$Port, [switch]$UploadOnly)
$ErrorActionPreference = 'Stop'
# Keep numeric IDs synchronized with board_config.h and build.sh.
$profiles = @{
    'esp32s3_axs15231b_qspi' = @{ Id = 3; Fqbn = 'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB' }
    'esp32_ili9341_xpt2046' = @{ Id = 1; Fqbn = 'esp32:esp32:esp32' }
    'esp32_st7789_parallel_cst820' = @{ Id = 4; Fqbn = 'esp32:esp32:esp32' }
    'esp32_ili9341_cap_touch' = @{ Id = 2; Fqbn = 'esp32:esp32:esp32' }
}
# Normalize every entry point, including a legacy name saved in board.txt.
$aliases = @{
    'amarela2.8' = 'esp32_ili9341_xpt2046'
    'amarela2.2' = 'esp32_st7789_parallel_cst820'
    'pink' = 'esp32_ili9341_cap_touch'
    'black' = 'esp32s3_axs15231b_qspi'
    'preta' = 'esp32s3_axs15231b_qspi'
}
# Persist only the selected profile name; artifact folders remain separated by profile.
$selection = Join-Path $PSScriptRoot 'board.txt'
if (!$Board) { $Board = (Get-Content -LiteralPath $selection -Raw).Trim() }
# Preserve build.cmd COM8 syntax for the saved board.
if ($Board -match '^(COM)?\d+$') {
    if ($Port) { throw 'Use build.cmd BOARD COMPORT' }
    $Port = $Board
    $Board = (Get-Content -LiteralPath $selection -Raw).Trim()
}
$Board = $Board.Trim().ToLowerInvariant()
if ($aliases.ContainsKey($Board)) { $Board = $aliases[$Board] }
if (!$profiles.ContainsKey($Board)) { throw 'Unknown profile. Use a documented hardware ID or personal alias.' }
if ($Port -match '^\d+$') { $Port = 'COM' + $Port }
if ($UploadOnly -and !$Port) { throw 'UploadOnly requires a COM port.' }
$profile = $profiles[$Board]
$cli = if ($env:ARD_CLI) { $env:ARD_CLI } else { 'arduino-cli' }
$libraries = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\libraries'))
$output = Join-Path $PSScriptRoot ('build_out\' + $Board)
$metadata = Join-Path $output 'board-build.json'
# UploadOnly skips rebuilding and must match the stored artifact profile metadata.
if (!$UploadOnly) {
    if (!(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'secrets.h'))) {
        throw 'Copy secrets_template.h to secrets.h and configure it before building.'
    }
    $counter = Join-Path $PSScriptRoot 'build_nr.txt'
    # Increment the attempt number before invoking the compiler, even when compilation fails.
    $number = [int](Get-Content -LiteralPath $counter -Raw).Trim() + 1
    Set-Content -LiteralPath $counter -Value $number -Encoding ascii
    Set-Content -LiteralPath (Join-Path $PSScriptRoot 'build.h') -Value "#pragma once`n#define BUILD_NR $number" -Encoding ascii
    $setup = (Join-Path $PSScriptRoot 'tft_setup.h').Replace('\','/')
    # Force profile setup into the sketch and libraries without editing their global selector.
    $cppFlags = '-DDASH_BOARD=' + $profile.Id + ' -include "' + $setup + '"'
    Write-Host "Building $Board -> $output (build #$number)"
    & $cli compile --fqbn $profile.Fqbn --libraries $libraries --output-dir $output --build-path (Join-Path $PSScriptRoot ('build\' + $Board)) --build-property "compiler.cpp.extra_flags=$cppFlags" --build-property "compiler.c.extra_flags=-DDASH_BOARD=$($profile.Id)" $PSScriptRoot
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $Board" }
    # Publish metadata only after a successful compile, never for stale/partial binaries.
    @{ Board=$Board; Fqbn=$profile.Fqbn; Build=$number; Date=(Get-Date -Format o) } | ConvertTo-Json | Set-Content -LiteralPath $metadata -Encoding ascii
    Set-Content -LiteralPath $selection -Value $Board -Encoding ascii
}
if ($Port) {
    if (!(Test-Path -LiteralPath $metadata)) { throw 'No successful build found for this board. Compile first.' }
    # Protect against loading an artifact compiled for another configured profile.
    $built = Get-Content -LiteralPath $metadata -Raw | ConvertFrom-Json
    if ($built.Board -ne $Board -or $built.Fqbn -ne $profile.Fqbn) { throw 'Build does not match selected board.' }
    & $cli upload --fqbn $profile.Fqbn --port $Port --input-dir $output $PSScriptRoot
    if ($LASTEXITCODE -ne 0) { throw 'Upload failed.' }
}
