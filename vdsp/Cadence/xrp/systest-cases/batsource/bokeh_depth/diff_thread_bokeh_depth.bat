adb root
adb remount
del .\VLog /s /q

adb shell /vendor/bin/test_vdsp 0 800x600 800x600 800x600 /data/left.yuv /data/right.yuv /data/OTP.txt /data/output.bmp  /data.param.yuv 0 0 0 0 1
