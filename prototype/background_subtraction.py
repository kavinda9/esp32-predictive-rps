import cv2
import numpy as np

cap = cv2.VideoCapture(0)

# Let the camera adjust exposure/white balance for a couple seconds
for i in range(30):
    ret, frame = cap.read()

print("Capturing background... make sure your hand is NOT in frame")
cv2.waitKey(2000)

ret, background = cap.read()
background = cv2.flip(background, 1)
background_gray = cv2.cvtColor(background, cv2.COLOR_BGR2GRAY)
background_gray = cv2.GaussianBlur(background_gray, (21, 21), 0)

print("Background captured. Now show your hand.")

while True:
    ret, frame = cap.read()
    if not ret:
        break

    frame = cv2.flip(frame, 1)
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray = cv2.GaussianBlur(gray, (21, 21), 0)

    # Difference between current frame and background
    diff = cv2.absdiff(background_gray, gray)
    _, mask = cv2.threshold(diff, 30, 255, cv2.THRESH_BINARY)

    # Clean up noise
    mask = cv2.erode(mask, None, iterations=2)
    mask = cv2.dilate(mask, None, iterations=2)

    cv2.imshow("Camera Feed", frame)
    cv2.imshow("Hand Mask", mask)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()