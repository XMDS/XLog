# XLog
XLog is a plug-in that records the Android system log function (Local), which can record all log information when the current game is running.  It also supports filtering [prio] and [tag] specified types of logs, or generating separate log files for them.

XLog is very important for players and plugin writers, CLEO script writers.
As long as plugin writers use standard Android Log functions:
```
__android_log_write
__android_log_print
__android_log_vprint
__android_log_assert
```

XLog will log the information printed by these 4 log functions and generate a separate local XLog.log file to log them (all plugins can log as long as the writer uses them).  An additional XLog.ini will be generated to set filtering, add separate file functions, players and plugin writers can configure them, all ini options have descriptions and comments, please check XLog.ini.  This plugin can make it easier for plugin writers to debug and print some text records.
