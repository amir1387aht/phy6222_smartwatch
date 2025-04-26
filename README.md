# 🕹️ phy6222_smartwatch  
Reverse engineering and hacking a cheap smartwatch with the PHY6222 MCU.

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