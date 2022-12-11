import numpy as np
import matplotlib.pyplot as plt
import os
import cv2
import random
import pickle

# prepare data
input_dir = "test_photos"
categories = ["true", "false"]

# training data
training_data = []
IMG_SIZE = 500

def create_training_data():
    for category in categories:
        path = os.path.join(input_dir, category)
        class_num = categories.index(category)
        for img in os.listdir(path):
            try:
                img_array = cv2.imread(os.path.join(path, img), cv2.IMREAD_GRAYSCALE)
                new_array = cv2.resize(img_array, (IMG_SIZE, IMG_SIZE))
                training_data.append([new_array, class_num])
            except Exception as e:
                pass


create_training_data()
# print(len(training_data))

random.shuffle(training_data)
# print(training_data[5][1])
x = []
y = []

for features, label in training_data:
    x.append(features)
    y.append(label)
x = np.array(x).reshape(-1, IMG_SIZE, IMG_SIZE, 1)
y = np.array(y)

pickle_out = open("X.pickle", "wb")
pickle.dump(x, pickle_out)
pickle_out.close()

pickle_out = open("y.pickle", "wb")
pickle.dump(y, pickle_out)
pickle_out.close()

pickle_in = open("X.pickle", "rb")
x = pickle.load(pickle_in)


