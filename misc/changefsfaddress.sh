#!/bin/sh

OLDADDRESS="59 Temple Place - Suite 330, Boston, MA  02111-1307, USA"
NEWADDRESS="51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"

for f in `grep -rl "$OLDADDRESS" src/* include/* gamera/* *.py LICENSE | grep -v '.svn/'`
#for f in bla.py
do
	echo "converting $f"
	sed "s/$OLDADDRESS/$NEWADDRESS/" $f > $f.new
	mv $f.new $f
done
