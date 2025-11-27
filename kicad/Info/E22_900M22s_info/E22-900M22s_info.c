
E22-900M22s 
SX1262
ESP32-C3 Super Mini

#==========
SX1276 — старый, отработанный чип, что делает модули на его базе более доступными.
E22 (SX1262) имеет более современную схему приёмника.
#==========

ali
https://aliexpress.ru/item/1005002700411209.html?spm=a2g2w.orderdetail.0.0.13664aa6QOrpgY&sku_id=12000021771434528

www.cdebyte.com 
https://www.cdebyte.com/products/E22-900M22S

pinout 
E22-900M22S_UserManual_EN_v1.3.pdf

esp32c3 
E22-900M22S

ESP32C3-E22-Mesh-Node

====
$ ls variants/esp32c3/
ai-c3  diy  hackerboxes_esp32c3_oled  heltec_esp32c3  heltec_hru_3601  m5stack-stamp-c3

===============
; Super Mini C3 with SX1262
[env:supermini_c3_sx1262]
extends = heltec_esp32c3  ; используем heltec как базовую конфигурацию
board = esp32-c3-devkitm-1  ; или другая подходящая плата
build_flags = 
    ${env.build_flags}
    -DUSING_SX1262
    ; Стандартные пины для SX1262 (можно изменить под свою распиновку)
    -DRADIOLIB_CS=7
    -DRADIOLIB_DIO1=3
    -DRADIOLIB_RST=4
    -DRADIOLIB_BUSY=5

 default_envs:
ini

[platformio]
default_envs = supermini_c3_sx1262
=======================
