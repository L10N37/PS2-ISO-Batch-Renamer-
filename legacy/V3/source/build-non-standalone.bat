@echo off
echo Building NON-STANDALONE version...

dotnet publish -c Release -r win-x64 --self-contained false -p:PublishSingleFile=false -o ./publish-non-standalone

echo.
echo Done! Non-standalone version published to: publish-non-standalone
pause
