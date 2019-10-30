@echo off

if "%1" equ "x86" (
set libdir=lib
set libsrc=x86
)

if "%1" equ "x86_64" (
set libdir=lib64
set libsrc=x86_64
)

if "%1" equ "arm" (
set libdir=lib
set libsrc=armeabi-v7a
)

if "%1" equ "arm_64" (
set libdir=lib64
set libsrc=arm64-v8a
)

if not defined libsrc (
echo arg: [x86^|x86_64^|arm^|arm_64]
goto SKIP
)

rem echo %libsrc%
rem echo %libdir%

adb root
adb remount
adb push hdr_scndet_in_hist_0x0_20120101105801.yuv /data
adb push IMG_3264x2448_YVU420S_20150115_150526_0_ev=-1.yuv /data
adb push IMG_3264x2448_YVU420S_20150115_150526_2_ev=1.yuv /data

adb shell setprop persist.vendor.cam.hdr2.dump_midbuf yes
adb shell setprop persist.vendor.cam.hdr2.vdsp_log_level 3


echo rm /data/hdr_output.bin > temp
echo rm /data/hdr_out9.nv21 >> temp

echo cd data >> temp
echo test_vdsp 1 -p "32x32"  -d  "/data/hdr_scndet_in_hist_0x0_20120101105801.yuv"  -s "3264x2448"  -i "/data/IMG_3264x2448_YVU420S_20150115_150526_0_ev=-1.yuv|/data/IMG_3264x2448_YVU420S_20150115_150526_2_ev=1.yuv" -o  "hdr_out9" >> temp

for /l %%i in (1,1,1) do (
adb shell < temp
)

del temp


adb pull /data/hdr_output.bin
adb pull /data/hdr_out9.nv21
del compare_result.txt
fc /c /w 	hdr_output.bin hdr_output_golden.bin >compare_result.txt

call log.bat

:SKIP