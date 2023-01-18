import cv2
import tensorflow as tf

categories = ['false', 'true']


def prepare(filepath):
    IMG_SIZE = 50
    img_array = cv2.imread(filepath)
    new_array = cv2.resize(img_array, (IMG_SIZE, IMG_SIZE))
    return new_array.reshape(-1, IMG_SIZE, IMG_SIZE, 3)


model = tf.keras.models.load_model('../Arduino/detector.model')
prediction = model.predict([prepare('test/false/66.jpg')])
print(categories[int(prediction[0][0])])
prediction1 = model.predict([prepare('test/true/11.jpg')])
print(categories[int(prediction1[0][0])])
