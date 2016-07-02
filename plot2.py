# -*- coding: UTF-8 -*-   import matplotlib
import matplotlib
import matplotlib.pyplot as plt
import numpy as np 
import math

font = {'family' : 'normal',
        'weight' : 'normal',
        'size'   : 15}

matplotlib.rc('font', **font)

lbd = 0.01
r = 100
size = 20

M = [x for x in xrange(1, size)]
y = [math.pow(lbd * r * r * 3.14, x) for x in M]
z = [math.pow(lbd * r * r * 10 * 3.14, x) for x in M]
ax = plt.axes() 
plt.axes().semilogy(M, y)
plt.axes().semilogy(M, z)
plt.plot(M, y, linewidth=2, label = "Communication range = 10m, lambda = 0.01")
plt.plot(M, z, linewidth=2, label = "Communication range = 100m, lambda = 0.1")
plt.xlabel("Number of components")
plt.ylabel("Min reduction")
plt.legend(loc = 2)
plt.setp(plt.gca().get_legend().get_texts(), fontsize='15')
ax.yaxis.grid() # horizontal lines
plt.show()