param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$')]
    [string]$Version
)

$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root 'out'
$dist = Join-Path $root 'dist'
$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("SRTrack-release-" + [guid]::NewGuid())
$packageStage = Join-Path ([System.IO.Path]::GetTempPath()) ("SRTrack-package-" + [guid]::NewGuid())
$archive = Join-Path $dist ("SRTrack-v" + $Version + '.zip')
$packageArchive = Join-Path $dist ("SRTrack-v" + $Version + '.au2pkg.zip')
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
New-Item -ItemType Directory -Force -Path $packageStage | Out-Null
New-Item -ItemType Directory -Force -Path $dist | Out-Null
try {
    Copy-Item -Recurse -Force (Join-Path $out 'Plugin') $stage
    Copy-Item -Recurse -Force (Join-Path $out 'Script') $stage
    Copy-Item -Force (Join-Path $root 'docs/release-readme.txt') (Join-Path $stage 'readme.txt')
    if (Test-Path $archive) { Remove-Item -Force $archive }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -CompressionLevel Optimal
    Write-Output "Created: $archive"

    Copy-Item -Recurse -Force (Join-Path $out 'Plugin') $packageStage
    Copy-Item -Recurse -Force (Join-Path $out 'Script') $packageStage
    $packageIni = @"
[package]
id=p_cyclase.SRTrack
name=SRTrack
information=SRTrack v$Version by p_cyclase
"@
    $packageText = @"
SRTrack v$Version

AviUtl2 で編集した動画に対応する SRT 字幕を出力するプラグインです。

動作確認環境: AviUtl2 / ExEdit2 2.1.13
前提プラグイン: GCMZDrops2、L-SMASH Works

詳しい使いかた、不具合報告、更新情報:
https://github.com/p-cyclase/SRTrack
"@
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText((Join-Path $packageStage 'package.ini'), $packageIni, $utf8)
    [System.IO.File]::WriteAllText((Join-Path $packageStage 'package.txt'), $packageText, $utf8)
    if (Test-Path $packageArchive) { Remove-Item -Force $packageArchive }
    Compress-Archive -Path (Join-Path $packageStage '*') -DestinationPath $packageArchive -CompressionLevel Optimal
    Write-Output "Created: $packageArchive"
}
finally {
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    if (Test-Path $packageStage) { Remove-Item -Recurse -Force $packageStage }
}
