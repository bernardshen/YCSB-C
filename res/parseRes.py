import sys
import json

def genFnamePre(db, comm, table):
    if db == "myKV":
        if comm == "2S":
            db = db + comm
        return "{}_{}".format(db, table)
    return "ycsbc"

def parseFile(fname):
    f = open(fname, 'r')
    lines = f.readlines()
    retList = []
    for l in lines:
        if l[0] == '#':
            continue
        tput = float(l.split('\t')[-1])
        retList.append(tput)
    print(retList)
    assert(len(retList) == 5)
    return retList

def parseLatFile(fname):
    f = open(fname, 'r')
    lines = f.readlines()
    retList = []
    for l in lines:
        if l[0] == '#':
            continue
        if l[0].isalpha():
            continue
        lat = float(l[:-1])
        retList.append(lat)
    assert(len(retList) == 5)
    return retList

# threadNumList = [1,2,3,4,5,6,7,8,9,10,15,20,25,30,35,40,45,50]
threadNumList = [1,2,3,4,5,6,7,8,9,10]
workloadNames = ['A', 'B', 'C', 'D', 'E']

if __name__ == "__main__":
    db = sys.argv[1]
    comm = None
    table = None

    if db == "myKV":
        comm = sys.argv[2]
        table = sys.argv[3]

    fnamePre = genFnamePre(db, comm, table)
    tputList = []
    for i in threadNumList:
        fname = fnamePre + "_t{}_lat.output".format(i)
        tmpList = parseLatFile(fname)
        tputList.append(tmpList)
    
    tmpDict = {}
    for i, wl in enumerate(workloadNames):
        tmpDict[wl] = {
            'tput': [l[i] for l in tputList],
            'tnum': threadNumList,
        }
    with open('parsed/' + fnamePre + '_lat.json', 'w') as f:
        json.dump(tmpDict, f, indent=2)