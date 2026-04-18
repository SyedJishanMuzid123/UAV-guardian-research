# UAV-Guardian: AI-Driven Predictive Maintenance

## 📌 Project Overview
**UAV-Guardian** is a Research-Grade AIoT system designed to detect mechanical failures in drones (such as propeller damage or motor imbalance) using real-time vibration analysis. 

---

## 🛠️ Research Methodology
This project implements a custom **Deep Learning pipeline** optimized for resource-constrained microcontrollers (TinyML), bypassing "black-box" automated tools to allow for deep architectural research.

### 1. Data Acquisition & Synthetic Augmentation
* **Hardware:** ESP32 Microcontroller + MPU6050 Accelerometer.
* **Sampling:** Data captured at **100Hz** to capture mechanical harmonics.
* **Augmentation:** To simulate structural damage without destroying hardware, we developed a **Synthetic Data Generator** in Python:
    * **Healthy State:** 50Hz baseline motor frequency + Gaussian noise.
    * **Faulty State:** 50Hz carrier wave + 10Hz "imbalance" frequency + high-variance noise.

### 2. Digital Signal Processing (DSP)
We manually implemented the following to extract features:
* **Fast Fourier Transform (FFT):** Translating time-domain vibrations into the frequency domain.
* **Spectral Analysis:** Identifying harmonic peaks that serve as the "vibration fingerprint" for mechanical failure.

### 3. Custom Neural Network Architecture
We designed a **1D Convolutional Neural Network (1D-CNN)**. This is superior to standard networks for "Sequence Data" as it scans for patterns within the vibration waves.

**Model Summary:**
| Layer | Type | Output Shape | Param # |
| :--- | :--- | :--- | :--- |
| 1 | Conv1D | (None, 98, 16) | 160 |
| 2 | MaxPooling1D | (None, 49, 16) | 0 |
| 3 | Conv1D | (None, 47, 32) | 1,568 |
| 4 | GlobalAvgPool | (None, 32) | 0 |
| 5 | Dense (Output) | (None, 2) | 66 |

* **Total Parameters:** 1,794 (~7 KB) — Optimized for ESP32 SRAM.

### 4. TinyML Optimization (Quantization)
To deploy the Python model onto the ESP32, we performed:
1. **TFLite Conversion:** Shrinking the Keras model.
2. **INT8 Quantization:** Reducing model size for 8-bit microcontroller efficiency.
3. **C++ Header Export:** Using `xxd` to generate `uav_model.h` for direct memory mapping.

---

## 📁 Repository Structure
* `UAV_Guardian_Research.ipynb` - The primary research notebook.
* `uav_model.h` - The compiled AI "brain" for the ESP32.
* `healthy.csv` / `faulty.csv` - Synthetic datasets for validation.

## 🚀 Future Scope
* **Phase 3:** Wireless "Red Alert" telemetry via **MQTT**.
* **Phase 4:** **InfluxDB & Grafana** dashboard for historical health analytics.
