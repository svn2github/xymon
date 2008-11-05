make -s -f Makefile.home clean
make -s -f Makefile.home 2>&1|egrep -v "unused|not used|In function|Ved..verste niveau|In file included|^ *from |misc.h:38|^Checking for|^config.h |misc.h:.*long long|At top level|signedness"

