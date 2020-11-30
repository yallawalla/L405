del   ..\bin\*.bin, ..\bin\*.hex
copy  iocBoard\object\*.hex  ..\bin\*.hex
srec_cat.exe ..\bin\iocBoard.hex -Intel -crop 0x8008000 -offset -0x8008000 -o ..\bin\iocBoard.bin -Binary
