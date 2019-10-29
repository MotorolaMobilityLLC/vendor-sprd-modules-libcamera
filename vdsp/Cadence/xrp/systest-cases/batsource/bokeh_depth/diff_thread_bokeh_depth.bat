adb root
adb remount
del .\VLog /s /q
adb push left.yuv /data
adb push Main_960x720.yuv /data
adb push Main_disp2depth.bin /data
adb push OTP.txt /data
adb push right.yuv /data
adb push output.bmp /data
adb shell < depth_bokeh_process.txt
