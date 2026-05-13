#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"

#include <MPU6050_tockn.h>
#include <Wire.h>
#include "uav_model.h"

// --- 1. THE ARENA: Defined exactly once with 16-byte alignment ---
// Using 90KB to stay within the ESP32's primary RAM segment limits
// 80KB is the safe zone. 
// It clears the 2.8KB overflow and leaves room for system globals.
const int kTensorArenaSize = 80 * 1024; 
uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)));

// --- 2. TFLite Globals ---
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

MPU6050 mpu6050(Wire);

void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("--- UAV-GUARDIAN: AEC STABLE DEPLOYMENT ---");

    // 3. Diagnostic: Check for memory fragmentation
    Serial.printf("Total Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest Contiguous Block: %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    Wire.begin(); 
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true); 

    // 4. Setup Error Reporting & Model
    static tflite::MicroErrorReporter micro_error_reporter;
    tflite::ErrorReporter* error_reporter = &micro_error_reporter;

    model = tflite::GetModel(uav_model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("Model version mismatch!");
        while (1);
    }

    // 5. Opcode Resolver: Loads everything including EXPAND_DIMS
    static tflite::AllOpsResolver resolver;

    // 6. Initialize Interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    Serial.println("Attempting to Allocate Tensors...");
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("CRITICAL: Allocation Failed! Check logs above.");
        while (1) delay(100);
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
    Serial.println("--- AI ENGINE ONLINE: READY ---");
}

void loop() {
    // Collect 100 samples (X, Y, Z) = 300 inputs
    for (int i = 0; i < 100; i++) {
        mpu6050.update();
        if (input != nullptr) {
            input->data.f[i * 3]     = mpu6050.getAccX();
            input->data.f[i * 3 + 1] = mpu6050.getAccY();
            input->data.f[i * 3 + 2] = mpu6050.getAccZ();
        }
        delay(10); 
    }

    // Run the 1D-CNN Inference
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Inference execution failed!");
        return;
    }

    // Healthy (0) vs Faulty (1)
    float healthy = output->data.f[0];
    float faulty  = output->data.f[1];

    Serial.print("H: "); Serial.print(healthy * 100, 1); Serial.print("% | ");
    Serial.print("F: "); Serial.print(faulty * 100, 1); Serial.println("%");

    if (faulty > 0.85) {
        Serial.println("🚨 ALERT: ANOMALY DETECTED!");
    }
    
    delay(500); 
}