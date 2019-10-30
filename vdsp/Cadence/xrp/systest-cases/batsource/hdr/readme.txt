1. 测试hdr需要调用run.bat ，参数arm/arm_64 视demo程序的位数决定。
目前编译的是arm的版本。
example:
run arm

2.结果输出到compare_result.txt
其中hdr_output.bin是vdsp处理的输出结果， hdr_output_golden.bin是参考结果，二者需要比对二进制一致

compare_result.txt中显示如下字样表示比对一致：
“正在比较文件 hdr_output.bin 和 HDR_OUTPUT_GOLDEN.BIN
FC: 找不到差异
”