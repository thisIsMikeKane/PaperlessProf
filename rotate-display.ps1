[CmdletBinding()]
Param(
  [string]$rotation = 0,
  <#
    Set display orientation
      default - default orientation.
      0       - natural orientation.
      90      - rotated 90 degrees.
      180     - rotated 180 degrees.
      270     - rotated 270 degrees.
      cw      - rotated 90 degrees clockwise.
      ccw     - rotated 90 degrees counter-clockwise.
  #>
  
  [string]$displayName = 'Surface Display'
  #TODO Prompt user the first time, then save to ini file.
  # Out-IniFile - https://gallery.technet.microsoft.com/scriptcenter/7d7c867f-026e-4620-bf32-eca99b4e42f4
  # Get-IniContent - https://gallery.technet.microsoft.com/scriptcenter/ea40c1ef-c856-434b-b8fb-ebd7a76e8d91
)

echo $rotation
echo $displayName

# Find the display number that corresponds to this name. 
(.\display.exe /listdevices | Select-String $displayName) -match '^ (\d) - '
$displayN = $matches[1]

# Rotate the disired display by the desired rotation
.\display.exe /rotate $rotation /device $displayN