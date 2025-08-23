A-Z Vibe Coded Project

---

# **ESP32 Air Mouse with WebSocket Control**

This project turns an **ESP32 + MPU6050** into a wireless air mouse for your computer using **WebSockets** for smooth, low-latency control. You can move the mouse by tilting the ESP32 and perform **left clicks** via a physical button.

---

## ✅ **Features**

* Control mouse cursor via ESP32 tilt (using MPU6050).
* Left-click functionality with button press.
* **WebSocket communication** for real-time, smooth movement.
* Calibration support for accurate center positioning.

---

## 📦 **Requirements**

### **Hardware**

* ESP32 development board
* MPU6050 IMU sensor
* Push button
* Jumper wires & breadboard

### **Software**

* **Arduino IDE** with the following libraries:

  * `Wire` (comes pre-installed)
  * **WebSocketsClient** by *Markus Sattler*
  * **ArduinoJson** by *Benoit Blanchon*
* **Python 3** with:

  * `websockets`
  * `pyautogui`

Install Python packages:

```bash
pip install websockets pyautogui
```

---

## 🔧 **Setup Instructions**

### **1. Arduino Side**

* Open Arduino IDE.
* Install libraries:

  * Go to **Sketch → Include Library → Manage Libraries**.
  * Search for and install:

    * `WebSocketsClient`
    * `ArduinoJson`
* Flash the ESP32 code (`ESP32_AirMouse.ino`) to your board.
* Set your **Wi-Fi SSID** and **password** in the code:

  ```cpp
  const char* ssid = "YourWiFiName";
  const char* password = "YourWiFiPassword";
  ```

---

### **2. Python Side (Laptop)**

* Save `air_mouse_receiver.py` and run:

  ```bash
  python air_mouse_receiver.py
  ```
* It will show:

  ```
  ✅ WebSocket Server started at ws://<Laptop_IP>:8888
  ```
* Make sure the IP and port match what you set in the ESP32 code.

---

## ✅ **Usage**

* Power on ESP32, wait for Wi-Fi connection.
* Open Serial Monitor → Check logs for:

  ```
  ✅ Connected to WebSocket: ws://<Laptop_IP>:8888
  ```
* Controls:

  * **Short Press (<0.5s)** → Toggle Air Mouse ON/OFF
  * **Long Press (1–3s)** → Calibrate center position
  * **Very Long (>3s)** → Left Click

---

## 📌 **Important Notes**

* **Both ESP32 and Laptop must be on the same Wi-Fi network.**
* Replace `<Laptop_IP>` in Arduino code with your actual laptop IP (printed by Python script at startup).
* If the mouse drifts, **calibrate again** by long-pressing the button.

---

## 📚 **Learnings**

* Initially used **HTTP requests**, but faced lag and `HTTP -1` errors due to repeated polling and connection instability.
* Switched to **WebSockets** for real-time bidirectional communication → reduced latency and made movement smoother.
* Calibration offsets are essential to avoid constant drift.

