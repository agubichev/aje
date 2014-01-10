import requests
import MySQLdb
import random
from sets import Set

db = MySQLdb.connect(host="localhost",user="root",db="alj_tagme")

cur=db.cursor()

cur.execute("select id,article from url_text")
url="http://tagme.di.unipi.it/tag"
results = cur.fetchall()

data_tagme={}

i = 0
while i<100:
	i += 1
	row = results[random.randint(0,len(results)-1)]
	data={"key":"andrey2014", "text":row[1]}
	#print "LEN", len(row[1])
	response = requests.post(url, params=data)
	print row[1]
	print len(row[1])
	print response.text
	ann = response.json()["annotations"]
	ann_threshold = Set()
	for a in ann:
		if float(a["rho"]) > 0.1:
			ann_threshold.add(a['title'])
	ann_results=[]
	for a in ann_threshold:
		ann_results.append(a)

	print row[0], ann_results

	data_tagme[int(row[0])] = ann_results

