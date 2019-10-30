adb root
adb remount
echo rm /data/logreg.txt > logscript
echo lookat -l 65536 0xf3fae000 ^> /data/logreg.txt >> logscript
adb shell < logscript
del logscript
adb pull /data/logreg.txt
parse logreg.txt
del logreg.txt
