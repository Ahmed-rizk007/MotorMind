<div align="center">
  <h1>⚙️ MotorMind 🧠</h1>
  <h3>Edge AI Predictive Maintenance for Industrial Motors</h3>

![Hardware](https://img.shields.io/badge/Hardware-ESP32-blue?style=for-the-badge\&logo=espressif)
![Firmware](https://img.shields.io/badge/Firmware-C++-00599C?style=for-the-badge\&logo=c%2B%2B)
![AI](https://img.shields.io/badge/Edge_AI-TinyML-3776AB?style=for-the-badge\&logo=python) <br><br> <img src="https://img.shields.io/badge/🏆_Award-5th_Place_@_IEEE_IC--SIT_2026-FFD700?style=for-the-badge" alt="IEEE 5th Place Award">

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

| ❌ Traditional Maintenance                                                        | ✅ The MotorMind Solution (Edge AI)                                                                                          |
| :------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------------- |
| **Unplanned Downtime:** Motors fail suddenly, causing massive production losses. | **Reduced Downtime:** Predicts anomalies *before* failure occurs.                                                           |
| **High Latency:** Cloud-dependent systems take too long to react.                | **Low-Latency Edge Response:** Local Edge ML enables autonomous protective decisions without relying on cloud connectivity. |
| **Expensive:** High installation and recurring subscription costs.               | **Cost-Effective:** Built on highly affordable ESP32 architecture.                                                          |

## 🏗️ System Architecture & Dual-Core Pipeline

To ensure the critical protection path is never blocked by network latency, tasks are explicitly pinned to the ESP32's dual cores using FreeRTOS:

<pre>
          INDUSTRIAL MOTOR
                 │
       ┌─────────┼─────────┐
       ↓         ↓         ↓
   Vibration   Current   Temperature
   (ADXL345)  (ACS712)   (DS18B20)
       │         │         │
       └─────────┼─────────┘
                 ↓
      ESP32 Dual-Core Processor
                 │
        ┌────────┴────────┐
        ↓ (Core 1)        ↓ (Core 0)
  Critical Path       IoT & Telemetry
  -------------       ---------------
  1. Acquisition      1. WiFi/MQTT
  2. FFT & Feat.      2. Cloud Sync
  3. ML Inference     3. Telegram Bot
        │                 │
        ↓                 ↓
  Anomaly Decision    Dashboard UI
        │
        ↓
 Prototype Protective
     Shutdown
        │
    Contactor
</pre>

## 🔌 Hardware & Sensor Stack

* **Vibration:** High-frequency sampling using **ADXL345** (target ODR: ~200 Hz).
* **Current:** Real-time load monitoring via **ACS712**.
* **Temperature:** Periodic thermal degradation tracking using **DS18B20**.
* **Actuation:** Relay module simulating industrial contactors for prototype protective shutdown.

## 📊 Technical Performance & Benchmarks

### 🧠 Model Evaluation (MotorMind Custom Dataset)

*The results suggest that the extracted FFT-based feature space is sufficiently linearly separable for Logistic Regression to achieve strong classification performance, while providing a significantly smaller model footprint than the Random Forest baseline.*

| Model                        | Accuracy   | Precision  | Recall     | F1-Score   | Model Size  |
| :--------------------------- | :--------- | :--------- | :--------- | :--------- | :---------- |
| Random Forest (15 Trees)     | 95.50%     | 95.60%     | 95.40%     | 95.50%     | 14.08 KB    |
| **Edge Logistic Regression** | **97.80%** | **97.85%** | **97.75%** | **97.80%** | **0.31 KB** |

*(Note: The 0.31 KB footprint refers strictly to the compiled ML model parameters and weights, leaving ample flash/RAM for FreeRTOS and WiFi stacks.)*

### ⚡ Edge Latency Metrics

* **ML Inference Latency:** `< 1 ms` (Prediction execution time on Core 1).
* **Total Pipeline Latency:** `~15 ms` (Sensor Acquisition → FFT Window → Feature Extraction → Inference → Relay Actuation).

## 📂 Dataset Details & Limitations

### Dataset Specs

* **Total FFT Windows:** `4,800`

  * Normal: `2,400`
  * Unbalance: `1,200`
  * Overload: `1,200`
* **Window Size:** `128` samples per FFT iteration.
* **Train/Validation/Test Split:** `70/15/15`

### ⚠️ Current Limitations & Future Work (Generalization)

Currently, the model achieves high accuracy on the specific motor used during dataset collection. Generalization across different industrial motor models and capacities has not yet been formally validated. Future iterations will involve cross-motor evaluation runs (Training on Motor A, Testing on Motor B) to prove domain adaptation and industrial scalability.

## 👨‍💻 Development Team

* **Ahmed Rizk**
* **Nourhan Hamdy**
* **Mohamed Nagah**

---

<div align="center">
  <i>Engineered for Reliability in Industry 4.0 🏭</i>
</div>
