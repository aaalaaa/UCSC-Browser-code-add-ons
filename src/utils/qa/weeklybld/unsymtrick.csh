#!/bin/tcsh
#
if ( -e ~/bin/${MACHTYPE}.orig ) then
    mv ~/bin/$MACHTYPE ~/bin/${MACHTYPE}.cluster
    mv ~/bin/${MACHTYPE}.orig ~/bin/$MACHTYPE
else
    echo "symtrick: $MACHTYPE.orig not found."
endif
