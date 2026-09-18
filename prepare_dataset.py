import cv2
import numpy as np
import os
import glob

IMG_SIZE = 96

def load_and_process_images(folder, label):
    images = []
    labels = []
    files = glob.glob(os.path.join(folder, "*.jpeg")) + glob.glob(os.path.join(folder, "*.jpg"))

    print(f"Found {len(files)} files in {folder}")

    for f in files:
        img = cv2.imread(f, cv2.IMREAD_GRAYSCALE)
        if img is None:
            print(f"Warning: could not load {f}")
            continue
        img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
        images.append(img)
        labels.append(label)

    return images, labels

# Label mapping: 0 = rock, 1 = paper, 2 = scissors
rock_images, rock_labels = load_and_process_images("images/rock", 0)
paper_images, paper_labels = load_and_process_images("images/paper", 1)
scissors_images, scissors_labels = load_and_process_images("images/scissors", 2)

all_images = rock_images + paper_images + scissors_images
all_labels = rock_labels + paper_labels + scissors_labels

X = np.array(all_images, dtype=np.float32) / 255.0
X = X.reshape(-1, IMG_SIZE, IMG_SIZE, 1)
y = np.array(all_labels, dtype=np.int32)

print(f"Total images: {len(X)}")
print(f"X shape: {X.shape}")
print(f"y shape: {y.shape}")
print(f"Rock count: {np.sum(y == 0)}, Paper count: {np.sum(y == 1)}, Scissors count: {np.sum(y == 2)}")

np.save("X_data.npy", X)
np.save("y_data.npy", y)
print("Saved X_data.npy and y_data.npy")