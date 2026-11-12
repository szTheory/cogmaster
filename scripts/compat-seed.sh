#!/bin/sh
# Writes the whole-list ledger: one default-profile row for every Master System and Game Gear catalog identity, from the catalog alone.
# Fault prevented: a ledger edited by hand into a state no generator would write, so a stale status, a dropped identity or a wrong default
# profile stays unseen. It writes and never judges; the shared parser stays the only parser (normalize and cases are read, never re-implemented).
# usage: compat-seed.sh LEDGER CATALOG DATE   (the new ledger is printed; the summary goes to standard error)
set -eu
export LC_ALL=C
[ $# -eq 3 ] || { echo 'usage: compat-seed.sh LEDGER CATALOG DATE' >&2; exit 2; }
ledger=$1 catalog=$2 date=$3
here=$(dirname "$0")
export COMPAT_CATALOG="$catalog" COMPAT_REASONS="${COMPAT_REASONS:-$here/../tests/compat/reasons.txt}"
tmp=$(mktemp -d) && trap 'rm -rf "$tmp"' EXIT HUP INT TERM
sh "$here/compat-common.sh" normalize "$ledger" > "$tmp/norm"
sh "$here/compat-common.sh" cases "$ledger" > "$tmp/cases"
awk -v date="$date" -v summary="$tmp/summary" '
function q(s) { gsub(/"/,"\"\"",s); return "\"" s "\"" }
function row(i, d) { return q(cid[i]) "," q(ct[i]) "," q(cm[i]) "," q(cr[i]) ",\"ntsc\"," q(cb[i]) "," q(cp[i] "/boots@600") "," q(cs[i]) "," q(ce[i]) "," q(d) }
FNR==1 { fi++ }
fi==1 { if(FNR>1) nl[++nr]=$0; next }
fi==2 {
  if(match($0,/^set\(compat_[0-9]+_[a-z0-9_]+ \[\[/)) { tag=substr($0,12,RLENGTH-14); n=index(tag,"_"); v=substr($0,RLENGTH+1); c[substr(tag,1,n-1),substr(tag,n+1)]=substr(v,1,length(v)-3) }
  next
}
{
  split($0,t,"\t"); if(t[2]!="sms" && t[2]!="gg") next
  if(t[6]!="unknown" && t[6]!="unsupported" && t[6]!="skipped") { print "compat-seed: catalog status " t[6] " of " t[1] " is not a ledger status" > "/dev/stderr"; failed=1; exit 1 }
  ++nc; cid[nc]=t[1]; cm[nc]=t[2]; cb[nc]=t[3]; ct[nc]=t[5]; cs[nc]=t[6]; cr[nc]=t[8]; ce[nc]=(t[6]=="unknown"?"CC0 metadata only src/cartdb.inc":"reason=" t[7])
  cp[nc]="cfg:video=" (t[2]=="gg"?"gg":"sms") ";fm=" (t[8]=="japan" && t[2]=="sms"?"on":"off") ";port1=" t[9] ";port2=" t[10] ";script=none;sha256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
  dk[t[1]]=nc; dkey[nc]=cid[nc] "|" cm[nc] "|" cr[nc] "|ntsc|" cb[nc] "|" cp[nc] "|boots@600"
}
END {
  if(failed) exit 1
  for(i=1;i<=nr;i++) {
    id=c[i,"id"]; st=c[i,"status"]; key=id "|" c[i,"machine"] "|" c[i,"region"] "|" c[i,"tv"] "|" c[i,"mapper"] "|" c[i,"config"] "|" c[i,"parameter"]
    if(id ~ /^fixture\//) { keep[++no]=nl[i]; have[key]=1; continue }
    if(!(id in dk)) { print "compat-seed: " id " is neither a fixture nor a Master System or Game Gear catalog identity" > "/dev/stderr"; exit 1 }
    if(st ~ /^(pass|fail|infrastructure_error)$/) { keep[++no]=nl[i]; tally[st]++; have[key]=1; continue }
    if(key!=dkey[dk[id]]) continue
    k=dk[id]; line=row(k,c[i,"date"]); if(line!=nl[i]) line=row(k,date); keep[++no]=line; tally[cs[k]]++; have[key]=1
  }
  for(k=1;k<=nc;k++) if(!(dkey[k] in have)) { keep[++no]=row(k,date); tally[cs[k]]++ }
  print "id,title,machine,region,tv,mapper,predicate,status,evidence,date"; for(i=1;i<=no;i++) print keep[i]
  print "compat-seed: identities=" nc " rows=" no " unknown=" tally["unknown"]+0 " unsupported=" tally["unsupported"]+0 " skipped=" tally["skipped"]+0 > summary
}' "$tmp/norm" "$tmp/cases" "$catalog" > "$tmp/cand"
sh "$here/compat-common.sh" normalize "$tmp/cand"
cat "$tmp/summary" >&2
