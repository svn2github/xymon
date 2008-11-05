#!/bin/bash
#-----------------------------------------------------------
# Program : hobbit-html.sh
# Purpose : Prepares an html file for use within the hobbit
#           monitor web system.
# Author  : Scott Smith <smith.js.8@pg.com>
#-----------------------------------------------------------
# * Tue 28 Nov 2005 Scott Smith <smith.js.8@pg.com>
# - Initial release.
#-----------------------------------------------------------
# I use this script as follows, starting in the */www/help
# directory:
#
# for file in *.html manpages/*.html manpages/man?/*.html
# do
#   cp $file $file.old
#   hobbit-html.sh --path /monitor $file > $file.new
#   mv $file.new $file
#   chown hobbit:hobbit $file $file.old
# done
#-----------------------------------------------------------

SYNTAX="syntax: ${0##*/} [--help] [--path prefix] htmlfile"

case "$1" in
-h|"")  echo "$SYNTAX"
        exit 1
        ;;
--help) echo "$SYNTAX"
        echo " where: --path is the path prefix where Hobbit is installed"
        echo
        echo "        This is not required if Hobbit is installed relative"
        echo "        to the root / path in the URL."
        echo
        echo "        Example: If Hobbit is installed in the /monitor URL,"
        echo "        then '--path /monitor' is required in order for the"
        echo "        menu system to find its files."
        echo
        exit 1
        ;;
--path) URL="$2"
        HTML="$3"
        ;;
*)      URL=""
        HTML="$1"
        ;;
esac

[ -f "$HTML" ] || { echo "error: not found - $HTML" ; exit 2 ; }

awk --assign URL="$URL" -- '
    /<[/][Tt][Ii][Tt][Ll][Ee]>/ {
        print
        style()
        next
    }
    /<[Bb][Oo][Dd][Yy]>/ {
        print
        body1()
        next
    }
    /<[/][Bb][Oo][Dd][Yy]>/ {
        body2()
        print
        next
    }
    { print }
    function style()
    {
        print ""
        print "<!-- Styles for the menu bar -->"
        print "<link rel=\"stylesheet\" type=\"text/css\" href=\"" URL "/menu/menu.css\">"
        print ""
    }
    function body1()
    {
        print ""
        print "<p>&nbsp;</p>  <!-- For the menu bar -->"
        print ""
    }
    function body2()
    {
        print ""
        print "<!-- menu script itself. you should not modify this file -->"
        print "<script type=\"text/javascript\" language=\"JavaScript\" src=\"" URL "/menu/menu.js\"></script>"
        print "<!-- items structure. menu hierarchy and links are stored there -->"
        print "<script type=\"text/javascript\" language=\"JavaScript\" src=\"" URL "/menu/menu_items.js\"></script>"
        print "<!-- files with geometry and styles structures -->"
        print "<script type=\"text/javascript\" language=\"JavaScript\" src=\"" URL "/menu/menu_tpl.js\"></script>"
        print "<script type=\"text/javascript\" language=\"JavaScript\">"
        print "        new menu (MENU_ITEMS, MENU_POS);"
        print "</script>"
        print ""
    }
' "$HTML"

#EOF
