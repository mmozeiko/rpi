#!/usr/bin/env fish
bash -c "printenv; source setup.sh $argv >&2; printenv" | sort | uniq -u | grep -v SHLVL | awk '{print "export " $0}' | source
