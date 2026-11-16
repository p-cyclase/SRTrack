param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$')]
    [string]$Version
)

$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root 'out'
$dist = Join-Path $root 'dist'
$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("SRTrack-release-" + [guid]::NewGuid())
$archive = Join-Path $dist ("SRTrack-v" + $Version + '.zip')
$required = @(
    'Plugin/SRTrack/SRTrack.aux2',
    'Plugin/SRTrack/SRTrack.lua',
    'Script/SRTrack/SRTrack.mod2',
    'Script/SRTrack/SRTrackObject.obj2',
    'Script/SRTrack/SRTrackObject(fix).obj2',
    'Script/SRTrack/SRTrackObject(txt).obj2'
)

foreach ($relative in $required) {
    if (-not (Test-Path (Join-Path $out $relative) -PathType Leaf)) {
        throw "Missing build product: out/$relative. Run build.cmd first."
    }
}

New-Item -ItemType Directory -Force -Path $stage | Out-Null
New-Item -ItemType Directory -Force -Path $dist | Out-Null
try {
    Copy-Item -Recurse -Force (Join-Path $out 'Plugin') $stage
    Copy-Item -Recurse -Force (Join-Path $out 'Script') $stage
    Copy-Item -Force (Join-Path $root 'docs/release-readme.txt') (Join-Path $stage 'readme.txt')
    if (Test-Path $archive) { Remove-Item -Force $archive }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -CompressionLevel Optimal
    Write-Output "Created: $archive"
}
finally {
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
}
