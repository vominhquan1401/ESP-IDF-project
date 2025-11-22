# 📡 Hệ thống IoT Quan Trắc Cảm Biến – ESP32 (WiFi + MQTT)

Dự án triển khai một hệ thống IoT sử dụng **ESP32**, với các chức năng:

- Đọc dữ liệu từ **tối thiểu 2 cảm biến** (DHT20 – nhiệt độ/độ ẩm, Light Sensor – ánh sáng).
- Gửi dữ liệu thời gian thực về **MQTT Server (Adafruit IO)** thông qua WiFi.
- Tích hợp **webserver cấu hình WiFi** (SSID/Password) ngay trên ESP32.
- Cho phép người dùng thay đổi thông số thiết bị từ xa: WiFi mới, chu kỳ gửi dữ liệu.

---

## 🚀 Tính năng chính

### 🔹 1. Đọc dữ liệu cảm biến
- DHT20 – nhiệt độ & độ ẩm  
- Light Sensor – cường độ ánh sáng  
- Lọc nhiễu với thuật toán loại bỏ giá trị min/max.

### 🔹 2. Gửi dữ liệu qua MQTT
Dữ liệu gửi dạng JSON đến các feed:
- Temperature  
- Humidity  
- Light  

```json
{
  "value": 27.51,
  "created_at": 1713856000
}
```

### 🔹 3. Webserver cấu hình WiFi
ESP32 tự bật Access Point khi chưa có WiFi trong NVS:

- **SSID:** `ESP32_WiFi_Lab`  
- **Password:** `12345678`

Truy cập bằng trình duyệt:  
👉 http://192.168.4.1

### 🔹 4. Nhận cấu hình từ MQTT
Cho phép thay đổi:
- WiFi SSID  
- WiFi Password  
- Chu kỳ gửi dữ liệu (interval)

### 🔹 5. LED trạng thái
- Nháy nhanh → mất kết nối  
- Nháy chậm → đang kết nối  
- Sáng liên tục → đã kết nối  
- LED bật đặc biệt khi vào chế độ Web Config

---

# 📁 Cấu trúc thư mục

```
ESP-IDF-project/
│── main/
│   ├── wifi_manager.c/h
│   ├── wifi_config_portal.c/h
│   ├── mqtt_module.c/h
│   ├── dht20.c/h
│   ├── light_sensor.c/h
│   ├── button.c/h
│   ├── data_handle.c/h
│   ├── wifi.c/h
│   ├── nvs_manager.c/h
│   ├── config.h
│   └── main.c
│
├── CMakeLists.txt
├── sdkconfig
└── README.md
```

---

# 🛠 Yêu cầu phần mềm

Cài đặt **ESP-IDF**:

```bash
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
install.sh        # Linux/macOS
install.bat       # Windows
```

Kích hoạt môi trường:

```bash
source export.sh
```

---

# 🔧 Cấu hình dự án

## 1. Thêm API Key Adafruit IO
Mở file:

```
main/secrects.h
```

Điền:

```c
#define USERNAME  "your_adafruit_username"
#define KEY       "your_adafruit_aio_key"
```

---

# ⚙️ Build & Flash Firmware

### 1. Cấu hình:
```bash
idf.py menuconfig
```

### 2. Build:
```bash
idf.py build
```

### 3. Flash:
```bash
idf.py -p COM3 flash
```

### 4. Serial monitor:
```bash
idf.py -p COM3 monitor
```

---

# ▶️ Cách chạy và sử dụng

## 1️⃣ Cấu hình WiFi lần đầu bằng Webserver
ESP32 sẽ bật AP mode nếu chưa có WiFi trong NVS:

- Kết nối điện thoại tới:  
  **SSID:** `ESP32_WiFi_Lab`  
  **Password:** `12345678`

- Truy cập: http://192.168.4.1  
- Nhập SSID/Password WiFi thật  
- Thiết bị tự động chuyển sang STA mode và kết nối

---

## 2️⃣ Thiết bị hoạt động bình thường
- Kết nối WiFi  
- Đồng bộ thời gian  
- Kết nối MQTT  
- Đọc dữ liệu sensor → gửi lên server theo chu kỳ

---

## 3️⃣ Điều chỉnh cấu hình qua MQTT
Các topic subscriber:

```
yolofarm.farm-wifi-ssid
yolofarm.farm-wifi-password
yolofarm.farm-send-interval
```

Ví dụ gửi interval mới:

```
20000
```

ESP32 sẽ cập nhật tự động.

---

## 4️⃣ Sử dụng nút nhấn để mở Web Config Portal
Nút nhấn (BUTTON_PIN) giúp mở lại chế độ AP để cấu hình WiFi mới.

---

# 📌 Ghi chú kỹ thuật

- Gồm các task chính:  
  WiFi Setup, WiFi Handler, LED Control, Sensor Read, Data Manager, MQTT Event  
- NVS lưu SSID/PASS  
- Tự động reconnect WiFi & MQTT  
- Queue buffer dữ liệu cảm biến

---
