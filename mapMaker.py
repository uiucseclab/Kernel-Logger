from bs4 import BeautifulSoup

ctable = "{"
lentable = "{"
soup = BeautifulSoup(open('table.html'),'html.parser')
all_trs = soup.find_all('tr')
all_trs = all_trs[1:]
for tr in all_trs: 
    all_tds = tr.find_all('td')
    # print(all_tds[1].text + ':' + all_tds[0].text)
    val = all_tds[0].text.lower().replace(" ","")
    if len(val) > 1:
        ctable += (" \" {} \",").format(val)
        lentable += (" {},").format(len(val) + 2) #+2 for spaces infront and behind
    else:
        ctable += (" \"{}\",").format(val)
        lentable += (" {},").format(len(val))
ctable = ctable[:-1]
lentable = lentable[:-1]
ctable += "}"
lentable += "}"