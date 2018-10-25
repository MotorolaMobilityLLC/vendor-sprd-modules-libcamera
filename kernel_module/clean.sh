find . -name *.o | xargs rm -f
find . -name *.cmd | xargs rm -f
find . -name Module.symvers | xargs rm -f
find . -name modules.order | xargs rm -f
find . -name *.ko | xargs rm -f
find . -name *.mod.c | xargs rm -f
find . -name .tmp_versions | xargs rm -rf
