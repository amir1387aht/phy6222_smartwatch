# 🕹️ phy6222_smartwatch  
Reverse engineering and hacking a cheap smartwatch with the PHY6222 MCU.

<b>For bitmaps: </b>You can show bitmaps using `gfx_draw_rgb_bitmap` function and for generating bitmaps you can use `https://notisrac.github.io/FileToCArray/` tool with `16bit RRRRRGGGGGGBBBBB (2byte/pixel)` color order

<b>Youtube Video: </b>https://youtu.be/wq2xSRMypCg?si=J9CA8hQtWVOK-v0q
---

## 🛠️ How to Build

1. Change `COM_PORT` with your own port in Makfile file setting and run `make` in the root directory.  
2. The code will compile, flash the device, and launch a simple log terminal.  
3. Connect the **GND**, **TX**, **RX**, and **RESET** pins to a UART converter.

---

## 🔌 Connection Images

<table>
  <tr>
    <td align="center">
      <img src="screenshots/ss_1.jpg" alt="TX & GND PIN" width="300"/><br/>
      <b>TX & GND PIN</b>
    </td>
    <td align="center">
      <img src="screenshots/ss_2.jpg" alt="RX PIN" width="300"/><br/>
      <b>RX PIN</b>
    </td>
    <td align="center">
      <img src="screenshots/ss_3.jpg" alt="RESET PIN" width="300"/><br/>
      <b>RESET PIN</b>
    </td>
  </tr>
</table>
