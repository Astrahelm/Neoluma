import json

exePath = ".build/.runtime/Debug/bin/neoluma.exe"
casesRoot = "../cases"


# """param(
#     [string]$ExePath = ".build/.runtime/Debug/bin/neoluma.exe",
#     [string]$CasesRoot = "tests/cases",
#     [string[]]$Suites = @("parser", "semantic", "orchestrator")
# )
#
# $ErrorActionPreference = "Stop"
# $PSNativeCommandUseErrorActionPreference = $false
#
# function Strip-Ansi {
#     param([string]$Text)
#     return [regex]::Replace($Text, "\x1B\[[0-9;?]*[ -/]*[@-~]", "")
# }
#
# function Get-ActualStage {
#     param(
#         [string]$Suite,
#         [string]$ErrorCode
#     )
#
#     if ($Suite -eq "orchestrator") {
#         return "orchestrator"
#     }
#
#     if ($ErrorCode -like "NSyE*") { return "parser" }
#     if ($ErrorCode -like "NPrE*") { return "orchestrator" }
#     if ($ErrorCode -like "NAnE*") { return "semantic" }
#     return ""
# }
#
# function New-ProjectFile {
#     param([string]$ProjectPath)
#
#     @'
# [project]
# name = "NeolumaFrontendTest"
# version = "1.0"
# authors = ["User"]
# license = "custom"
# output = "exe"
# sourceFolder = "src/"
# buildFolder = ".build/"
# '@ | Set-Content -LiteralPath $ProjectPath
# }
#
# function Invoke-Case {
#     param(
#         [string]$Suite,
#         [string]$Kind,
#         [string]$CasePath
#     )
#
#     $expectPath = Join-Path $CasePath "expect.json"
#     $expect = Get-Content -LiteralPath $expectPath -Raw | ConvertFrom-Json
#
#     $tmpRoot = Join-Path "tests/.tmp" ("{0}-{1}-{2}" -f $Suite, $Kind, [IO.Path]::GetFileName($CasePath))
#     if (Test-Path $tmpRoot) {
#         Remove-Item -LiteralPath $tmpRoot -Recurse -Force
#     }
#
#     $srcRoot = Join-Path $tmpRoot "src"
#     New-Item -ItemType Directory -Path $srcRoot -Force | Out-Null
#
#     Get-ChildItem -LiteralPath $CasePath -Recurse -File | Where-Object { $_.Name -ne "expect.json" } | ForEach-Object {
#         $relative = $_.FullName.Substring($CasePath.Length).TrimStart('\', '/')
#         $dest = Join-Path $srcRoot $relative
#         $destDir = Split-Path -Parent $dest
#         if ($destDir) {
#             New-Item -ItemType Directory -Path $destDir -Force | Out-Null
#         }
#         Copy-Item -LiteralPath $_.FullName -Destination $dest -Force
#     }
#
#     $projectPath = Join-Path $tmpRoot "case.nlp"
#     New-ProjectFile -ProjectPath $projectPath
#
#     $command = ('"{0}" check --project "{1}" --json 2>&1' -f $ExePath, $projectPath)
#     $rawOutput = & cmd /c $command | Out-String
#     $output = Strip-Ansi $rawOutput
#     $jsonStart = $output.IndexOf('{')
#     $jsonEnd = $output.LastIndexOf('}')
#     if ($jsonStart -lt 0 -or $jsonEnd -lt $jsonStart) {
#         throw "No JSON payload found in output for case '$CasePath'. Raw output:`n$output"
#     }
#     $jsonText = $output.Substring($jsonStart, $jsonEnd - $jsonStart + 1)
#     $resultJson = $jsonText | ConvertFrom-Json
#     $status = $resultJson.status
#
#     $errorCode = ""
#     $actualStage = ""
#     $line = $null
#     $column = $null
#     $messageKey = ""
#
#     if ($resultJson.errors.Count -gt 0) {
#         $firstError = $resultJson.errors[0]
#         $errorCode = $firstError.error_code
#         $actualStage = if ($firstError.stage) { $firstError.stage } else { Get-ActualStage -Suite $Suite -ErrorCode $errorCode }
#         if ($null -ne $firstError.line) { $line = [int]$firstError.line }
#         if ($null -ne $firstError.column) { $column = [int]$firstError.column }
#         $messageKey = $firstError.message_key
#         $output = $firstError.message
#     } else {
#         $output = ""
#     }
#
#     $failures = @()
#     if ($expect.status -ne $status) {
#         $failures += "status expected '$($expect.status)' got '$status'"
#     }
#
#     if ($expect.stage -and $expect.stage -ne $actualStage) {
#         $failures += "stage expected '$($expect.stage)' got '$actualStage'"
#     }
#
#     if ($expect.error_code -and $expect.error_code -ne $errorCode) {
#         $failures += "error_code expected '$($expect.error_code)' got '$errorCode'"
#     }
#
#     if ($null -ne $expect.line -and $expect.line -ne $line) {
#         $failures += "line expected '$($expect.line)' got '$line'"
#     }
#
#     if ($null -ne $expect.column -and $expect.column -ne $column) {
#         $failures += "column expected '$($expect.column)' got '$column'"
#     }
#
#     if ($expect.message_key -and $expect.message_key -ne $messageKey) {
#         $failures += "message_key expected '$($expect.message_key)' got '$messageKey'"
#     }
#
#     if ($expect.message_contains -and -not $output.Contains($expect.message_contains)) {
#         $failures += "message missing '$($expect.message_contains)'"
#     }
#
#     [pscustomobject]@{
#         Suite = $Suite
#         Kind = $Kind
#         Case = [IO.Path]::GetFileName($CasePath)
#         Passed = ($failures.Count -eq 0)
#         Failures = $failures
#         Output = $output.Trim()
#     }
# }
#
# if (-not (Test-Path -LiteralPath $ExePath)) {
#     throw "Executable not found: $ExePath"
# }
#
# New-Item -ItemType Directory -Path "tests/.tmp" -Force | Out-Null
#
# $results = @()
# foreach ($suite in $Suites) {
#     foreach ($kind in @("valid", "invalid")) {
#         $root = Join-Path $CasesRoot "$suite/$kind"
#         if (-not (Test-Path -LiteralPath $root)) { continue }
#
#         Get-ChildItem -LiteralPath $root -Directory | Sort-Object Name | ForEach-Object {
#             $results += Invoke-Case -Suite $suite -Kind $kind -CasePath $_.FullName
#         }
#     }
# }
#
# $failed = $results | Where-Object { -not $_.Passed }
# foreach ($result in $results) {
#     if ($result.Passed) {
#         Write-Host "[PASS] $($result.Suite)/$($result.Kind)/$($result.Case)"
#     } else {
#         Write-Host "[FAIL] $($result.Suite)/$($result.Kind)/$($result.Case)"
#         foreach ($failure in $result.Failures) {
#             Write-Host "  - $failure"
#         }
#     }
# }
#
# Write-Host ""
# Write-Host ("Passed {0}/{1} cases." -f ($results.Count - $failed.Count), $results.Count)
#
# if ($failed.Count -gt 0) {
#     exit 1
# }
"""