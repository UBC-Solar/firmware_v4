<#
.SYNOPSIS
    Set up the sim on Windows, once per machine (safe to re-run, e.g. after pulling).

.DESCRIPTION
    Installs, into gitignored folders:
      1. the pinned Renode build in sim\.renode\
      2. the Python environment in sim\.venv\ (pytest, pyrenode3 ... from sim\requirements.txt)
    Linux: setup.sh next to this file does the same.

    Needs git (pip fetches pyrenode3 from GitHub) and Python 3.11-3.15 from python.org (the
    "py" launcher is used if present). Renode's Windows build brings its own .NET; nothing else to
    install. Works in Windows PowerShell 5.1 and PowerShell 7.

    Why a pinned Renode, not "whatever Renode you have": the chip model
    (emulation\chip\stm32f103rct6.repl) uses peripheral classes that only exist in newer Renodes (the
    tagged 1.16.1 release has no F103 DMA or STM32F1_I2C). pytest uses sim\.renode\. Set
    PYRENODE_PATH to use another Renode folder (at your own risk).

    To move to a newer Renode, in BOTH setup.ps1 and setup.sh: change RenodeBuild and the checksum
    (SHA256 of the Windows .zip here, of the Linux .tar.gz there). Re-run, copy
    sim\.renode\platforms\cpus\stm32f103.repl over sim\emulation\chip\stm32f103rct6.repl (never edit
    that copy; our fixes live in sim\emulation\chip\stm32f103rct6.resc), run the tests.

.PARAMETER Python
    Python to create sim\.venv with: a path or command name, e.g. C:\Python312\python.exe.
    Default: the newest Python 3.11-3.15 the "py" launcher (python.org) knows, else "python".

.EXAMPLE
    From the repo root (-ExecutionPolicy Bypass because Windows blocks unsigned scripts by default):
    powershell -ExecutionPolicy Bypass -File sim\setup\setup.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File sim\setup\setup.ps1 -Python C:\Python312\python.exe
#>
[CmdletBinding()]
param(
    [string]$Python = $env:PYTHON
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# PowerShell 7.3+ can turn a native command's stderr into errors; keep it off so Python/pip warnings
# don't stop the script (failures are caught through exit codes instead)
$PSNativeCommandUseErrorActionPreference = $false
# Windows PowerShell 5.1 downloads many times slower while drawing a progress bar
$ProgressPreference = 'SilentlyContinue'

$RenodeBuild = 'renode-1.16.1+20260623git3f5a91013'
$RenodeSha256 = '39addb5f071fdbdefe7423405b18be32fdf59757d05175c69505302f4ab03e26'
$Url = "https://builds.renode.io/$RenodeBuild.windows-portable-dotnet.zip"

# Python versions pythonnet (in requirements.txt) supports: >=3.11, <3.16
$PyMinMinor = 11
$PyMaxMinor = 15

$SimDir = Split-Path -Parent $PSScriptRoot
$Dest = Join-Path $SimDir '.renode'
$Stamp = Join-Path $Dest '.build'
$Venv = Join-Path $SimDir '.venv'
$VenvPython = Join-Path $Venv 'Scripts\python.exe'

function Install-Renode {
    if ((Test-Path -LiteralPath $Stamp) -and
        ("$(Get-Content -LiteralPath $Stamp -Raw)".Trim() -eq $RenodeBuild) -and
        (Test-Path -LiteralPath (Join-Path $Dest 'renode.exe'))) {
        Write-Host "Renode $RenodeBuild already installed in $Dest"
        return
    }

    $tmp = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName())
    [IO.Directory]::CreateDirectory($tmp) | Out-Null
    try {
        $zipPath = Join-Path $tmp 'renode.zip'
        Write-Host "Downloading $Url (~150 MB)"
        # Windows PowerShell 5.1 may not offer TLS 1.2 by default
        [Net.ServicePointManager]::SecurityProtocol =
            [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $Url -OutFile $zipPath -UseBasicParsing

        $hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
        if ($hash -ne $RenodeSha256) {  # -ne ignores case; Get-FileHash returns upper case
            throw "Checksum mismatch on the Renode download (got $hash): not installing"
        }

        # Extract straight into .renode\, dropping the zip's one top folder
        # (renode_<version>-portable/). Not Expand-Archive + Move-Item: in Windows PowerShell 5.1
        # Move-Item can't move a folder from %TEMP% to another drive.
        Add-Type -AssemblyName System.IO.Compression
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
        try {
            $top = $zip.Entries[0].FullName.Split('/')[0] + '/'
            $files = @($zip.Entries | Where-Object { -not $_.FullName.EndsWith('/') })
            foreach ($entry in $files) {
                if (-not $entry.FullName.StartsWith($top)) {
                    throw "Unexpected layout in the Renode zip: $($entry.FullName)"
                }
            }

            # Windows limits paths to 259 characters unless long paths are enabled (and only
            # PowerShell 7 honours that). Check before deleting the old install.
            $longest = ($files | ForEach-Object { $_.FullName.Length - $top.Length } |
                Measure-Object -Maximum).Maximum
            $needed = $Dest.Length + 1 + $longest
            if ($needed -gt 259) {
                $longPaths = $false
                if ($PSVersionTable.PSVersion.Major -ge 7) {
                    $key = Get-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' `
                        -Name LongPathsEnabled -ErrorAction SilentlyContinue
                    $longPaths = ($null -ne $key) -and ($key.LongPathsEnabled -eq 1)
                }
                if (-not $longPaths) {
                    throw ("Renode's files would need paths of $needed characters under $Dest; " +
                        "Windows allows 259. Clone the repo to a shorter folder (e.g. C:\src\firmware_v4), " +
                        "or enable long paths and run this with PowerShell 7 (pwsh).")
                }
            }

            if (Test-Path -LiteralPath $Dest) {
                Remove-Item -LiteralPath $Dest -Recurse -Force
            }
            # [IO.Directory], not New-Item: New-Item -Path treats [ ] in folder names as wildcards
            [IO.Directory]::CreateDirectory($Dest) | Out-Null
            $destRoot = [IO.Path]::GetFullPath($Dest) + [IO.Path]::DirectorySeparatorChar
            foreach ($entry in $files) {
                $relative = $entry.FullName.Substring($top.Length).Replace('/', [IO.Path]::DirectorySeparatorChar)
                $target = [IO.Path]::GetFullPath([IO.Path]::Combine($Dest, $relative))
                if (-not $target.StartsWith($destRoot)) {
                    throw "Refusing to extract outside $Dest`: $($entry.FullName)"
                }
                [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
                [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target, $true)
            }
        }
        finally {
            $zip.Dispose()
        }

        if (-not (Test-Path -LiteralPath (Join-Path $Dest 'renode.exe'))) {
            throw "renode.exe missing after extracting the Renode zip"
        }
        Set-Content -LiteralPath $Stamp -Value $RenodeBuild -Encoding Ascii
        Write-Host "Installed $RenodeBuild in $Dest"
    }
    finally {
        Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}

# Runs Python ($exe plus $preArgs, e.g. "py" "-3") with more arguments; throws if it fails
function Invoke-Python([string]$exe, [string[]]$preArgs, [string[]]$moreArgs) {
    & $exe @preArgs @moreArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Failed (exit code $LASTEXITCODE): $exe $(($preArgs + $moreArgs) -join ' ')"
    }
}

# Throws unless the Python is one the requirements support
function Assert-PythonVersion([string]$exe, [string[]]$preArgs) {
    $version = & $exe @preArgs -c "import sys; print('%d.%d' % sys.version_info[:2])"
    if ($LASTEXITCODE -ne 0 -or -not $version) {
        # e.g. the Microsoft Store "python" stub, which only prints a message
        throw "Can't run Python ($exe $($preArgs -join ' ')). Install Python 3.$PyMinMinor-3.$PyMaxMinor from python.org, or pass -Python <path to python.exe>."
    }
    $parts = "$version".Trim().Split('.')
    $major = [int]$parts[0]
    $minor = [int]$parts[1]
    if ($major -ne 3 -or $minor -lt $PyMinMinor -or $minor -gt $PyMaxMinor) {
        throw ("$exe $($preArgs -join ' ') is Python $version; need 3.$PyMinMinor-3.$PyMaxMinor (pythonnet). " +
            "Pass -Python <path to python.exe>, and delete $Venv first if it was made with the wrong one.")
    }
}

function Install-PythonEnv {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "git not found (pip needs it to fetch pyrenode3). Install Git for Windows and reopen the terminal."
    }

    if (-not (Test-Path -LiteralPath $VenvPython)) {
        if ($Python) {
            $exe = $Python
            $preArgs = @()
        }
        elseif (Get-Command py -ErrorAction SilentlyContinue) {
            # The python.org launcher: newest installed version we support ("py -3" could pick an
            # unsupported newer one)
            $exe = 'py'
            $preArgs = $null
            foreach ($minor in $PyMaxMinor..$PyMinMinor) {
                # 'Continue' here: Windows PowerShell 5.1 turns redirected stderr ("not installed")
                # into a terminating error under 'Stop'
                $ErrorActionPreference = 'Continue'
                & py "-3.$minor" -c "pass" 2>$null
                $found = ($LASTEXITCODE -eq 0)
                $ErrorActionPreference = 'Stop'
                if ($found) {
                    $preArgs = @("-3.$minor")
                    break
                }
            }
            if ($null -eq $preArgs) {
                throw "The py launcher has no Python 3.$PyMinMinor-3.$PyMaxMinor. Install one from python.org, or pass -Python <path to python.exe>."
            }
        }
        elseif (Get-Command python -ErrorAction SilentlyContinue) {
            $exe = 'python'
            $preArgs = @()
        }
        else {
            throw "Python not found. Install Python 3.$PyMinMinor-3.$PyMaxMinor from python.org, or pass -Python <path to python.exe>."
        }
        if (-not (Get-Command $exe -ErrorAction SilentlyContinue)) {
            throw "Python not found: $exe"
        }
        Assert-PythonVersion $exe $preArgs
        Write-Host "Creating Python environment in $Venv"
        Invoke-Python $exe $preArgs @('-m', 'venv', $Venv)
    }
    Assert-PythonVersion $VenvPython @()

    # Run every time: a no-op when requirements.txt hasn't changed, an update when it has
    Write-Host "Installing Python packages (requirements.txt)"
    Invoke-Python $VenvPython @() @('-m', 'pip', 'install', '--quiet', '--disable-pip-version-check',
        '-r', (Join-Path $SimDir 'requirements.txt'))
}

Write-Host '== Renode'
Install-Renode
Write-Host '== Python'
Install-PythonEnv

Write-Host ''
Write-Host 'Done. Run the tests from the repo root:  sim\.venv\Scripts\pytest sim'
