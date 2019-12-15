#!/usr/bin/env bash

OUTFILE=$1

cd $(dirname $0)

echo "// Automatically generated by cmake on $(date)" >$OUTFILE
echo "// DO NOT EDIT" >>$OUTFILE
echo >>$OUTFILE
echo '#include "internal.hh"' >>$OUTFILE
echo >>$OUTFILE
echo 'DWARFPP_BEGIN_NAMESPACE' >>$OUTFILE
echo >>$OUTFILE
python3 ../elf/enum-print.py <dwarf++.hh >>$OUTFILE
python3 \
  ../elf/enum-print.py -s _ -u --hex -x hi_user -x lo_user <data.hh >>$OUTFILE
echo 'DWARFPP_END_NAMESPACE' >>$OUTFILE
