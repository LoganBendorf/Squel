Run qmake then make. 
If ccache is not installed, remove the "QMAKE_CXX = ccache g++" line from main.pro. 
If lld is not installed, remove the "QMAKE_LD = lld" line from main.pro.