import re 
code = input()
iterator = input()
numLoops = int(input())

for i in range(numLoops):
    l = re.findall("[ \[;]" + iterator + "[ \];]", code)
    l = list(set(l))
    c = re.sub("\\" + l[0], re.sub(iterator,str(i), l[0]),code)
    # print(l)
    # print(c)

    for x in l[1:]:
        c = re.sub("\\" + x, re.sub(iterator,str(i), x),c)
    print(c)