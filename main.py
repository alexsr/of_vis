import os
import numpy as np
import matplotlib.pyplot as plt

root_dir = '../11_back'  # root dir of the evaluation case
wss = []
labels = []

ticks = range(0, 100, 10)

for subdir in os.listdir(root_dir):
    dir_path = os.path.join(root_dir, subdir)
    labels.append(subdir)
    for f in os.listdir(dir_path):
        if "wallShearStress" in f:  # change type of variable here
            data = np.genfromtxt(os.path.join(dir_path, f))
            data = data[np.nonzero(data)]  # add * 1055 for WSS
            wss.append(data)

labels.insert(0, labels.pop())
wss.insert(0, wss.pop())

plt.rcParams.update({'font.size': 22})
fig = plt.figure(figsize=(20, 6))
plt.yticks(ticks)
plt.boxplot(wss, 0, '', labels=labels, showmeans=True, meanline=True, meanprops={"linewidth": 3.0}, medianprops={"linewidth": 2.0})
plt.axhline(wss[0].mean(), color='b', linestyle='dashed')
plt.ylabel("Wall shear stress in Pa")
plt.show()
