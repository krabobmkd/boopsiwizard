ZIPNAME=circlegadget
zip -u ${ZIPNAME}.zip *.c
zip -u ${ZIPNAME}.zip *.h
zip -u ${ZIPNAME}.zip *.s
zip -u ${ZIPNAME}.zip *.txt
zip -u ${ZIPNAME}.zip makefile
zip -u ${ZIPNAME}.zip smakefile
# you can have recursion in dirs, for resource or else:
#zip -u ${ZIPNAME}.zip sub/woot.png


