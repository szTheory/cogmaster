#!/bin/sh
# Shared bounded quoted CSV, read as format 1 or 2, written as 2: a row names the whole run profile, one parser serves every reader. No private execution.
set -eu
export LC_ALL=C
export COMPAT_CATALOG="${COMPAT_CATALOG:-tests/compat/catalog.tsv}" COMPAT_REASONS="${COMPAT_REASONS:-tests/compat/reasons.txt}"
mode=$1; shift
for input do
  [ -f "$input" ] && [ "$(wc -c < "$input")" -le 4194304 ] || { echo 'compat: invalid ledger input' >&2; exit 2; }
done
awk -v mode="$mode" '
function bad() { print "compat: invalid ledger record " NR > "/dev/stderr"; failed=1; exit 2 }
function day(s, a,y,m,d,n,i) {
  if(s !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$/) bad(); split(s,a,"-"); y=a[1]+0; m=a[2]+0; d=a[3]+0;
  n=(m==2?28+(y%4==0 && (y%100!=0 || y%400==0)):(m==4 || m==6 || m==9 || m==11?30:31)); if(y<1970 || m<1 || m>12 || d<1 || d>n) bad();
  n=365*(y-1)+int((y-1)/4)-int((y-1)/100)+int((y-1)/400)+d; for(i=1;i<m;i++) n+=(i==2?28+(y%4==0 && (y%100!=0 || y%400==0)):(i==4 || i==6 || i==9 || i==11?30:31)); return n
}
function quote(s) { gsub(/"/,"\"\"",s); return "\"" s "\"" }
function md(s, o,i,p,e) { split("&amp; &lt; &gt; &#124; &#92; &#96; &#42; &#95; &#126; &#91; &#93; &#40; &#41; &#33; &#35;",e," "); o=""; for(i=1;i<=length(s);i++) { p=index("&<>|\\`*_~[]()!#",substr(s,i,1)); o=o (p?e[p]:substr(s,i,1)) } return o }
function put(s, x) { if(++pn[s]<=pc[s]) pb[s]=pb[s] "\n" (s>2?cell(x):x) }
function cell(k, t,w,d,j,x,p) { split(r[k,13],t,";"); sub(/^cfg:/,"",t[1]); split("video=" (r[k,3]=="gg"?"gg":"sms") ";fm;port1=pad;port2=pad;script=none",w,";"); for(j=1;j<=5;j++) if(j!=2 && t[j]!=w[j]) d=d " " substr(t[j],1,24); p=r[k,11]; sub(/:.*/,"",p); x=substr(md(r[k,2]),1,64); gsub(/[^ -~]/,"?",x)
  return "| " substr(r[k,1],1,(length(r[k,1])==40?12:40)) " | " x " | " r[k,3] "/" r[k,4] "/" r[k,5] "/" substr(r[k,6],1,32) " " t[2] " " p d " | " r[k,8] " | " r[k,10] " |" }
function hex(s, n,i,d) { n=0; s=toupper(s); for(i=1;i<=length(s);i++) { d=index("0123456789ABCDEF",substr(s,i,1))-1; if(d<0) bad(); n=n*16+d } return n }
function predicate(s, machine, region, p,n,a,b,t,frame,address,value,cfg,fm) {
  cfg="cfg:video=" (machine=="gg"?"gg":"sms") ";port1=pad;port2=pad;script=none;sha256=" empty;
  if(index(s,"cfg:")==1) { n=index(s,"/"); if(!n) bad(); cfg=substr(s,1,n-1); s=substr(s,n+1) }
  if(cfg !~ /^cfg:video=(sms|gg)(;fm=(on|off))?;port1=(pad|none|paddle|sports|lphaser);port2=(pad|none|paddle|sports|lphaser);script=[A-Za-z0-9_.-]+;sha256=[0-9a-f]+$/ || (ver==1 && index(cfg,";fm="))) bad();
  fm=(region=="japan" && machine ~ /^sms1?$/?"on":"off"); if(match(cfg,/;fm=(on|off)/)) fm=substr(cfg,RSTART+4,RLENGTH-4); else sub(/;port1=/,";fm=" fm ";port1=",cfg);
  if(fm=="on" && machine=="gg") bad();
  split(cfg,t,"sha256="); if(length(t[2])!=64 || (cfg ~ /;script=none;/ && t[2]!=empty)) bad();
  p=index(s,"@"); kind=substr(s,1,p-1); s=substr(s,p+1); n=match(s,/[:=]/); frame=n?substr(s,1,n-1):s; value=""; address="";
  if(frame !~ /^[0-9]+$/ || length(frame)>10 || frame+0<1 || frame+0>1000000) bad(); frame=sprintf("%.0f",frame+0);
  if(kind=="frame" || kind=="audio") { value=substr(s,n+1); if(!n || substr(s,n,1)!="=" || value !~ /^[0-9a-f]+$/ || length(value)!=64) bad() }
  else if(kind=="ram") { a=substr(s,n); if(a !~ /^:\$[0-9a-fA-F]+=[0-9a-fA-F]+$/) bad(); split(substr(a,3),b,"="); if(length(b[1])>4 || hex(b[1])<49152 || length(b[2])>2) bad(); address=sprintf("%04X",hex(b[1])); value=sprintf("%02X",hex(b[2])) }
  else if((kind!="boots" && kind!="mailbox") || n) bad();
  expected=value; parameter=kind "@" frame (kind=="ram"?":$" address:""); config=cfg; cap=frame;
  return cfg "/" parameter (value!=""?"=" value:"")
}
function reasons(r) { if(!loaded++) while((r=(getline line < ENVIRON["COMPAT_REASONS"]))!=0) { if(r<0) bad(); if(line !~ /^#/ && line!="") { split(line,rs," "); ok[rs[1]]=rs[2] } } }
function rel(k, s) { s=statuses[1,k]; return (s=="unsupported" && ok[substr(evid[1,k],8)]=="retired" && cs[ids[k]]!="unsupported") || (s=="skipped" && cs[ids[k]]!="skipped") }
function emit(k, j,line) { line=""; for(j=1;j<=10;j++) line=line (j>1?",":"") quote(r[k,j]); print line }
BEGIN { header1="id,title,machine,region,mapper,predicate,status,evidence,date"; header="id,title,machine,region,tv,mapper,predicate,status,evidence,date"; empty="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"; if(mode!="normalize" && mode!="cases" && mode!="docs" && mode!="ratchet" && mode!="results" && mode!="update") bad();
  cap_fixture=24; cap_unsupported=20; cap_fail=40; cap_other=40; cap_groups=60; cap_reasons=32; split(cap_groups " " cap_reasons " " cap_fixture " " cap_unsupported " " cap_fail " " cap_other,pc," ")
  # Fault prevented: a protected comparison or an update that quietly skips the catalog rules because it could not read the data, so a pass on a board that does not exist is accepted.
  if(mode=="update" || mode=="ratchet" || mode=="docs") while((got=(getline line < ENVIRON["COMPAT_CATALOG"]))!=0) { if(got<0) bad(); split(line,t,"\t"); if(length(t[1])==40 && t[1] ~ /^[0-9a-f]+$/) titles[t[1]]=t[5]; cm[t[1]]=t[2]; cp[t[1]]=t[3]; cs[t[1]]=(t[6]==""?"unknown":t[6]); sg+=(t[2]=="sg"); locked+=(t[6]=="unsupported" || t[6]=="skipped") }
}
{
  sub(/\r$/,"" ); if(length($0)>4096 || $0=="" || $0 ~ /[[:cntrl:]]/ || FNR>8193) bad();
  if(FNR==1) { stage++; ver=($0==header1?1:($0==header?2:0)); if(!ver) bad(); next }
  delete f; field=1; text=""; state=0;
  for(i=1;i<=length($0);i++) { c=substr($0,i,1);
    if(state==1) { if(c=="\"") { if(substr($0,i+1,1)=="\"") { text=text "\""; i++ } else state=2 } else text=text c }
    else if(c==",") { f[field++]=text; text=""; state=0 }
    else if(c=="\"" && state==0 && text=="") state=1;
    else if(c=="\"" || state==2) bad(); else { text=text c; state=3 }
  }
  f[field]=text; if(state==1 || field!=(ver==1?9:10)) bad();
  if(ver==1) { lt=f[4]; if(lt !~ /^(ntsc-u|ntsc-j|pal|any)$/) bad(); f[4]=(lt=="ntsc-j"?"japan":"export"); for(i=9;i>=5;i--) f[i+1]=f[i]; f[5]=(lt=="pal"?"pal":"ntsc") }
  for(i=1;i<=10;i++) if(f[i]=="" || length(f[i])>1024) bad();
  if(!(f[1] ~ /^fixture\/[A-Za-z0-9_.-]+$/ || (f[1] ~ /^[0-9a-f]+$/ && length(f[1])==40)) || f[3] !~ /^(sms|sms1|gg|sg)$/ || f[4] !~ /^(export|japan)$/ || f[5] !~ /^(ntsc|pal)$/ || f[6] !~ /^[a-z][a-z0-9_-]*$/ || f[8] !~ /^(pass|fail|unknown|unsupported|infrastructure_error|skipped)$/ || f[10] !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$/ || (f[5]=="pal" && (f[3]=="gg" || f[4]=="japan"))) bad();
  # Fault prevented: a title that spoofs text direction, hides characters, breaks a line (U+2028, U+2029, C1 controls) or becomes a spreadsheet formula (a lone dash is the facts placeholder).
  if(f[2] ~ /^[=+@-]./ || f[2] ~ /\342\200[\213-\217\250-\256]|\342\201[\240-\244\246-\251]|\357\273\277|\302[\200-\237\255]|\330\234/) bad();
  # Fault prevented: an unsupported or skipped row with no reason, or free text a reader cannot check, so a silent refusal reads as an honest status.
  if(f[8] ~ /^(unsupported|skipped)$/) {
    if(f[9] !~ /^reason=[a-z0-9_]+$/) bad();
    reasons(); if(!(substr(f[9],8) in ok)) bad()
  }
  f[7]=predicate(f[7],f[3],f[4]); key=f[1] "|" f[3] "|" f[4] "|" f[5] "|" f[6] "|" config "|" parameter;
  if(parameter ~ /^audio@/ && (config !~ /;script=none;/ || f[1] !~ /^fixture\//)) bad();
  if(length(f[1])==40 && kind!="boots" && !(mode=="update" && stage==2)) bad();
  if((stage SUBSEP key) in seen) bad(); seen[stage,key]=1;
  if(mode=="update" && stage==2) {
    # Fault prevented: a candidate adding rows by frame, device or profile for a catalog id, growing the page past its cap or smuggling a golden into the ledger; or turning a skipped or unsupported row into a pass, or its reason into core=<revision> (a library run observes only pass and fail).
    if(!(f[1] in titles) || cs[f[1]]!="unknown") next;
    if(key SUBSEP 1 in r) { if(f[7]!=r[key,7] || f[8] ~ /^(unsupported|skipped)$/) next }
    else {
      match(config,/;fm=(on|off)/); pf=substr(config,RSTART+4,RLENGTH-4); w=(f[3]=="gg"?" export,ntsc,off japan,ntsc,off ":(f[3]~/^sms1?$/?" export,ntsc,off export,pal,off japan,ntsc,on ":""));
      if(f[3]!=cm[f[1]] || f[6]!=cp[f[1]] || parameter!="boots@600" || f[8]!~/^(pass|fail)$/ || !index(w," " f[4] "," f[5] "," pf " ") || config!="cfg:video=" (f[3]=="gg"?"gg":"sms") ";fm=" pf ";port1=pad;port2=pad;script=none;sha256=" empty) next;
      order[++count]=key
    }
    f[2]=titles[f[1]]; f[9]="core=" ENVIRON["COMPAT_REV"]; day(f[10]); for(i=1;i<=10;i++) r[key,i]=f[i]; next;
  }
  order[++count]=key;
  if(mode=="ratchet") { values[stage,key]=expected; statuses[stage,key]=f[8]; ids[key]=f[1]; evid[stage,key]=f[9]; machines[key]=f[3]; regions[key]=f[4]; tvs[key]=f[5]; predicates[stage,key]=f[7]; next }
  for(i=1;i<=10;i++) r[key,i]=f[i]; r[key,11]=parameter; r[key,12]=expected; r[key,13]=config; r[key,14]=cap;
}
END {
  if(failed) exit 2; if(NR==0) bad();
  if(mode=="update" && (stage!=2 || ENVIRON["COMPAT_REV"] !~ /^[0-9a-f]+$/ || length(ENVIRON["COMPAT_REV"])!=40)) bad();
  if(mode=="ratchet") {
    today=day(ENVIRON["COMPAT_TODAY"]); base=ENVIRON["COMPAT_BASE_REV"]; head=ENVIRON["COMPAT_HEAD_REV"]; reasons();
    if(stage<2 || stage>3 || base !~ /^[0-9]+$/ || head !~ /^[0-9]+$/ || head+0>4294967295 || head+0<base+0 || !locked) bad();
    while((readline=(getline line < ENVIRON["COMPAT_ALLOW"]))!=0) {
      if(readline<0 || ++allow_rows>4096 || length(line)>4096) bad(); if(line ~ /^#/ || line=="") continue; n=split(line,a," "); pt=(a[4] ~ /^(export|japan)\/(ntsc|pal)$/); if(n<6+pt || a[n-1] !~ /^behaviour-rev=[0-9]+$/ || a[n] !~ /^expires=/) bad();
      rev=a[n-1]; sub(/^behaviour-rev=/,"",rev); expiry=a[n]; sub(/^expires=/,"",expiry); until_day=day(expiry);
      if(day(a[1])>today || until_day<today || until_day-today>60 || day(a[1])>until_day || rev+0!=head+0 || head+0<=base+0) bad();
      matches=0; for(key in ids) if(seen[1,key] && statuses[1,key]=="pass" && ids[key]==a[2] && (!pt || a[4]==regions[key] "/" tvs[key]) && predicates[1,key]==predicate(a[3],machines[key],regions[key])) { allowed[key]=1; matches++ }
      if(matches!=1) bad();
    }
    close(ENVIRON["COMPAT_ALLOW"]);
    for(key in ids) {
      old=statuses[1,key]; now=statuses[2,key];
      if(seen[1,key] && old=="pass" && (!seen[2,key] || now!="pass" || values[1,key]!=values[2,key]) && !allowed[key]) bad();
      if(seen[1,key] && seen[2,key] && old!=now && old!="unknown" && !(old=="fail" && now=="pass") && !rel(key) && !allowed[key]) bad();
      # Fault prevented: a head pass or fail for an id whose catalog still says unsupported or skipped, so a board that does not exist is credited.
      if(seen[2,key] && now ~ /^(pass|fail)$/ && cs[ids[key]] ~ /^(unsupported|skipped)$/) bad();
      # Fault prevented: hiding a failure by deleting its row (per key: another row of the id does not excuse it), calling an unsupported row passing with no board behind it, and a pin bump that deadlocks on the base judge (rel releases a row only on a retired token and a catalog change).
      if(seen[1,key] && old ~ /^(fail|unsupported|skipped)$/ && now!=old && !(old=="fail" && now=="pass") && !rel(key)) bad();
      if(stage==3 && ids[key] ~ /^fixture\// && ((seen[2,key] && now ~ /^(pass|fail)$/) || seen[3,key]) && (!seen[2,key] || !seen[3,key] || now!=statuses[3,key] || values[2,key]!=values[3,key])) bad();
    }
    print "compat: protected comparison passed"; exit 0
  }
  for(i=2;i<=count;i++) { key=order[i]; j=i-1; while(j>0 && order[j]>key) { order[j+1]=order[j]; j-- } order[j+1]=key }
  if(mode=="normalize" || mode=="results" || mode=="update") { print header; for(i=1;i<=count;i++) { if(mode=="results" && (r[order[i],1] !~ /^fixture\// || r[order[i],8] !~ /^(pass|fail)$/)) continue; emit(order[i]) } }
  if(mode=="cases") { print "set(compat_count " count+0 ")"; split("id title machine region tv mapper predicate status evidence date parameter expected config cap",names," "); split("video fm port1 port2 script",parts," ");
    for(i=1;i<=count;i++) { key=order[i]; for(j=1;j<=14;j++) if(j!=2 && j!=9) print "set(compat_" i "_" names[j] " [[" r[key,j] "]])"; split(r[key,13],t,";");
      for(j=1;j<=5;j++) { sub(/^[a-z0-9:]*=/,"",t[j]); print "set(compat_" i "_" parts[j] " [[" t[j] "]])" } print "set(compat_" i "_runner_region [[" (r[key,5]=="pal"?"pal":(r[key,4]=="japan"?"ntsc-j":"ntsc-u")) "]])" } }
  # Fault prevented: a generated page that grows with the ledger past the docs cap, or prints an untrusted title, a count claim that can go stale, or a bidi or zero-width character.
  if(mode=="docs") {
    for(i=1;i<=count;i++) { k=order[i]; st=r[k,8]; ev=(st=="pass" || st=="fail"); status[st]++; split(r[k,11],a,"@"); kinds[a[1]]++; pal+=(ev && r[k,5]=="pal"); fmj+=(ev && index(r[k,13],";fm=on;")>0); g=r[k,3] "/" substr(r[k,6],1,32) "/" st; if(!gn[g]++) gl[++ng]=g;
      if(r[k,1] ~ /^fixture\//) fx[r[k,1]]=1; if(st ~ /^(unsupported|skipped)$/) { g=substr(r[k,9],8,32) " | " st; if(!rn[g]++) rl[++nr]=g }
      s=(r[k,1] ~ /^fixture\//?3:(st=="unsupported"?4:(st=="pass"?6:(ev || st=="infrastructure_error"?5:0)))); if(s) put(s,k) }
    for(i=1;i<=ng;i++) put(1,"| " gl[i] " | " gn[gl[i]] " |"); for(i=1;i<=nr;i++) put(2,"| " rl[i] " | " rn[rl[i]] " |"); for(k in fx) nfx++
    print "# Compatibility\n\nGenerated from `tests/compat/ledger.csv` by `sh scripts/compat-common.sh docs tests/compat/ledger.csv`.\n\nExperimental. A row marked unknown has no accepted check; that says nothing about whether the game works.\nPass and fail describe accepted checks only (boots is a sampled display and first-fault smoke, audio a pinned hash of chip writes, never how it sounds), with no playable or completable claim. Unsupported means the runner refuses the dump with exit 77. Skipped is the MAME verdict about the dump and is never recorded as pass.\nA 40-hex id of a compound entry is derived from the text `mame/<list>/<name>` and is not an image hash. Experimental leaves only on the own-library evidence of the maintainer, never on a pass rate.\nA row names its setting: machine, region, tv, mapper, fm and the check with its frame; tv is a setting of the run, not a fact about the cartridge.\n"
    print "Rows: " count+0 ". Evaluated coverage: " status["pass"]+status["fail"] ". Own fixtures: " nfx+0 ".\n\n- pass=" status["pass"]+0 " fail=" status["fail"]+0 " infrastructure_error=" status["infrastructure_error"]+0 "\n- unevaluated: unknown=" status["unknown"]+0 " unsupported=" status["unsupported"]+0 " skipped=" status["skipped"]+0 "\n- pal rows judged: " pal+0 "; fm=on rows judged: " fmj+0 "\n- predicates: frame=" kinds["frame"]+0 " ram=" kinds["ram"]+0 " mailbox=" kinds["mailbox"]+0 " boots=" kinds["boots"]+0 " audio=" kinds["audio"]+0 "\n- SG-1000 entries excluded: " sg+0 " (counted in the catalog; the ledger has no row for them)"
    split("groups,reasons,fixtures,unsupported rows,fail rows,other evaluated rows",nm,","); hd[1]="| Machine/mapper/status | Rows |\n|---|---:|"; hd[2]="| token | status | rows |\n|---|---|---:|"; rh="| ID | Title | Profile | Status | Date |\n|---|---|---|---|---|";
    for(s=1;s<=6;s++) print "\n## " toupper(substr(nm[s],1,1)) substr(nm[s],2) "\n\n" (pn[s]+0 || s<3 ? (s<3?hd[s]:rh) pb[s] : "none") (pn[s]>pc[s] ? "\n\n+" pn[s]-pc[s] " more " nm[s] (s>2 ? "; the rest is in [the ledger](../tests/compat/ledger.csv)" : "") : "")
  }
}' "$@"
