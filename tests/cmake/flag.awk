# Reads is_experimental as RetroArch's config reader does: the last non-comment line wins, spacing, key and value case and quotes are tolerated, and any value but an explicit false is present. Prints present or absent.
tolower($0) ~ /^[ \t]*is_experimental[ \t]*=/ { v=tolower($0); sub(/^[^=]*=[ \t]*/,"",v); if(v ~ /^["']/) { v=substr(v,2); sub(/["'].*$/,"",v) } else sub(/[ \t\r]*(#.*)?$/,"",v); on=(v!="false") }
END { print (on ? "present" : "absent") }
