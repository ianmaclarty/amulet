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
ChangesAssociations=yes

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.

[Files]
Source: "installer-payload\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\Amulet Console"; Filename: "{cmd}"; Parameters: "/K ""{app}\amulet_env.bat"""; WorkingDir: "{userdocs}"; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\RGB Triangle"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\rgb_triangle.lua"""; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\Kaleidoscope"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\kaleidoscope.lua"""; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\City Lights"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\city.lua"""; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\Paint"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\paint.lua"""; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\5000 Tori"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\3dsandbox.lua"""; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\Tweens"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\tween.lua"""; IconFilename: "{app}\icon.ico" 
Name: "{group}\Examples\Synth"; Filename: "{app}\amulet-window.exe"; Parameters: """{app}\examples\synth.lua"""; IconFilename: "{app}\icon.ico" 

[Tasks]
Name: modifypath; Description: "Add ""{app}"" to your PATH";
Name: fileassoc; Description: "Associate .lua files with Amulet"; Flags: unchecked

[Registry]
Root: HKCR; Subkey: ".lua"; ValueType: string; ValueName: ""; ValueData: "amulet_lua_file"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCR; Subkey: "amulet_lua_file"; ValueType: string; ValueName: ""; ValueData: "Amulet Lua Script"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKCR; Subkey: "amulet_lua_file\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icon.ico,0"; Tasks: fileassoc
Root: HKCR; Subkey: "amulet_lua_file\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\amulet-window.exe"" ""%1"""; Tasks: fileassoc

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
