import numpy as np
import tensorflow as tf
from tensorflow import keras
from keras.models import Sequential
from keras.layers import Conv2D, MaxPooling2D, Dense, Flatten, Dropout
from sklearn.model_selection import train_test_split

# Load prepared data
X = np.load("X_data.npy")
y = np.load("y_data.npy")

print(f"Loaded X: {X.shape}, y: {y.shape}")

# Split into train/test sets (80% train, 20% test)
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

print(f"Train set: {X_train.shape[0]} images")
print(f"Test set: {X_test.shape[0]} images")

# Build a small CNN
model = Sequential([
    Conv2D(16, (3, 3), activation='relu', input_shape=(96, 96, 1)),
    MaxPooling2D((2, 2)),
    Dropout(0.2),

    Conv2D(32, (3, 3), activation='relu'),
    MaxPooling2D((2, 2)),
    Dropout(0.2),

    Conv2D(32, (3, 3), activation='relu'),
    MaxPooling2D((2, 2)),

    Flatten(),
    Dense(64, activation='relu'),
    Dense(2, activation='softmax')  # 2 classes: rock, paper
])

model.compile(
    optimizer='adam',
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

model.summary()

# Train
history = model.fit(
    X_train, y_train,
    epochs=15,
    batch_size=8,
    validation_data=(X_test, y_test)
)

# Final evaluation
test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
print(f"\nFinal test accuracy: {test_acc*100:.2f}%")

# Save the trained model
model.save("rps_model.h5")
print("Model saved as rps_model.h5")