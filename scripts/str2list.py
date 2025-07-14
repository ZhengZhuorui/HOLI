import json
s = input()
print(s)
l = s.split(" ")
x = json.dumps(l[1:])
print(x)
