import cv2
import numpy as np

cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    frame = cv2.flip(frame, 1)  # mirror view, easier to work with

    # Define a region of interest (ROI) box where you'll place your hand
    roi_top, roi_bottom, roi_left, roi_right = 100, 400, 100, 400
    roi = frame[roi_top:roi_bottom, roi_left:roi_right]

    # Convert ROI to HSV color space (better for skin detection than RGB)
    hsv = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)

    # Basic skin color range (we will likely need to tune these values for your skin tone/lighting)
    lower_skin = np.array([0, 20, 70], dtype=np.uint8)
    upper_skin = np.array([20, 255, 255], dtype=np.uint8)

    mask = cv2.inRange(hsv, lower_skin, upper_skin)

    # Clean up the mask a bit (remove noise)
    mask = cv2.GaussianBlur(mask, (5, 5), 0)

    # Draw the ROI box on the main frame for reference
    cv2.rectangle(frame, (roi_left, roi_top), (roi_right, roi_bottom), (0, 255, 0), 2)

    cv2.imshow("Camera Feed", frame)
    cv2.imshow("Hand Mask", mask)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()