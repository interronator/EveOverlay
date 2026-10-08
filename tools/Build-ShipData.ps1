<#
Builds universeData\types.tsv, the table the d-scan reader uses to turn what a scan lists into ships, drones and structures.

1. Download the JSON Lines static data export from https://developers.eveonline.com/static-data
   (eve-online-static-data-latest-jsonl.zip) and unzip types.jsonl, groups.jsonl and categories.jsonl into one folder.
2. Run: powershell -ExecutionPolicy Bypass -File tools\Build-ShipData.ps1 -SdeDirectory <that folder>

Each output line is: type id, type name, group name, category name, separated by tabs.
#>
param(
    [Parameter(Mandatory = $true)][string]$SdeDirectory,
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

if ($OutputPath -eq '')
{
    $OutputPath = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\universeData\types.tsv'
}

# Ship, Entity (NPCs), Drone, Deployable, Starbase, Structure, Fighter
$WantedCategories = @(6, 11, 18, 22, 23, 65, 87)

function Read-English($Line)
{
    $Match = [regex]::Match($Line, '"name": \{[^}]*?"en": "((?:[^"\\]|\\.)*)"')
    if ($Match.Success -eq $false)
    {
        return $null
    }

    return [regex]::Unescape($Match.Groups[1].Value)
}

$CategoryNames = @{}
foreach ($Line in [System.IO.File]::ReadLines((Join-Path $SdeDirectory 'categories.jsonl')))
{
    $Key = [regex]::Match($Line, '"_key": (\d+)')
    $Name = Read-English $Line
    if ($Key.Success -and $Name)
    {
        $CategoryNames[[int]$Key.Groups[1].Value] = $Name
    }
}

$Groups = @{}
foreach ($Line in [System.IO.File]::ReadLines((Join-Path $SdeDirectory 'groups.jsonl')))
{
    $Key = [regex]::Match($Line, '"_key": (\d+)')
    $Category = [regex]::Match($Line, '"categoryID": (\d+)')
    $Name = Read-English $Line
    if ($Key.Success -and $Category.Success -and $Name -and ($WantedCategories -contains [int]$Category.Groups[1].Value))
    {
        $Groups[[int]$Key.Groups[1].Value] = @{ Name = $Name; Category = $CategoryNames[[int]$Category.Groups[1].Value] }
    }
}

$Rows = New-Object System.Collections.Generic.List[string]
foreach ($Line in [System.IO.File]::ReadLines((Join-Path $SdeDirectory 'types.jsonl')))
{
    $GroupMatch = [regex]::Match($Line, '"groupID": (\d+)')
    if ($GroupMatch.Success -eq $false)
    {
        continue
    }

    $Group = $Groups[[int]$GroupMatch.Groups[1].Value]
    if ($null -eq $Group)
    {
        continue
    }

    $Key = [regex]::Match($Line, '^\{"_key": (\d+)')
    $Name = Read-English $Line
    if ($Key.Success -eq $false -or -not $Name)
    {
        continue
    }

    $Rows.Add(($Key.Groups[1].Value, $Name, $Group.Name, $Group.Category) -join "`t")
}

$Sorted = $Rows | Sort-Object { [int]($_ -split "`t")[0] }
[System.IO.File]::WriteAllLines($OutputPath, $Sorted, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "Wrote $($Sorted.Count) types to $OutputPath"
