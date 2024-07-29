import numpy as np
import cv2

def undistort_image(image, camera_matrix, dist_coeffs):
    h, w = image.shape[:2]
    new_camera_matrix, _ = cv2.getOptimalNewCameraMatrix(camera_matrix, dist_coeffs, (w, h), 1, (w, h))

    # Undistort image
    undistorted_image = cv2.undistort(image, camera_matrix, dist_coeffs, None, new_camera_matrix)

    return undistorted_image

# Example usage
if __name__ == "__main__":
    # Load image
    image = cv2.imread('/home/rick/Datasets/SN_00198/cam0/data/727090000.png')

    # Example camera parameters (replace with your actual parameters)
    h = image.shape[0]
    w = image.shape[1]

    camera_matrix = np.array([[w* 0.965014271943, 0, w*0.5229657745016],
                              [0, h*1.2830669287, h*0.428267065753],
                              [0, 0, 1]])
    
    dist_coeffs = np.array([-0.019033517364870117, -0.01622767899713151, 0.005752409177199101, 0.002690467209823389, 0])

    # Undistort image
    undistorted_image = undistort_image(image, camera_matrix, dist_coeffs)

    # Display results
    cv2.imshow('Original Image', image)
    cv2.imshow('Undistorted Image', undistorted_image)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
