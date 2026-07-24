@echo off

for /f "tokens=* USEBACKQ" %%x in (`powershell -Command "[guid]::NewGuid()"`) do (
  set guid=%%x
)

set appId=USBSync

cmake -B build -DUSBSYNC_GUID=%guid% -DUSBSYNC_APP_ID=%appId% . || exit /b 1
cmake --build build --config Release || exit /b 1
cmake --install build --config Release --prefix install || exit /b 1

wix build -src dist\USBSync.wxs -bindpath install -out install\USBSyncInstaller.msi -pdbtype none -arch x64 ^
  -define name=usbsync -define displayName=USBSync -define exe=usbsync.exe -define root=install ^
  -define version=0.1.0 -define upgradeCode=%guid% || exit /b 1
