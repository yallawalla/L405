- Middlewares/STM32_USBD_Library/Class/MSC dodan na roko in ima include na lokalni Inc, projektni Include ga ne vidi !!!
- zunanji klic void MSC_USB_DEVICE_Init(void)  in referenca na globalni USB handler dodani v usbd_msc.c
- nekaj dvojnih imen zamenjanih v usbd_desc.c
- fops v usbd_msc_storage.h se sklicujejo na reference v diskio.h

- po ponovnem generiranju projekta obrni 