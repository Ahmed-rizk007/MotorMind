<div align="center">
  <h1>⚙️ MotorMind 🧠</h1>
  <h3>Edge AI Predictive Maintenance for Industrial Motors</h3>
  
  ![Hardware](https://img.shields.io/badge/Hardware-ESP32-blue?style=for-the-badge&logo=espressif)
  ![Firmware](https://img.shields.io/badge/Firmware-C++-00599C?style=for-the-badge&logo=c%2B%2B)
  ![AI](https://img.shields.io/badge/Edge_AI-TinyML-3776AB?style=for-the-badge&logo=python)
  <br><br>
  <img src="https://img.shields.io/badge/🏆_Award-5th_Place_@_IEEE_IC--SIT_2026-FFD700?style=for-the-badge" alt="IEEE 5th Place Award">
</div>

---

> **🏆 5th Place Winner at IEEE IC-SIT 2026:** MotorMind officially secured the **5th Rank** at the International Competition on Smart Innovation Technologies (IC-SIT'2026) for the submission: *"MotorMind: Edge AI-Based Predictive Maintenance for Industrial Motors Using IOT and Real-Time Signal Processing,"* representing the Faculty of AI, Delta University for Science and Technology, Egypt.
> 
> **[🔗 Click here to view the official IEEE IC-SIT 2026 page](https://r8.ieee.org/egypt-apmtt/ic-sit2026/)**

## 📸 Project Gallery

<div align="center">
  <img src="https://github.com/user-attachments/assets/bd6e08ed-de77-4cca-8ba5-350d6e2d49b3" width="32%" alt="Hardware Setup">
  <img src="https://github.com/user-attachments/assets/f7bfa238-0448-4901-9039-7ced14511307" width="32%" alt="IoT Dashboard">
  <img src="https://github.com/user-attachments/assets/33eccb47-d94a-4450-9e37-f6bc7812abec" width="32%" alt="Telegram Alert">
</div>

## 💡 The Problem vs. Our Solution

| ❌ Traditional Maintenance | ✅ The MotorMind Solution (Edge AI) |
| :--- | :--- |
| **Unplanned Downtime:** Motors fail suddenly, causing massive production losses. | **Reduced Unplanned Downtime:** Predicts anomalies *before* failure occurs. |
| **High Latency:** Cloud-dependent systems take too long to react to sudden spikes. | **Low-Latency Edge Response:** Local Edge AI makes autonomous decisions. |
| **Expensive:** High installation and recurring subscription costs. | **Cost-Effective:** Built on highly affordable ESP32 architecture. |

## 📊 Technical Performance & Benchmarks

### 🧠 Model Performance
| Model | Dataset | Accuracy | F1-Score | Flash Footprint |
| :--- | :--- | :--- | :--- | :--- |
| Random Forest (15 Trees) | MotorMind Custom Dataset | 95.00% | 1.00 | 14.08 KB |
| **TinyML Edge (Logistic Regression)**| **MotorMind Custom Dataset** | **97.79%** | **1.00** | **0.31 KB** |

### ⚡ Edge & Hardware Metrics
* **MCU:** ESP32 (Dual-Core Processing)
* **Sensors:** ADXL345 (Vibration), ACS712 (Current), DS18B20 (Temperature)
* **Edge ML Model:** Logistic Regression (Custom C++ Header `MotorModel.h`)
* **Inference Latency:** `< 1 ms` (Ultra-low latency inference on ESP32)
* **Memory Footprint:** `0.31 KB` (Extremely lightweight, leaving ample memory for FreeRTOS tasks & WiFi)

## 📂 Repository Architecture

| Directory | Description | Status |
| :--- | :--- | :--- |
| 📁 **`/Firmware`** | ESP32 C++ code for real-time sensor data acquisition & relay actuation. | 🚧 Active |
| 📁 **`/Model_Training`** | Python pipelines (FFT, Feature Extraction) for TinyML classification. | 🚧 Active |
| 📁 **`/Data`** | Vibration, Current, and Temp datasets for normal/faulty states. | 📦 Archived |

## 🏗️ How It Works (The Workflow)

1. 📡 **Sense:** Continuous high-frequency sampling using ADXL345, ACS712, and DS18B20 sensors.
2. 🧠 **Process (Edge AI):** The ESP32 dual-core runs FFT and ML inference locally without cloud latency.
3. ⚡ **Act:** Triggers a protective shutdown via industrial contactors for critical detected anomalies.
4. 📱 **Report:** Pushes live telemetry to a Cloud Dashboard & sends instant Telegram alerts.

## 👨‍💻 Development Team
* **Ahmed Rizk** 
* **Nourhan Hamdy** 
* **Mohamed Nagah** 

---
<div align="center">
  <i>Engineered for Reliability in Industry 4.0 🏭</i>
</div>
