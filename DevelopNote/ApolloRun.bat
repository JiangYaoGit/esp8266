@echo off
for /f "tokens=4" %%a in ('route print^|findstr 0.0.0.0.*0.0.0.0') do (
 set IP=%%a
)
echo ÄãµÄIPÊÇ£º%IP%
cmd /k "cd /d D:\7MQTT\apache-apollo-1.7.1\bin\myapollo\bin&&apollo-broker.cmd run"