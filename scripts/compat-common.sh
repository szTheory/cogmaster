#!/bin/sh
# Shared bounded quoted CSV: normalize, cases (safe CMake facts), or docs. No private execution.
set -eu
export LC_ALL=C
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
function md(s) { gsub(/&/,"\&amp;",s); gsub(/</,"\&lt;",s); gsub(/>/,"\&gt;",s); gsub(/\|/,"\&#124;",s); return s }
function hex(s, n,i,d) { n=0; s=toupper(s); for(i=1;i<=length(s);i++) { d=index("0123456789ABCDEF",substr(s,i,1))-1; if(d<0) bad(); n=n*16+d } return n }
function title(s, a,n) {
  while(match(s,/\\[0-7][0-7][0-7]/)) { a=substr(s,RSTART+1,3); n=substr(a,1,1)*64+substr(a,2,1)*8+substr(a,3,1); s=substr(s,1,RSTART-1) sprintf("%c",n) substr(s,RSTART+4) }
  gsub(/\\"/,"\"",s); gsub(/\\\\/,"\\",s); return s
}
function predicate(s, machine, p,n,a,b,t,frame,address,value,cfg) {
  cfg="cfg:video=" (machine=="gg"?"gg":"sms") ";port1=pad;port2=pad;script=none;sha256=" empty;
  if(index(s,"cfg:")==1) { n=index(s,"/"); if(!n) bad(); cfg=substr(s,1,n-1); s=substr(s,n+1) }
  if(cfg !~ /^cfg:video=(sms|gg);port1=(pad|none);port2=(pad|none);script=[A-Za-z0-9_.-]+;sha256=[0-9a-f]+$/) bad();
  split(cfg,t,"sha256="); if(length(t[2])!=64 || (cfg ~ /;script=none;/ && t[2]!=empty)) bad();
  p=index(s,"@"); kind=substr(s,1,p-1); s=substr(s,p+1); n=match(s,/[:=]/); frame=n?substr(s,1,n-1):s; value=""; address="";
  if(frame !~ /^[0-9]+$/ || length(frame)>10 || frame+0<1 || frame+0>1000000) bad(); frame=sprintf("%.0f",frame+0);
  if(kind=="frame") { value=substr(s,n+1); if(!n || substr(s,n,1)!="=" || value !~ /^[0-9a-f]+$/ || length(value)!=64) bad() }
  else if(kind=="ram") { a=substr(s,n); if(a !~ /^:\$[0-9a-fA-F]+=[0-9a-fA-F]+$/) bad(); split(substr(a,3),b,"="); if(length(b[1])>4 || hex(b[1])<49152 || length(b[2])>2) bad(); address=sprintf("%04X",hex(b[1])); value=sprintf("%02X",hex(b[2])) }
  else if((kind!="boots" && kind!="mailbox") || n) bad();
  expected=value; parameter=kind "@" frame (kind=="ram"?":$" address:""); config=cfg; cap=frame;
  return cfg "/" parameter (value!=""?"=" value:"")
}
BEGIN { header="id,title,machine,region,mapper,predicate,status,evidence,date"; empty="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"; if(mode!="normalize" && mode!="cases" && mode!="docs" && mode!="ratchet" && mode!="results" && mode!="update") bad();
  if(mode=="update") while((getline line < ENVIRON["COMPAT_CATALOG"])>0) {
    if(line !~ /,1u},$/) continue; id=substr(line,3,40); text=substr(line,45); if(match(text,/^"([^"\\]|\\.)*"/)) titles[id]=title(substr(text,2,RLENGTH-2));
  }
}
{
  sub(/\r$/,"" ); if(length($0)>4096 || $0=="" || $0 ~ /[[:cntrl:]]/ || FNR>4097) bad();
  delete f; field=1; text=""; state=0;
  for(i=1;i<=length($0);i++) { c=substr($0,i,1);
    if(state==1) { if(c=="\"") { if(substr($0,i+1,1)=="\"") { text=text "\""; i++ } else state=2 } else text=text c }
    else if(c==",") { f[field++]=text; text=""; state=0 }
    else if(c=="\"" && state==0 && text=="") state=1;
    else if(c=="\"" || state==2) bad(); else { text=text c; state=3 }
  }
  f[field]=text; if(state==1 || field!=9) bad();
  for(i=1;i<=9;i++) if(f[i]=="" || length(f[i])>1024) bad();
  if(FNR==1) { stage++; h=""; for(i=1;i<=9;i++) h=h (i>1?",":"") f[i]; if(h!=header) bad(); next }
  if(!(f[1] ~ /^(fixture|built)\/[A-Za-z0-9_.-]+$/ || (f[1] ~ /^[0-9a-f]+$/ && length(f[1])==40)) || f[3] !~ /^(sms|sms1|gg|sg)$/ || f[4] !~ /^(ntsc-u|ntsc-j|pal|any)$/ || f[5] !~ /^[a-z][a-z0-9_-]*$/ || f[7] !~ /^(pass|fail|unknown|unsupported|infrastructure_error|skipped)$/ || f[9] !~ /^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$/) bad();
  if(f[4]=="any") f[4]="ntsc-u"; f[6]=predicate(f[6],f[3]); key=f[1] "|" f[3] "|" f[4] "|" f[5] "|" config "|" parameter;
  if((stage SUBSEP key) in seen) bad(); seen[stage,key]=1;
  if(mode=="update" && stage==2) {
    if(!(f[1] in titles) || !(key SUBSEP 1 in r) || f[6]!=r[key,6]) next;
    f[2]=titles[f[1]]; f[8]="core=" ENVIRON["COMPAT_REV"]; day(f[9]); for(i=1;i<=9;i++) r[key,i]=f[i]; next;
  }
  order[++count]=key;
  if(mode=="ratchet") { values[stage,key]=expected; statuses[stage,key]=f[7]; ids[key]=f[1]; machines[key]=f[3]; predicates[stage,key]=f[6]; next }
  for(i=1;i<=9;i++) r[key,i]=f[i]; r[key,10]=parameter; r[key,11]=expected; r[key,12]=config; r[key,13]=cap;
}
END {
  if(failed) exit 2; if(NR==0) bad();
  if(mode=="update" && (stage!=2 || ENVIRON["COMPAT_REV"] !~ /^[0-9a-f]+$/ || length(ENVIRON["COMPAT_REV"])!=40)) bad();
  if(mode=="ratchet") {
    today=day(ENVIRON["COMPAT_TODAY"]); base=ENVIRON["COMPAT_BASE_REV"]; head=ENVIRON["COMPAT_HEAD_REV"];
    if(stage<2 || stage>3 || base !~ /^[0-9]+$/ || head !~ /^[0-9]+$/ || head+0>4294967295 || head+0<base+0) bad();
    while((readline=(getline line < ENVIRON["COMPAT_ALLOW"]))!=0) {
      if(readline<0 || ++allow_rows>4096 || length(line)>4096) bad(); if(line ~ /^#/ || line=="") continue; n=split(line,a," "); if(n<6 || a[n-1] !~ /^behaviour-rev=[0-9]+$/ || a[n] !~ /^expires=/) bad();
      rev=a[n-1]; sub(/^behaviour-rev=/,"",rev); expiry=a[n]; sub(/^expires=/,"",expiry); until_day=day(expiry);
      if(day(a[1])>today || until_day<today || until_day-today>60 || day(a[1])>until_day || rev+0!=head+0 || head+0<=base+0) bad();
      matches=0; for(key in ids) if(seen[1,key] && statuses[1,key]=="pass" && ids[key]==a[2] && predicates[1,key]==predicate(a[3],machines[key])) { allowed[key]=1; matches++ }
      if(matches!=1) bad();
    }
    close(ENVIRON["COMPAT_ALLOW"]);
    for(key in ids) {
      old=statuses[1,key]; now=statuses[2,key];
      if(seen[1,key] && old=="pass" && (!seen[2,key] || now!="pass" || values[1,key]!=values[2,key]) && !allowed[key]) bad();
      if(seen[1,key] && seen[2,key] && old!=now && old!="unknown" && !(old=="fail" && now=="pass") && !allowed[key]) bad();
      if(stage==3 && ids[key] ~ /^fixture\// && ((seen[2,key] && now ~ /^(pass|fail)$/) || seen[3,key]) && (!seen[2,key] || !seen[3,key] || now!=statuses[3,key] || values[2,key]!=values[3,key])) bad();
    }
    print "compat: protected comparison passed"; exit 0
  }
  for(i=2;i<=count;i++) { key=order[i]; j=i-1; while(j>0 && order[j]>key) { order[j+1]=order[j]; j-- } order[j+1]=key }
  if(mode=="normalize" || mode=="results" || mode=="update") { print header; for(i=1;i<=count;i++) { if(mode=="results" && (r[order[i],1] !~ /^fixture\// || r[order[i],7] !~ /^(pass|fail)$/)) continue; line=""; for(j=1;j<=9;j++) line=line (j>1?",":"") quote(r[order[i],j]); print line } }
  if(mode=="cases") { print "set(compat_count " count+0 ")"; split("id title machine region mapper predicate status evidence date parameter expected config cap",names," "); for(i=1;i<=count;i++) for(j=1;j<=13;j++) if(j!=2 && j!=8) print "set(compat_" i "_" names[j] " [[" r[order[i],j] "]])" }
  if(mode=="docs") {
    for(i=1;i<=count;i++) { key=order[i]; status[r[key,7]]++; split(r[key,10],a,"@"); predicates[a[1]]++; if(r[key,1] ~ /^fixture\//) fixtures[r[key,1]]=1; group=r[key,3] "/" r[key,5] "/" r[key,7]; groups[group]++; }
    for(key in fixtures) fixture_count++;
    print "# Compatibility\n\nGenerated from `tests/compat/ledger.csv`; regenerate with `sh scripts/compat-common.sh docs tests/compat/ledger.csv`.\n";
    print "Rows: " count+0 ". Evaluated coverage: " status["pass"]+status["fail"] ". Own fixtures: " fixture_count+0 ".";
    print "\nPass/fail describe accepted predicates only. Unknown has no accepted evaluation; unsupported names an unavailable capability; infrastructure_error describes a runner/judge failure. Metadata membership proves no support. No playable, completable or sound claim; boots is sampled display/distinct-frame/first-fault smoke and may reject static screens. HALT is not a rejection. Experimental through v2.0.\n";
    split("pass fail unknown unsupported infrastructure_error skipped",a," "); for(i=1;i<=6;i++) print "- " a[i] ": " status[a[i]]+0;
    print ""; split("frame ram mailbox boots",a," "); for(i=1;i<=4;i++) print "- " a[i] " predicates: " predicates[a[i]]+0;
    print "\n| Machine/mapper/status | Rows |\n|---|---:|"; for(i=1;i<=count;i++) { key=order[i]; group=r[key,3] "/" r[key,5] "/" r[key,7]; if(!printed[group]++) print "| " group " | " groups[group] " |" }
    print "\n| ID | Title | Profile | Predicate | Status | Evidence | Date |\n|---|---|---|---|---|---|---|";
    for(i=1;i<=count;i++) { key=order[i]; print "| " r[key,1] " | " md(r[key,2]) " | " r[key,3] "/" r[key,4] "/" r[key,5] " | " r[key,6] " | " r[key,7] " | " md(r[key,8]) " | " r[key,9] " |" }
  }
}' "$@"
