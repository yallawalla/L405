del   ..\bin\*.bin, ..\bin\*.hex
copy  L405\object\*.hex  ..\bin\*.hex
srec_cat.exe ..\bin\L405.hex -Intel -crop 0x8008000 -offset -0x8008000 -o ..\bin\L405.bin -Binary


