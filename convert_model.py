import tensorflow as tf
import numpy as np

# Load the trained model
model = tf.keras.models.load_model("rps_model.h5")

# Load training data again — needed for quantization calibration
X = np.load("X_data.npy")

# Representative dataset generator (used to calibrate int8 quantization)
def representative_dataset():
    for i in range(min(100, len(X))):
        yield [X[i:i+1].astype(np.float32)]

# Convert to TFLite with full integer quantization
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

tflite_model = converter.convert()

with open("rps_model.tflite", "wb") as f:
    f.write(tflite_model)

print(f"Saved rps_model.tflite ({len(tflite_model)} bytes)")

# Also convert to a C array for embedding in firmware
def convert_to_c_array(tflite_path, output_path, var_name="rps_model"):
    with open(tflite_path, "rb") as f:
        data = f.read()

    with open(output_path, "w") as f:
        f.write(f"#ifndef {var_name.upper()}_H\n")
        f.write(f"#define {var_name.upper()}_H\n\n")
        f.write(f"const unsigned char {var_name}[] = {{\n")
        for i, byte in enumerate(data):
            f.write(f"0x{byte:02x}, ")
            if (i + 1) % 12 == 0:
                f.write("\n")
        f.write(f"\n}};\n")
        f.write(f"const unsigned int {var_name}_len = {len(data)};\n\n")
        f.write(f"#endif\n")

    print(f"Saved {output_path}")

convert_to_c_array("rps_model.tflite", "rps_model.h", "rps_model")