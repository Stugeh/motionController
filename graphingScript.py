
from itertools import count
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

plt.style.use('fivethirtyeight')

x_vals = []
y_vals = []

index = count()


def animate(i):
    try:
        data = pd.read_csv('/Debug/data.csv')



        x = data['time']
        ax = data['ax']
        ay = data['ay']
        az = data['az']
        gx = data['gx']
        gy = data['gy']
        gz = data['gz']

        plt.cla()

        plt.plot(x, ax, label='ax')
        plt.plot(x, ay, label='ay')
        plt.plot(x, az, label='az')
        plt.plot(x, gx, label='gx')
        plt.plot(x, gy, label='gy')
        plt.plot(x, gz, label='gz')

        plt.legend(loc='upper left')
        plt.tight_layout()
    except:
        pass


ani = FuncAnimation(plt.gcf(), animate, interval=1000)

plt.tight_layout()
plt.show()
