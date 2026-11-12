# Shared comparison legs of the whole-list tests. Each leg is a function of file paths that returns an error text (empty means
# pass) and reads no -D variable and no environment, so a script can include this file and call a leg in-process for many small
# stand-ins without a cmake child per control. One awk per leg reads whole files; nothing here loops over ledger rows in CMake.
# Fault prevented: a denominator that shrinks unnoticed, a status that moves without a slot-table change, a ledger that is not the
# seed's output, and a catalog whose derived ids or boards drift from the facts the core is built from.
set(wl_scripts "${CMAKE_CURRENT_LIST_DIR}/../../scripts")

# A ledger line is read from its tail: the last six quoted fields (predicate, status, evidence, date and the two before) never hold a
# quote, only the title does, so splitting at quotes puts the id at 2 and status at n-5, evidence at n-3 whatever the title holds.
set(wl_count_awk [==[
function bad(m) { if(++errs<=3) print m }
FNR==1 { fi++ }
fi==1 { if(NF==2) pc[$1]=$2; next }
fi==2 {
  split($0,t,"\t")
  if(t[2]=="sms" || t[2]=="gg") { cat[t[1]]=1; ncat++; if(t[3]=="compound") compc++; else img++ }
  else if(t[2]=="sg") { sg++; if(t[3]=="compound") sgc++ }
  next
}
FNR>1 {
  split($0,q,"\""); rows++; id=q[2]
  if(id ~ /^fixture\//) nfx++
  else if(length(id)==40 && id !~ /[^0-9a-f]/) lid[id]=1
  else bad("ledger id " id " is neither a fixture nor a catalog SHA-1")
}
END {
  split("entries_sms entries_gg entries_sg merged_sms_gg merged_sg compound_sms_gg compound_sg", need, " ")
  for(i=1;i<=7;i++) if(!(need[i] in pc)) bad("pin-counts.txt lacks " need[i])
  for(id in lid) { ncart++; if(!(id in cat)) bad("ledger id " id " is absent from the catalog") }
  for(id in cat) if(!(id in lid)) bad("catalog identity " id " has no ledger row")
  want=pc["entries_sms"]+pc["entries_gg"]-pc["merged_sms_gg"]
  if(ncart!=want) bad("ledger cart identities " ncart " differ from the pin counts " want)
  if(ncat!=want) bad("catalog identities " ncat " differ from the pin counts " want)
  if(compc!=pc["compound_sms_gg"]) bad("catalog compound identities " compc " differ from the pin counts " pc["compound_sms_gg"])
  if(img!=want-pc["compound_sms_gg"]) bad("catalog images " img " differ from the pin counts " want-pc["compound_sms_gg"])
  if(sg!=pc["entries_sg"]-pc["merged_sg"] || sgc!=pc["compound_sg"]) bad("catalog SG-1000 lines " sg " (compound " sgc ") differ from the pin counts")
  if(rows*2>=8192) bad("ledger rows " rows " are not below half of the 8192 row cap")
  print "info rows=" rows " cart_identities=" ncart " images=" img " compound=" compc " sg1000=" sg " fixture_rows=" nfx+0
}
]==])

set(wl_reason_awk [==[
function bad(m) { if(++errs<=3) print m }
FNR==1 { fi++ }
fi==1 { if(NF==2) pc[$1]=$2; next }
fi==2 {
  split($0,t,"\t"); if(t[2]!="sms" && t[2]!="gg") next
  st[t[1]]=t[6]; tok[t[1]]=t[7]; ncat++
  if((t[6]=="unsupported" || t[6]=="skipped") && t[7] !~ /^[a-z0-9_]+$/) bad("catalog reason of " t[1] " is not a token")
  if(t[6]=="unsupported" && t[3]!="unsupported") bad("catalog marks " t[1] " unsupported on the supported board " t[3])
  if(t[3]=="unsupported" && t[6]!="unsupported" && t[6]!="skipped") bad("catalog board 0 identity " t[1] " is " t[6] ", not unsupported or skipped")
  if(t[7]=="mame_no") nmame++
  next
}
FNR>1 {
  n=split($0,q,"\""); id=q[2]; s=q[n-5]; e=q[n-3]
  if(!(id in st)) next
  cu=(st[id]=="unsupported" || st[id]=="skipped"); nrow++
  if(cu && (s=="pass" || s=="fail")) bad("a judged row for " id " whose catalog status is " st[id])
  else if(cu && s!=st[id]) bad("ledger status " s " of " id " differs from the catalog status " st[id])
  else if(!cu && (s=="unsupported" || s=="skipped")) bad("ledger status " s " of " id " differs from the catalog status " st[id])
  else if(cu && e!="reason=" tok[id]) bad("ledger reason " e " of " id " differs from the catalog token " tok[id])
}
END {
  if(nmame!=pc["mame_no"]) bad("catalog mame_no lines " nmame+0 " differ from the pin counts " pc["mame_no"]+0)
  if(!ncat || !nrow) bad("no catalog line or no ledger row was compared")
  print "info mame_no=" nmame+0 " catalog_lines=" ncat " cart_rows=" nrow
}
]==])

# The board, machine, slot and port names are those of tools/cartdb.c (board_names, machines, ports, the slot table at the head of the
# inc) and the COGMASTER_BOARD_* and COGMASTER_MACHINE_* constants of include/cogmaster.h; they are typed here, never read from the
# generator, so a generator slip is a difference. Both directions: every catalog line has its fact and every public Master System or
# Game Gear fact has its catalog line (an SG-1000 fact keeps its line in the catalog, not in the ledger).
set(wl_table_awk [==[
function bad(m) { if(++errs<=3) print "catalog differs from src/cartdb.inc: " m }
function num(k) { return match($0, "\\." k " = [0-9]+u") ? substr($0,RSTART+length(k)+4,RLENGTH-length(k)-5)+0 : -1 }
BEGIN { split("unsupported none sega codemasters korean korean_nb hicom zemina nemesis hes_4pak sega_eeprom", x, " "); for(i=1;i<=11;i++) bn[i-1]=x[i]; mn[0]="sg"; mn[1]="sms"; mn[2]="gg"; mn[3]="sms"; split("pad lphaser paddle sports pad", pn, " ") }
FNR==1 { fi++ }
fi==1 {
  if(match($0,/^\{[0-9]+u, [0-9]+u, "/)) { split($0,q,"\""); sn[substr($0,2)+0]=q[2]; next }
  if(index($0,"{.sha1 = {")!=1) next
  s=substr($0,11); h=substr(s,1,index(s,"}")-1); gsub(/0x|,/,"",h)
  if(length(h)!=40 || num("board")<0) next
  fb[h]=bn[num("board")]; fm[h]=mn[num("machine")]; fs[h]=(num("slot")?sn[num("slot")]:"-"); f1[h]=pn[num("ctrl1")+1]; f2[h]=pn[num("ctrl2")+1]
  fr[h]=(num("region")==2 || num("ctrl1")==2 || num("ctrl2")==2 ? "japan" : "export")
  if(num("flags")==1 && fm[h]!="sg") pub[h]=1
  nfact++; next
}
{
  split($0,t,"\t"); if((t[2]!="sms" && t[2]!="gg") || t[3]=="compound") next
  nchecked++; seen[t[1]]=1
  if(!(t[1] in fb)) bad("identity " t[1] " has no fact")
  else if(t[2]!=fm[t[1]] || t[3]!=fb[t[1]]) bad("identity " t[1] " is " t[2] " on " t[3] " but its fact is " fm[t[1]] " on " fb[t[1]])
  else if(t[4]!=fs[t[1]] || t[8]!=fr[t[1]] || t[9]!=f1[t[1]] || t[10]!=f2[t[1]]) bad("identity " t[1] " has slot " t[4] ", " t[8] ", ports " t[9] "/" t[10] " but its fact has " fs[t[1]] ", " fr[t[1]] ", " f1[t[1]] "/" f2[t[1]])
}
END { for(h in pub) if(!(h in seen)) bad("public fact " h " has no catalog line"); if(!nfact || !nchecked) bad("nothing was compared"); print "info facts=" nfact " compared=" nchecked }
]==])

set(wl_compound_awk [==[
BEGIN { FS="\t" }
($2=="sms" || $2=="gg" || $2=="sg") && $3=="compound" { print $1 " " $4 }
]==])

# Runs one awk program over files and splits its output into error lines and the info line.
function(wl_awk out info work name program)
  file(MAKE_DIRECTORY "${work}")
  file(CONFIGURE OUTPUT "${work}/${name}.awk" CONTENT "${program}" @ONLY NEWLINE_STYLE UNIX)
  execute_process(COMMAND awk -f "${work}/${name}.awk" ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE text ERROR_VARIABLE err)
  set(${out} "" PARENT_SCOPE)
  set(${info} "" PARENT_SCOPE)
  if(NOT rc EQUAL 0)
    set(${out} "awk ${name} exited ${rc}: ${err}" PARENT_SCOPE)
    return()
  endif()
  string(REPLACE "\n" ";" lines "${text}")
  set(errors "")
  foreach(line IN LISTS lines)
    if(line MATCHES "^info (.*)$")
      set(${info} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    elseif(NOT line STREQUAL "")
      string(APPEND errors "${line}. ")
    endif()
  endforeach()
  set(${out} "${errors}" PARENT_SCOPE)
endfunction()

function(wl_count out info ledger catalog counts work)
  wl_awk(errors infotext "${work}" count "${wl_count_awk}" "${counts}" "${catalog}" "${ledger}")
  set(${out} "${errors}" PARENT_SCOPE)
  set(${info} "${infotext}" PARENT_SCOPE)
endfunction()

function(wl_reason out info ledger catalog counts work)
  wl_awk(errors infotext "${work}" reason "${wl_reason_awk}" "${counts}" "${catalog}" "${ledger}")
  set(${out} "${errors}" PARENT_SCOPE)
  set(${info} "${infotext}" PARENT_SCOPE)
endfunction()

function(wl_table out info table catalog work)
  wl_awk(errors infotext "${work}" table "${wl_table_awk}" "${table}" "${catalog}")
  set(${out} "${errors}" PARENT_SCOPE)
  set(${info} "${infotext}" PARENT_SCOPE)
endfunction()

# The seed over a ledger and a catalog must give the ledger back byte for byte; a ledger that is not the seed's output is not whole.
function(wl_fresh out ledger catalog work)
  file(MAKE_DIRECTORY "${work}")
  execute_process(COMMAND sh "${wl_scripts}/compat-seed.sh" "${ledger}" "${catalog}" 1970-01-01
    OUTPUT_FILE "${work}/seeded.csv" RESULT_VARIABLE rc ERROR_VARIABLE err)
  set(${out} "" PARENT_SCOPE)
  if(NOT rc EQUAL 0)
    set(${out} "ledger is not whole: the seed exits ${rc}: ${err}" PARENT_SCOPE)
    return()
  endif()
  file(SHA256 "${ledger}" have)
  file(SHA256 "${work}/seeded.csv" want)
  if(NOT have STREQUAL want)
    set(${out} "ledger is not whole: the seed over it prints other bytes" PARENT_SCOPE)
  endif()
endfunction()

# A compound identity has no image: its id is the SHA-1 of the text mame/<list>/<name>, recomputed here by CMake, not by the C of the generator.
function(wl_compound out info catalog work)
  file(MAKE_DIRECTORY "${work}")
  file(CONFIGURE OUTPUT "${work}/compound.awk" CONTENT "${wl_compound_awk}" @ONLY NEWLINE_STYLE UNIX)
  execute_process(COMMAND awk -f "${work}/compound.awk" "${catalog}" OUTPUT_VARIABLE text RESULT_VARIABLE rc ERROR_VARIABLE err)
  set(${out} "" PARENT_SCOPE)
  set(${info} "" PARENT_SCOPE)
  if(NOT rc EQUAL 0)
    set(${out} "awk compound exited ${rc}: ${err}" PARENT_SCOPE)
    return()
  endif()
  string(REPLACE "\n" ";" lines "${text}")
  set(checked 0)
  foreach(line IN LISTS lines)
    if(line MATCHES "^([0-9a-f]+) (.+)$")
      string(SHA1 want "mame/${CMAKE_MATCH_2}")
      if(NOT CMAKE_MATCH_1 STREQUAL want)
        set(${out} "ledger is not whole: compound id ${CMAKE_MATCH_1} is not the SHA-1 of mame/${CMAKE_MATCH_2}" PARENT_SCOPE)
        return()
      endif()
      math(EXPR checked "${checked}+1")
    endif()
  endforeach()
  if(checked EQUAL 0)
    set(${out} "no compound identity was checked" PARENT_SCOPE)
  endif()
  set(${info} "compound ids checked=${checked}" PARENT_SCOPE)
endfunction()
