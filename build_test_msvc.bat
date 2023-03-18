@echo off

call vcvars64

call cl ./test/main.c -Z7 -Od -EHsc -MT /TC /I"%VK_SDK_PATH%\Include" /utf-8 /validate-charset /link /DEBUG:FULL
