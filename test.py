import os
import re
import math

str_new_pkt = "New"
str_recv_pkt = "Received"
str_latop = "latop"

pkt_total = 0
pkt_recvd = 0

latency = []

f = open("./output_detail", "r")
f.readline()

for line in f:
	# print line.find(str_recv_pkt)
	# print line.find(str_new_pkt)
	if (line.find(str_new_pkt) == 0):
		pkt_total += 1
	if (line.find(str_recv_pkt) == 0):
		pkt_recvd += 1
	if (line.find(str_latop) == 0):
		latency.append(int(re.findall(r'\d+',line)[0]))

sum1 = 0.0
sum2 = 0.0

for e in latency:
	sum1 += e
	sum2 += e*e

print "Average latency (ms): %0.3f" % (sum1/len(latency))
print "Standard variance: %0.3f" % math.sqrt(sum1*sum1-sum2)
print "Packet Loss %%: %0.3f" % ((1-float(pkt_recvd)/pkt_total)*100)
