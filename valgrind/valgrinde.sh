valgrind --show-leak-kinds=definite,indirect,possible --tool=memcheck --show-leak-kinds=all \
         --track-origins=yes \
         --keep-stacktraces=alloc-and-free \
         --freelist-vol=100000000 \
         --freelist-big-blocks=10000000 \
         --suppressions=./valgrind/valgrind.supp \
         ./build/squel