import matplotlib
import matplotlib.pyplot as plt
import numpy as np 
from matplotlib.ticker import FuncFormatter
	
font = {'family' : 'normal',
        'weight' : 'normal',
        'size'   : 15}

matplotlib.rc('font', **font)

def to_percent(y, position):
    # Ignore the passed in position. This has the effect of scaling the default
    # tick locations.
    s = str(100 * y)

    # The percent symbol needs escaping in latex
    if matplotlib.rcParams['text.usetex'] is True:
        return s + r'$\%$'
    else:
        return s + '%'

formatter = FuncFormatter(to_percent)
       
def rrd2():
	data = 0
	l = 0
	for i in xrange(35):
		filename = 'rrd' + str(i)
		try:
			ifile = open(filename)
			inlist = ifile.readlines()
			inlist = [float(ch.strip('\n')) if float(ch.strip('\n')) < 100000000 else 0 for ch in inlist]
			data += sum(inlist)
			l += len(inlist)
		except:
			pass
	return data / l


def rrd(filename):
	ifile = open(filename, 'r')
	inlist = ifile.readlines()
	ifile.close()
	res = 0
	i = 0
	for ch in inlist:
		ch.strip('\n')
		res += float(ch)
		i += 1
	return res / i

def time_based_curve(filename):
	x = []
	y = []
	ifile = open(filename, "r")
	inlist = ifile.readlines()
	ifile.close()
	L = len(inlist)
	for i in xrange(L):
		data = [float(ch.strip('\n')) for ch in inlist[i].split(' ')]
		x.append(data[0])
		y.append(data[1])
	return x, y

def contribution(file_name):
	ifile = open(file_name, "r")
	inlist = ifile.readlines()
	ifile.close()
	L = len(inlist)


	x = range(90)
	y = [float(ch.strip('\n')) for ch in inlist]

	return x, y

def data_from_file(name):
	ifile = open(name, "r")
	inlist = ifile.readlines()
	ifile.close()
	L = len(inlist)

	data_dict = {}
	sample = 0
	for i in xrange(L):
		data = [float(ch.strip('\n')) for ch in inlist[i].split(' ')]
		s = data[0]
		u = data[1]
		sample += 1.0
		if s in data_dict:
			data_dict[s][0] += u
			data_dict[s][1] += 1
		else:
			data_dict[s] = []
			data_dict[s].append(u)
			data_dict[s].append(1)
		
	result = sorted(data_dict.iteritems(), key = lambda data_dict : data_dict[0])

	size = []
	usage = []
	cdf = []
	for ele in result:
		size.append(ele[0])
		usage.append(ele[1][0] / ele[1][1])
		cdf.append(ele[1][1] / sample)
	x = np.array(size)
	y = np.array(usage)
	z = np.array(cdf)
	return x, y, z

def average(y):
	result = []
	L = len(y)
	s = 0
	for i in xrange(L):
		s += y[i]
		result.append(s / (i + 1))
	# s = 0
	# for i in range(L)[::-1]:
	# 	s += y[i]
	# 	result[i] = (result[i] + s / (L - i)) / 2
	return result

def ave(y):
	L = len(y)
	result = [0] * L
	for i in xrange(2, L - 2, 1):
		result[i] = (y[i-2] + y[i-1] + y[i] + y[i+1] + y[i+2]) / 5
	return result
 
def lerp(x, y, t):
	ttt = t * t * t
	t = ttt * (3.0 * t * (2.0 * t - 5.0) + 10.0)
	return (1 - t) * x + t * y

def smooth(x, y, step):
	result_x = []
	result_y = []
	L = len(x)
	for i in xrange(L - 1):
		left = x[i]
		right = x[i + 1]
		xx = left
		while xx < right:
			result_x.append(xx)
			res = lerp(y[i], y[i + 1], (xx - left) / (right - left))
			result_y.append(res)
			xx += step
	return result_x, result_y

'''
x1, y1, z1 = data_from_file("1_30_high")
x2, y2, z2 = data_from_file("1_30_normal")
x3, y3, z3 = data_from_file("1_30_low")

#plt.suptitle("1 super nodes and 29 normal nodes")

y1 = average(y1)
y2 = average(y2)
y3 = average(y3)

plt.plot(x1, y1, color = "green")
plt.scatter(x1, y1, marker = '.', color = "green", label = 'aai ~ P(10), 1/30 super node')
plt.plot(x2, y2, color = "red")
plt.scatter(x2, y2, marker = 'o', color = "red", label = 'aai ~ P(300), 1/30 super node')
plt.plot(x3, y3, color = "blue")
plt.scatter(x3, y3, marker = '*', color = "blue", label = 'aai ~ P(3000), 1/30 super node')
plt.grid(True)
plt.legend()
plt.axis([0, 30, 0, 0.3])
plt.ylabel("Cluster usage")
plt.xlabel("Cluster size")
plt.grid(True)
plt.legend(loc = 2, fontsize = 15)
plt.show()
'''

'''
x1, y1, z1 = data_from_file("1_30_high")
x2, y2, z2 = data_from_file("5_30_high")

#plt.suptitle("1 super nodes and 29 normal nodes")

y1 = average(y1)
y2 = average(y2)

plt.plot(x1, y1, color = "green")
plt.scatter(x1, y1, marker = '*', color = "green", label = 'aai ~ P(10), 1/30 super node')
plt.plot(x2, y2, color = "red")
plt.scatter(x2, y2, marker = 'o', color = "red", label = 'aai ~ P(10), 5/30 super node')
plt.grid(True)
plt.legend()
plt.axis([0, 30, 0, 0.3])
plt.ylabel("Cluster usage")
plt.xlabel("Cluster size")
plt.grid(True)
plt.legend(loc = 2, fontsize = 15)
plt.show()
'''
'''
x1, y1, z1 = data_from_file('1_30_rwp_su')
x2, y2, z2 = data_from_file('5_30_high')
#plt.plot(x1, z1, color = 'red')
xx1, zz1 = smooth(x1, z1, 0.1)
plt.plot(xx1, zz1, label = 'Random Way Point', color = 'blue')
xx2, zz2 = smooth(x2, z2, 0.1)
plt.plot(xx2, zz2, label = "Social Based Mobility", color = 'red')
plt.xlabel('size')
plt.legend()
plt.setp(plt.gca().get_legend().get_texts(), fontsize='20')
#plt.title("size propability distribution")
plt.show()'''

'''
x, size = time_based_curve('_size')
x, cap = time_based_curve('_cap')
x, usage = time_based_curve('_usage')
usage = average(usage)

plt.figure(figsize=(8,12))
plt.subplot(311)
plt.axis([0, 90, 0, 50])
plt.plot(x, size, linewidth=2)
plt.legend(loc = 2)
plt.xlabel('Time (s)')
plt.ylabel('Cluster size')

plt.subplot(312)
plt.axis([0, 90, 0, 50])
plt.plot(x, cap, linewidth=2)
plt.legend(loc = 2)
plt.xlabel('Time (s)')
plt.ylabel('Cluster capacity')

plt.subplot(313)
plt.axis([0, 90, 0.02, 0.1])
plt.plot(x, usage, linewidth=2)
plt.legend(loc = 2)
plt.gca().set_yticks(np.linspace(0., 0.1, 6))  
plt.gca().set_yticklabels( ('0', '0.02', '0.04', '0.06', '0.08', '0.1')) 
plt.xlabel('Time (s)')
plt.ylabel('Cluster usage')
plt.show()
'''

'''
x1, y1, z1 = data_from_file('su')
x2, y2, z2 = data_from_file('5_30_high')
x1, z1 = smooth(x1, z1, 0.1)
x2, z2 = smooth(x2, z2, 0.1)
plt.plot(x1, z1, color = 'blue', label = 'Threshold-M= 0.1')
plt.plot(x2, z2, color = 'red', label = 'Threshold-M= 0.8')
plt.legend()
plt.gca().yaxis.set_major_formatter(formatter)
plt.setp(plt.gca().get_legend().get_texts(), fontsize='20')
plt.xlabel("Cluster size")
plt.ylabel("Distribution")
plt.show()
'''

'''
#plt.title("Cluster Size Propability Distribution")
x1, y1, z1 = data_from_file('1_30_normal')
x2, y2, z2 = data_from_file('1_30_rwp_su')
x1, z1 = smooth(x1, z1, 0.1)
x2, z2 = smooth(x2, z2, 0.1)
ax = plt.plot(x1, z1, color = 'blue', label = 'SCB-model')
plt.plot(x2, z2, color = 'red', label = 'RWP-model')
plt.legend()
plt.gca().yaxis.set_major_formatter(formatter)
plt.setp(plt.gca().get_legend().get_texts(), fontsize='20')
plt.xlabel("Cluster size")
plt.ylabel("Distribution")
plt.show()
'''

'''
#plt.title('Re-running Delay')
w = 0.07
x1 = [0.05, 0.25, 0.45]
x2 = [x + w for x in x1]
y1 = [0.123272221763, 0.062967866548, 0.0348147285714]
y2 = [0.112467666244, 0.0585150729852, 0.0275402042184]
plt.xlabel('Move Speed')
plt.ylabel('RRD(s)')
plt.xticks(x2, ('high speed', 'normal speed', 'low speed'))
rects1 = plt.bar(x1, y1, width = w, color = 'red', label = 'core number = 1')
rects2 = plt.bar(x2, y2, width = w, color = 'blue', label = 'core number = 2')
plt.tight_layout()
def autolabel(rects):
    for rect in rects:
        height = rect.get_height()
        plt.text(rect.get_x() + w / 5, 1.03 * height, '%s' % '%.3f' % float(height)) 
autolabel(rects1)
autolabel(rects2)
plt.legend()
ax = plt.axes()        
ax.yaxis.grid() # horizontal lines
plt.show()
'''

'''
plt.axis([0, 90, -3, 3])
x1, y1 = contribution("con_bad")
x1, y1 = smooth(x1, y1, 0.1)
x2, y2 = contribution("con_normal")
x2, y2 = smooth(x2, y2, 0.1)
x3, y3 = contribution("con_super")
x3, y3 = smooth(x3, y3, 0.1)
plt.plot(x1, y1, linewidth = 2, label = "selfish node")
plt.plot(x2, y2, linewidth = 2, label = "normal node")
plt.plot(x3, y3, linewidth = 2, label = "super node")
plt.legend()
plt.ylabel("NRCSD")
plt.xlabel("Time(s)")
ax = plt.axes()        
ax.yaxis.grid() # horizontal lines
plt.show()
'''

plt.axis([0, 110, 0, 600])
x = [10, 30, 50, 80, 100]
y0 = \
[
	95.579849601,
	135.2146495686,
	299.562232054,
	450.407352048,
	587.981243303
]
y1 = \
[
	46.9192159920,
	78.8629562059,
	119.2289596331,
	196.338969702,
	338.141898487
]
y2 = \
[
	13.858334869,
	30.2531833865,
	49.8055092474,
	64.2211779125,
	87.3750716281
]
plt.plot(x, y0, marker = '*', label = 'handoff time = 0ms')
plt.plot(x, y1, marker = 'o', label = 'handoff time = 10ms')
plt.plot(x, y2, marker = '^', label = 'handoff time = 100ms')
plt.grid(True)
plt.legend(loc = 2)
plt.ylabel("RRD(ms)")
plt.xlabel("Method load(instructions)")
plt.show()
