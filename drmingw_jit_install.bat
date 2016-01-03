@if not exist %~dp0drmingw.exe echo you need to copy this script to a directory containing drmingw.exe& goto :eof
echo versioning&@%~dp0drmingw.exe --version || (echo @%~dp0drmingw.exe --version FAILED& goto :eof)
echo installing&@%~dp0drmingw.exe --install --auto || (echo @%~dp0drmingw.exe --install --auto FAILED& goto :eof)
echo @%~dp0drmingw.exe --install --auto SUCCEEDED
