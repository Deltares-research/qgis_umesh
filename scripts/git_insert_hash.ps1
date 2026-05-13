param (
    [Parameter(Mandatory=$true)]
    [string]$IncludeDir,

    [Parameter(Mandatory=$true)]
    [string]$FileName
)

$ErrorActionPreference = "Stop"

Write-Host "ARG1=$IncludeDir"
Write-Host "ARG2=$FileName"

# --- Normalize paths ---
$IncludeDir   = Resolve-Path $IncludeDir
$InTextFile   = Join-Path $IncludeDir "$FileName.vcs"
$OutTextFile  = Join-Path $IncludeDir $FileName
$TempTextFile = "$OutTextFile.temp"

if (-not (Test-Path $InTextFile)) {
    throw "Input template not found: $InTextFile"
}

Push-Location $IncludeDir

# --- Constants ---
$SEARCHTEXT       = "VCS_BUILD_HASH"
$SEARCHSOURCEURL  = "VCS_SOURCE_URL"

# --- Git state ---
$GitModified = ""
if (git diff --stat origin/master) {
    $GitModified = "M-"
}

$GitHash = git rev-parse --short HEAD
$GitDate = git show -s --format="%cd" --date=format:"%Y-%m-%d %H:%M:%S" HEAD

$FullHash = "$GitDate, $GitModified$GitHash"
Write-Host "GIT DATE/HASH: $FullHash"

# --- Source URL ---
$SourceUrl = git ls-remote --get-url
if (-not $SourceUrl) {
    throw "SOURCE_URL not found (git not available?)"
}

# --- Version numbers ---
function Read-IniValue($Key) {
    cmd /c "..\..\scripts\read_ini.cmd /i $Key version_number.ini"
}

$VN_MAJOR    = Read-IniValue "major"
$VN_MINOR    = Read-IniValue "minor"
$VN_REVISION = Read-IniValue "revision"

Write-Host "Version: $VN_MAJOR.$VN_MINOR.$VN_REVISION"

# --- Transform file ---
$content = Get-Content $InTextFile -Raw

$content = $content -replace $SEARCHTEXT,       $FullHash
$content = $content -replace $SEARCHSOURCEURL,  $SourceUrl
$content = $content -replace "VN_MAJOR",        $VN_MAJOR
$content = $content -replace "VN_MINOR",        $VN_MINOR
$content = $content -replace "VN_REVISION",     $VN_REVISION

# --- Write temp file ---
Set-Content -Path $TempTextFile -Value $content -NoNewline

# --- Compare and replace if changed ---
if (Test-Path $OutTextFile) {
    if ((Get-FileHash $TempTextFile).Hash -eq (Get-FileHash $OutTextFile).Hash) {
        Remove-Item $TempTextFile
        Write-Host "No changes detected"
    }
    else {
        Move-Item $TempTextFile $OutTextFile -Force
        Write-Host "Updated $OutTextFile"
    }
}
else {
    Move-Item $TempTextFile $OutTextFile -Force
    Write-Host "Created $OutTextFile"
}

Pop-Location
