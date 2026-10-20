#!/bin/sh
# Shared scans and set comparisons; every failure names the original prevented fault.
set -eu
LC_ALL=C
export LC_ALL
fail() { echo "$*" >&2; exit 1; }
scan() { find "$1" -type f \( -name "$2" -o -name "${3-$2}" \) -exec awk "$program" {} +; }
case $1 in
action_pins|timeouts)
  program='BEGIN { mode=ENVIRON["CHECK_MODE"]; sha="[0-9a-f]"; for(i=1;i<40;i++) sha=sha "[0-9a-f]" }
  /^[ \t-]*uses:/ { if(mode!="action_pins") next; n++; sub(/^[ \t-]*/, ""); if($0 !~ /uses: \.\// && $0 !~ "uses: [A-Za-z0-9_.-]+/[A-Za-z0-9_./-]+@" sha " +# v[0-9]") { file=FILENAME; sub(/^.*\//,"",file); print "action_pins: " file ": " $0; bad++ } }
  /^[ \t-]*timeout-minutes:/ { if(mode!="timeouts") next; n++; if($0 !~ /timeout-minutes:[^#]*#.*(provisional|runs? [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9])/) { sub(/^[ \t]*/, ""); print FILENAME ": " $0; bad++ } }
  END { if(mode=="action_pins" && bad) print "action_pins: " bad " uses: lines not pinned to a commit with a # v comment"; if(mode=="timeouts" && (bad || !n)) print "timeout_comments: " bad " lines without a run ID or provisional"; exit(bad || (mode=="timeouts" && !n)) }'
  export CHECK_MODE=$1
  scan "$DIR" '*.yml' '*.yaml' ;;
pins)
  awk 'NF && $1 !~ /^#/ {
    n++; if(NF!=4) { print "line " NR ": " NF " fields, not 4"; bad=1; next }
    if(names[$1]++) {print "line " NR ": " $1 " is listed twice"; bad=1}
    if($3 !~ /^https:\/\//) {print $1 ": the URL is not https://"; bad=1}
    if($4 !~ /^[0-9a-f]+$/ || length($4)!=64) {print $1 ": the digest is not 64 lower-case hex digits"; bad=1}
  } END { if(!n) print "no pins"; exit(bad || !n) }' "$PINS" ;;
no_skip|no_float)
  export CHECK_MODE=$1
  program='BEGIN { mode=ENVIRON["CHECK_MODE"]; skip="SKIP_"; disabled="DIS" "ABLED" }
  { line=$0; if(mode=="no_skip") { if(match(line,"(^|[^A-Za-z0-9_])(" skip "RETURN_CODE|" skip "REGULAR_EXPRESSION|" disabled ")([^A-Za-z0-9_]|$)")) {rule=substr(line,RSTART,RLENGTH); gsub(/^[^A-Za-z_]+|[^A-Za-z_]+$/,"",rule); file=FILENAME; sub(ENVIRON["ROOT"] "/","",file); print file ":" FNR ": " rule; bad++ } }
  else { s=" " line " "; gsub(/0[xX][0-9a-fA-F]+|section [0-9.]*[0-9]/,"",s); if(line ~ /0[xX][0-9a-fA-F]*\.?[0-9a-fA-F]*[pP][+-]?[0-9]/ || s ~ /[^A-Za-z0-9_](float|double)[^A-Za-z0-9_]|<(math|float)\.h>|[0-9]\.[0-9eE]|[^A-Za-z0-9_][0-9]+[eE][+-]?[0-9]/) { print FILENAME ":" FNR ": " line; bad++ } } }
  END { if(bad) print mode ": " bad " lines violate source policy"; exit(bad!=0) }'
  if [ "$1" = no_skip ]; then
    for dir in "$ROOT/tests" "$ROOT/cmake" "$ROOT/CMakeLists.txt"; do
      [ ! -e "$dir" ] || find "$dir" -type f \( -name '*.cmake' -o -name CMakeLists.txt \) -exec awk "$program" {} +
    done
  else
    printf '%s\n' "$ROOTS" | while IFS= read -r dir; do scan "$dir" '*.c' '*.h' || exit 1; done
  fi ;;
api_names)
  # Compare distinct identifier sets in both directions; trailing underscores are guide wildcards.
  awk 'function names(s,a, n) { while(match(s,/(cogmaster_[a-z0-9_]+|COGMASTER_[A-Z0-9_]+)/)) { n=substr(s,RSTART,RLENGTH); if(n!="COGMASTER_H") a[n]=1; s=substr(s,RSTART+RLENGTH) } }
  BEGIN { while((getline s < ENVIRON["HEADER"])>0) names(s,declared); split(ENVIRON["OTHER"],files,"\n"); for(f in files) while((getline s < files[f])>0) names(s,cited) }
  { names($0,written) }
  END {
    for(n in declared) if(n !~ /_$/ && !(n in written)) {print n " is declared in the header but the guide never names it"; bad=1}
    for(n in written) {
      known=(n in declared || n in cited); if(n ~ /_$/) for(d in declared) if(index(d,n)==1) known=1;
      if(!known) {print n " is named in the guide but the header does not declare it"; bad=1}
    } exit bad
  }' "$DOC" ;;
api_version)
  awk 'BEGIN {
  while((getline s < ENVIRON["HEADER"])>0) h=h s "\n";
  v=h; sub(/^.*#define COGMASTER_API_VERSION /,"",v); sub(/[^0-9].*$/,"",v);
  if(v !~ /^[0-9]+$/) {print "defines no COGMASTER_API_VERSION"; exit 1}
  path=ENVIRON["API_DIR"] "/v" v ".c";
  if((getline s < path)<0) {print path " is missing for API version " v; exit 1} close(path);
  for(k=1;k<=v;k++) {path=ENVIRON["API_DIR"] "/v" k ".c"; while((getline s < path)>0) tests=tests " " s " "; close(path)}
  gsub(/\/\*([^*]|\*+[^*\/])*\*+\//," ",h); s=h; while(match(s,/[Cc][Oo][Gg][Mm][Aa][Ss][Tt][Ee][Rr][A-Za-z0-9_]*/)) {names[substr(s,RSTART,RLENGTH)]=1; s=substr(s,RSTART+RLENGTH)} delete names["COGMASTER_H"];
  while(match(h,/struct [a-z_]+ {[^}]*}/)) {s=substr(h,RSTART,RLENGTH); h=substr(h,RSTART+RLENGTH); gsub(/[A-Za-z0-9_)]\([^()]*\)/,"",s); while(match(s,/[A-Za-z_][A-Za-z0-9_]*\)*;/)) {n=substr(s,RSTART,RLENGTH); gsub(/[);]/,"",n); names[n]=1; s=substr(s,RSTART+RLENGTH)} }
  for(n in names) if(tests !~ "[^A-Za-z0-9_]" n "[^A-Za-z0-9_]") { print "in no tests/api/vK.c with K <= " v ": " n; bad=1 } exit bad }' ;;
info_claims)
  awk 'BEGIN {claims["supports_no_game"]="libretro.host.xrgb8888"; claims["input_descriptors"]="libretro.host.content"; while((getline s < ENVIRON["INVENTORY"])>0) {split(s,a,"\t"); inventory[a[1]]=1} }
  /^[a-z_0-9]+ = "true"$/ && $1!="is_experimental" { if(!($1 in claims)) {print $0 " maps to no test"; bad=1} else if(!(claims[$1] in inventory)) {print $1 " needs " claims[$1] ", which is not in " ENVIRON["INVENTORY"]; bad=1} } END {exit bad}' "$INFO" ;;
single_version)
  [ -n "$VERSION" ] || VERSION=$(cat "$ROOT/version.txt")
  printf '%s\n' "$VERSION" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$' || fail 'single_version: expected MAJOR.MINOR.PATCH'
  if [ "$NEXT" = ON ]; then VERSION=$(printf '%s\n' "$VERSION" | awk -F. '{printf "%d.%d.0|%d.%d.%d",$1,$2+1,$1,$2,$3+1}'); fi
  export VERSION
  listing=$(git -C "$ROOT" -c core.quotePath=false ls-files) || fail 'single_version: cannot list repository'
  printf '%s\n' "$listing" | while IFS= read -r file; do [ ! -f "$ROOT/$file" ] || printf '%s\0' "$file"; done |
    (cd "$ROOT" && xargs -0 awk 'BEGIN {v=ENVIRON["VERSION"]; gsub(/\./,"\\.",v)}
    FILENAME !~ /^(version\.txt|\.release-please-manifest\.json|release-please-config\.json|CHANGELOG\.md|\.planning\/.*)$/ && $0 ~ "(^|[^0-9.])(" v ")([^0-9.]|$)" {
      match($0,"(^|[^0-9.])(" v ")([^0-9.]|$)"); s=substr($0,RSTART,RLENGTH);
      sub(/^[^0-9]*/,"",s); sub(/[^0-9.]*$/,"",s); print FILENAME " holds " s; bad=1
    } END {exit bad}') ;;
vendored)
  [ -n "$PROVENANCE" ] || PROVENANCE=$ROOT/PROVENANCE.md
  awk '/^## / {section=($0 ~ /^## 8\. /)} section && /^\| `/ {n=split($0,a,"`"); if(n>=5) print a[2] " " a[n-1]}' "$PROVENANCE" |
    { n=0; while read -r path digest; do n=$((n+1)); [ -f "$ROOT/$path" ] || fail "vendored: $path is listed but missing"; got=$(cmake -E sha256sum "$ROOT/$path"); [ "${got%% *}" = "$digest" ] || fail "vendored: $path: sha256 differs, PROVENANCE.md records $digest"; done; [ "$n" -gt 0 ] || fail 'vendored: no file rows found'; } ;;
abi)
  symbols=$("$NM" -P "$LIB") || fail "abi: nm failed for $LIB"
  printf '%s\n' "$symbols" | awk '
  BEGIN {
    sys=ENVIRON["SYSTEM"]; mode=ENVIRON["MODE"];
    split("memcpy memmove memset memcmp",a," "); for(i in a) allowed[a[i]]=1;
    if(sys ~ /^Darwin\//) {allowed["bzero"]=1; allowed["__bzero"]=1}
    if(sys ~ /^Linux\/(x86_64|amd64|AMD64)$/) allowed["_GLOBAL_OFFSET_TABLE_"]=1;
    if(sys ~ /^Linux\/(ppc|powerpc)$/) {split("__udivdi3 __umoddi3 __ashldi3 __lshrdi3",a," "); for(i in a) allowed[a[i]]=1}
    if(sys ~ /^Windows\/(AMD64|x86_64)$/) {allowed["__isa_available"]=1; allowed["__ImageBase"]=1}
    if(mode=="symbols") while((getline s < ENVIRON["HEADER"])>0) while(match(s,/cogmaster_[a-z0-9_]*\(/)) {n=substr(s,RSTART,RLENGTH-1); declared[n]=1; s=substr(s,RSTART+RLENGTH)}
  }
  /^[^ ]+ [A-Za-z]( |$)/ {
    n=$1; type=$2; if(sys ~ /^Darwin\//) sub(/^_/,"",n);
    if(type=="U") {undefined[n]=1; next} defined[n]=1;
    if(sys ~ /^Windows\// && n ~ /^(\?\?_C@_|__xmm@)/) next;
    if(sys ~ /^Windows\/(AMD64|x86_64)$/ && n=="__isa_available_default") next;
    if(type ~ /^[BbDdC]$/) writable[n]=type;
    if(type ~ /^[A-Z]$/) external[n]=1;
  }
  END {
    if(mode=="undefined") for(n in undefined) {if(!(n in defined) && !(n in allowed)) {print "unexpected undefined symbol: " n; bad=1}}
    if(mode=="globals") for(n in writable) {print "writable data symbol: " writable[n] " " n; bad=1}
    if(mode=="symbols") {for(n in declared) if(!(n in external)) {print "public name mismatch: declared, not defined: " n; bad=1} for(n in external) if(n !~ /^cogmaster__/ && !(n in declared)) {print "public name mismatch: defined, not declared or outside the cogmaster_ namespace: " n; bad=1}}
    if(mode!="undefined" && mode!="globals" && mode!="symbols") {print "abi: unknown mode"; bad=1}
    exit bad
  }' ;;
notices)
  listing=$(git -C "$ROOT" -c core.quotePath=false ls-files -- '*.c' '*.h') || fail 'notices: cannot list repository'
  printf '%s\n' "$listing" | while IFS= read -r file; do printf '%s\0' "$file"; done | (cd "$ROOT" && xargs -0 awk '
  BEGIN {
    while((getline s < "THIRD-PARTY-NOTICES.md")>0) {
      if(s ~ /^\*\*Applies to:\*\*/) while(match(s,/`[^`]+`/)) {p=substr(s,RSTART+1,RLENGTH-2); named[p]=1; if((getline line < p)<0) {print p " does not exist"; bad=1} close(p); s=substr(s,RSTART+RLENGTH)}
      if(s ~ /^\*\*Licence:\*\*/) {sub(/^\*\*Licence:\*\* /,"",s); licences[s]=1}
    }
    licences["MIT"]=1;
    while((getline s < "libretro/cogmaster_libretro.info.in")>0) if(s ~ /^license = /) {sub(/^license = "/,"",s); sub(/"$/,"",s); split(s,a,/, */); for(i in a) info[a[i]]=1}
  }
  /SPDX-License-Identifier:/ && !seen[FILENAME]++ {
    s=$0; sub(/^.*SPDX-License-Identifier: /,"",s); sub(/ \*\/$/,"",s);
    if(s !~ /^MIT( AND (BSD-2-Clause|BSD-3-Clause|Zlib|BSL-1\.0|ISC|CC0-1\.0))?$/) {print FILENAME ": SPDX is not MIT or MIT AND a PROVENANCE.md licence"; bad=1}
    else if(s!="MIT") {sub(/^MIT AND /,"",s); licences[s]=1; if(!(FILENAME in named)) {print FILENAME " has no notices section"; bad=1}}
  }
  END {for(s in licences) if(!(s in info)) {print ".info licence lacks " s; bad=1} for(s in info) if(!(s in licences)) {print ".info licence adds " s; bad=1} exit bad}') ;;
check_junit)
  awk 'BEGIN {while((getline s < ENVIRON["INVENTORY"])>0) if(s !~ /^#/ && index(s,"\t")) {split(s,a,"\t"); if((ENVIRON["LABELS"]=="" || a[2] ~ ENVIRON["LABELS"]) && (ENVIRON["EXCLUDE"]=="" || a[2] !~ ENVIRON["EXCLUDE"])) expected[a[1]]=1} }
  {xml=xml $0 "\n"}
  END {
    n=split(xml,cases,"<testcase "); for(i=2;i<=n;i++) {
      s=cases[i]; name=s; sub(/^name="/,"",name); sub(/".*$/,"",name); ran[name]++;
      if(s !~ /^[^>]*status="run"/ || s ~ /<(failure|error|skipped)/) {print name ": failed, errored or did not run"; bad=1}
    }
    for(name in expected) {selected++; if(ran[name]!=1) {print name ": in the JUnit file " ran[name]+0 " times, expected once"; bad=1}}
    if(!selected || n<2) {print "no test ran"; bad=1} exit bad
  }' "$JUNIT" ;;
e2e_budget)
  [ -d "$DIR" ] || fail 'e2e_budget: DIR must name a directory'
  counts=$(find "$DIR" -type f -exec awk 'END {print NR}' {} +) || fail 'e2e_budget: unreadable helper'
  printf '%s\n' "$counts" | awk '{n+=$1} END {print "e2e_budget: " n+0 " lines under tests/retroarch, at most 300"; exit(n>300)}' ;;
exit_table)
  awk 'BEGIN {while((getline s < ENVIRON["DOC"])>0) {doc[s]=1; if(s ~ /^  [0-9]+  [a-z]+ /) documented++}}
  /^exit status:$/ {table=1; next}
  table && NF {n++; if(!($0 in doc)) {print "document lacks the exit status line: " $0; bad=1}}
  END {if(!n || n!=documented) {print "exit_table: mismatched exit status lines"; bad=1} exit bad}' "$RUN_HELP" ;;
*) fail "unknown repository check: $1" ;;
esac
