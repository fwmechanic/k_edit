@if not defined BITBUCKET_USER echo %%BITBUCKET_USER%% not set?& goto :eof
@hg pull https://%BITBUCKET_USER%@bitbucket.org/fwmechanik/k-editor-linux-port