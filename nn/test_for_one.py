import cv2
import tensorflow as tf

categories = ['false', 'true']


def prepare(filepath):
    IMG_SIZE = 500
    img_array = cv2.imread(filepath, cv2.IMREAD_GRAYSCALE)
    new_array = cv2.resize(img_array, (IMG_SIZE, IMG_SIZE))
    return new_array.reshape(-1, IMG_SIZE, IMG_SIZE, 1)


model = tf.keras.models.load_model('detector.model')
prediction = model.predict([prepare('test_photos/true/photo1670670610.jpeg')])
print(categories[int(prediction[0][0])])
