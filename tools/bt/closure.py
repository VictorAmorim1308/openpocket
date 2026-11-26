import subprocess,os,sys,collections
D='/tmp/bt2'
mem={}
for sub in ['btstack','btctrler','as','cpu']:
    for f in os.listdir(os.path.join(D,sub)):
        p=os.path.join(D,sub,f)
        out=subprocess.run(['llvm-nm-18',p],capture_output=True,text=True).stdout
        d=set();u=set();w=set()
        for l in out.splitlines():
            parts=l.split()
            if len(parts)<2: continue
            t,n=parts[-2],parts[-1]
            if t=='U': u.add(n)
            elif t in 'TDBRCV': d.add(n)
            elif t in 'Ww': w.add(n)
        mem[p]=(d,u,w,os.path.getsize(p))
defs={}
for p,(d,u,w,s) in mem.items():
    for n in d: defs.setdefault(n,p)
roots=sys.argv[1:]
pulled=set();need=collections.deque(roots);seen=set()
unres=set()
while need:
    n=need.popleft()
    if n in seen: continue
    seen.add(n)
    p=defs.get(n)
    if not p: unres.add(n); continue
    if p in pulled: continue
    pulled.add(p)
    for x in mem[p][1]: need.append(x)
size=sum(mem[p][3] for p in pulled)
print("members pulled:",len(pulled),"bitcode bytes:",size)
print("by dir:",collections.Counter(p.split('/')[3] for p in pulled))
print("unresolved:",len(unres))
print(' '.join(sorted(unres)))
print("\n".join(sorted(os.path.basename(p) for p in pulled)))
