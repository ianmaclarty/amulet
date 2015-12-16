[Setup]
AppName=Amulet
AppVersion={#VERSION}
DefaultDirName={pf}\Amulet-{#VERSION}
DefaultGroupName=Amulet-{#VERSION}
UninstallDisplayIcon={app}\icon.ico
WizardImageFile=installer_img.bmp
WizardSmallImageFile=installer_img_small.bmp
Compression=lzma2
SolidCompression=yes
OutputDir=output
ChangesEnvironment=true

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.

[Files]
Source: "installer-payload\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\Amulet test"; Filename: "{app}\amulet.exe"; Parameters: """{app}\examples\rgb_triangle.lua"""; IconFilename: "{app}\icon.ico" 

[Tasks]
Name: modifypath; Description: "Add ""{app}"" to your path"; Flags: unchecked

[Code]
const
	ModPathName = 'modifypath';
	ModPathType = 'user';

function ModPathDir(): TArrayOfString;
begin
	setArrayLength(Result, 1);
	Result[0] := ExpandConstant('{app}');
end;
#include "modpath.iss"
