@echo off
echo del results.txt
del results.txt
FOR %%A IN (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16) DO (
	echo Debug %%A...
        debug\tests.exe saw %%A >> results.txt
)
echo Done!