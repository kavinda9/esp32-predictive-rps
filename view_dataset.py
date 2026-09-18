import cv2
import numpy as np
import os
import glob

def load_pgm(filepath):
    return cv2.imread(filepath, cv2.IMREAD_GRAYSCALE)

def view_session(folder, session_num):
    pattern = os.path.join(folder, f"session{session_num:02d}_frame*.pgm")
    files = sorted(glob.glob(pattern))

    if not files:
        print(f"No frames found for session {session_num:02d} in {folder}")
        return

    print(f"Found {len(files)} frames for session {session_num:02d}")

    # Load all frames, arrange in a grid
    frames = [load_pgm(f) for f in files]
    frames = [f for f in frames if f is not None]

    cols = 8
    rows = (len(frames) + cols - 1) // cols
    h, w = frames[0].shape

    grid = np.zeros((rows * h, cols * w), dtype=np.uint8)
    for i, frame in enumerate(frames):
        r, c = divmod(i, cols)
        grid[r*h:(r+1)*h, c*w:(c+1)*w] = frame

    cv2.imshow(f"Session {session_num:02d} ({len(frames)} frames)", grid)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

if __name__ == "__main__":
    folder = input("Folder path (e.g. ai/dataset/rock): ").strip()
    session_num = int(input("Session number to view (e.g. 10): ").strip())
    view_session(folder, session_num)