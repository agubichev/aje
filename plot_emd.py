import matplotlib.pyplot as plt
import sys


f=open(sys.argv[1])

emd=[]
freq=[]

for line in f.readlines():
	l=line.split()
	print l
	freq.append(float(l[0]))
	emd.append(float(l[1]))

plt.plot(freq,emd,'ro')
plt.xlabel('Topic Frequency')
plt.ylabel('EMD')
plt.title(sys.argv[2])

plt.show()
