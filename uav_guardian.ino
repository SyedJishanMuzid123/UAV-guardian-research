#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include <MPU6050_tockn.h>
#include <Wire.h>
#include "uav_model.h"
#include <WiFi.h>
#include <PubSubClient.h>

// --- 1. NETWORK CONFIG ---
const char* ssid = "Airtel_EL_bicho_2.4ghz";
const char* password = "air51991";
// Use a Public Broker if 192.168.1.22 (Local) gives rc=-2
//const char* mqtt_server = "broker.hivemq.com"; 
const char* mqtt_server = "192.168.1.22";
WiFiClient espClient;
PubSubClient client(espClient);

// --- 2. AI GLOBALS ---
const int kTensorArenaSize = 60 * 1024; 
uint8_t* tensor_arena = nullptr;

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
MPU6050 mpu6050(Wire);

void setup_wifi() {
    delay(10);
    Serial.println("\nConnecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Unique ID for the broker
        if (client.connect("UAV_Guardian_AEC_Final")) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" (Check Firewall/Port 1883)");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Allocate memory for AI
    tensor_arena = (uint8_t*)malloc(kTensorArenaSize);
    if (tensor_arena == nullptr) {
        while(1) { Serial.println("Arena Allocation Failed!"); delay(1000); }
    }

    setup_wifi();
    client.setServer(mqtt_server, 1883);

    Wire.begin(); 
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true); 

    static tflite::MicroErrorReporter micro_error_reporter;
    model = tflite::GetModel(uav_model_tflite);
    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        while (1) { Serial.println("Tensor Allocation Failed!"); delay(1000); }
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("--- UAV-GUARDIAN AI ONLINE ---");
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();

    // Data Collection: Raw 3-axis input (100 samples x 3 axes = 300 inputs)
    for (int i = 0; i < 100; i++) {
        mpu6050.update();
        if (input != nullptr) {
            input->data.f[i * 3]     = mpu6050.getAccX();
            input->data.f[i * 3 + 1] = mpu6050.getAccY();
            input->data.f[i * 3 + 2] = mpu6050.getAccZ();
        }
        delay(10); 
    }

    // Run AI Inference
    if (interpreter->Invoke() != kTfLiteOk) return;

    float result = output->data.f[0]; // Sigmoid output (0.0 to 1.0)

    Serial.print("Vibration Score: "); Serial.println(result, 4);

    // Send Alert based on Threshold
    String status = (result > 0.80) ? "ANOMALY" : "HEALTHY";
    String payload = "{\"val\":" + String(result, 4) + ", \"status\":\"" + status + "\"}";
    
    client.publish("uav/guardian/health", payload.c_str());

    if (result > 0.80) Serial.println("🚨 ALERT: ANOMALY DETECTED!");
    
    delay(500); 
}