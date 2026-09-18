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
        img = cv2.imread(f, cv2.IMREAD_GRAYSCALE)  # load directly as grayscale
        if img is None:
            print(f"Warning: could not load {f}")
            continue
        img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
        images.append(img)
        labels.append(label)

    return images, labels

# Label mapping: 0 = rock, 1 = paper
rock_images, rock_labels = load_and_process_images("images/rock", 0)
paper_images, paper_labels = load_and_process_images("images/paper", 1)

all_images = rock_images + paper_images
all_labels = rock_labels + paper_labels

X = np.array(all_images, dtype=np.float32) / 255.0  # normalize to 0-1
X = X.reshape(-1, IMG_SIZE, IMG_SIZE, 1)  # add channel dimension
y = np.array(all_labels, dtype=np.int32)

print(f"Total images: {len(X)}")
print(f"X shape: {X.shape}")
print(f"y shape: {y.shape}")
print(f"Rock count: {np.sum(y == 0)}, Paper count: {np.sum(y == 1)}")

np.save("X_data.npy", X)
np.save("y_data.npy", y)
print("Saved X_data.npy and y_data.npy")