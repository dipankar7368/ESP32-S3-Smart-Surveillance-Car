# 🚗 ESP32-S3 Wi-Fi RC Car with External ESP32-CAM

A DIY Wi-Fi controlled RC car built using an **ESP32-S3** and an **external ESP32-CAM module**.

The ESP32-S3 is responsible for **car movement, motor speed, camera pan/tilt, and horn control**, while the separate ESP32-CAM provides a **live camera view through its own web page**.

## ✨ Project Features

### 🚗 RC Car Controller — ESP32-S3

* Forward / backward / left / right control
* Adjustable motor speed
* L298N dual-motor control
* Camera pan control using SG90 servo
* Camera tilt control using SG90 servo
* Buzzer horn
* Mobile-friendly control dashboard
* W/A/S/D and arrow-key support
* Automatic motor safety timeout

### 📷 Camera System — External ESP32-CAM

* Separate ESP32-CAM module
* Live camera viewing
* Dedicated camera web page
* Camera operates independently from the RC-car controller
* Can be opened from a phone, tablet, or computer
* Useful for FPV-style RC car projects

## 🧠 System Architecture

```text
                 📱 Phone / PC
                       │
             ┌─────────┴─────────┐
             │                   │
             ▼                   ▼
     🚗 ESP32-S3 Web       📷 ESP32-CAM Web
       Control Page          Camera Page
             │                   │
             ▼                   ▼
          L298N              Live Video
             │                   │
       ┌─────┴─────┐             │
       ▼           ▼             ▼
   DC Motor     DC Motor      Camera Module
       
             │
             ▼
       SG90 Pan/Tilt
       
             │
             ▼
          🔊 Horn
```

## 🌐 Two Separate Web Pages

This project uses **two independent web interfaces**.

### 1️⃣ ESP32-S3 RC Car Controller

Used for:

* 🚗 Driving
* ⚡ Motor speed
* 🎥 Pan
* 🎥 Tilt
* 🔊 Horn

The ESP32-S3 creates the RC-car control web page.

```text
http://192.168.4.1
```

### 2️⃣ ESP32-CAM Camera Page

The external ESP32-CAM provides a separate web interface for viewing the camera.

```text
ESP32-CAM IP Address
```

Open the ESP32-CAM's IP address in your browser to access the live camera view.

> The exact ESP32-CAM IP address depends on the camera firmware and network configuration.

## 🔌 Main Controller Hardware

| Component          | Quantity |
| ------------------ | -------: |
| ESP32-S3           |        1 |
| L298N Motor Driver |        1 |
| DC Gear Motors     |        2 |
| SG90 Servo         |        2 |
| Buzzer             |        1 |
| ESP32-CAM          |        1 |
| RC Car Chassis     |        1 |
| Battery            |        1 |

## 📡 Communication

The project separates **vehicle control** and **video streaming**:

```text
ESP32-S3
   │
   └── RC Car Control
       ├── Motors
       ├── Speed
       ├── Pan Servo
       ├── Tilt Servo
       └── Horn

ESP32-CAM
   │
   └── Camera Streaming
       └── Live Video
```

This keeps the camera processing separate from the main RC-car controller.

## 🎮 Controls

### Mobile Control

The ESP32-S3 web dashboard provides:

* ▲ Forward
* ▼ Backward
* ◀ Left
* ▶ Right
* 🛑 Stop
* ⚡ Speed control
* ↔️ Camera pan
* ↕️ Camera tilt
* 🔊 Horn

### Keyboard

| Key   | Function |
| ----- | -------- |
| W / ↑ | Forward  |
| S / ↓ | Backward |
| A / ← | Left     |
| D / → | Right    |

## 📷 Camera Operation

The **ESP32-CAM is physically mounted on the RC car** and works as the video system.

The camera module runs its own web server. Connect to the camera's network/IP and open its web page to view the live video.

The **SG90 pan and tilt servos are controlled by the ESP32-S3**, allowing the camera to be physically pointed in different directions while the ESP32-CAM handles video capture.

## 🧩 Recommended Repository Structure

```text
ESP32-S3-WiFi-RC-Car/
│
├── RC-Car-ESP32-S3/
│   └── RC-Car-ESP32-S3.ino
│
├── ESP32-CAM/
│   └── ESP32-CAM.ino
│
├── images/
│   ├── rc-car.jpg
│   ├── wiring-diagram.png
│   ├── controller-page.png
│   └── camera-page.png
│
├── README.md
└── LICENSE
```

## 🚀 Future Upgrades

* 📱 Combined control + camera interface
* 🎥 Full-screen FPV mode
* 🕹️ Virtual joystick
* 🔋 Battery monitoring
* 🚨 Obstacle detection
* 📡 Long-range control
* 🤖 Autonomous driving
* 🧠 AI object detection
* 💡 Headlights and indicators
* 📍 GPS tracking

## 📺 YouTube Demo

🎥 Watch the complete project demonstration:

**[▶️ Watch the Project on YouTube](https://youtube.com/shorts/FswMrmbbiWg?si=zjB5lu6dlF7Ph1c-)**


## 👨‍💻 Author

**Dipankar Bhunia**

IoT • Embedded Systems • Robotics • ESP32 • Arduino • DIY Electronics

---

⭐ **Star this repository if you like the project!**

🔧 Built for makers, robotics enthusiasts, and DIY electronics projects.

**Made with ❤️ by Dipankar Bhunia**
