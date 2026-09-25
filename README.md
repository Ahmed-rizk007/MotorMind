<div align="center">
  <h1>⚙️ MotorMind 🧠</h1>
  <h3>Next-Gen Edge AI Predictive Maintenance for Industry 4.0</h3>
  
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


<img width="1536" height="1024" alt="ChatGPT Image 23 يونيو 2026، 01_54_33 م" src="https://github.com/user-attachments/assets/f7bfa238-0448-4901-9039-7ced14511307" />

<img width="1672" height="941" alt="ChatGPT Image 23 يونيو 2026، 02_18_17 م" src="https://github.com/user-attachments/assets/bd6e08ed-de77-4cca-8ba5-350d6e2d49b3" />

<img width="720" height="1280" alt="WhatsApp Image 2026-09-25 at 11 19 05 PM" src="https://github.com/user-attachments/assets/33eccb47-d94a-4450-9e37-f6bc7812abec" />



## 💡 The Problem vs. Our Solution

| ❌ Traditional Maintenance | ✅ The MotorMind Solution (Edge AI) |
| :--- | :--- |
| **Unplanned Downtime:** Motors fail suddenly, costing factories thousands per minute. | **Zero Downtime:** Predicts anomalies *before* failure occurs. |
| **High Latency:** Cloud-dependent systems take too long to react to sudden spikes. | **Ultra-Fast (< 1s):** Local Edge AI cuts power instantly. |
| **Expensive:** High installation and recurring subscription costs. | **Cost-Effective:** Built on highly affordable ESP32 architecture. |

## 📂 Repository Architecture

| Directory | Description | Status |
| :--- | :--- | :--- |
| 📁 **`/Firmware`** | ESP32 C++ code for real-time sensor fusion & relay actuation. | 🚧 Active |
| 📁 **`/Model_Training`** | Python pipelines (FFT, Feature Extraction) for TinyML. | 🚧 Active |
| 📁 **`/Data`** | Vibration, Current, and Temp datasets for normal/faulty states. | 📦 Archived |

## 🏗️ How It Works (The Workflow)

1. 📡 **Sense:** Continuous high-frequency sampling (Vibration, Current, Temperature).
2. 🧠 **Process (Edge AI):** The ESP32 dual-core runs ML inference locally without cloud latency.
3. ⚡ **Act:** Triggers industrial contactors in **under 1 second** for critical, life-saving faults.
4. 📱 **Report:** Pushes live telemetry to a Cloud Dashboard & sends instant Telegram alerts.

## 👨‍💻 Development Team
* **Ahmed Rizk** 
* **Nourhan Hamdy** 
* **Mohamed Nagah** 

---
<div align="center">
  <i>Built to Protect the Heart of Industry 🏭</i>
</div>
