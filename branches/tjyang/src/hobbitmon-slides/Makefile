# -----------------------------------------------------------------------------
#     M A K E F I L E     S C R I P T   S P E C I F I C A T I O N
# -----------------------------------------------------------------------------
# NAME
#       Makefile 
#
# REVISION HISTORY
#     08/04 2008 T.J. Yang Init
#     08/26 2008 T.J. Yang Add slides in text format  for content checking.
# 
# USAGE
#       make
#
# DESCRIPTION
#       This GNU makefile script will call up pdflatex to generate pdf file using beamer 
#       latex class.
#       Slides:
#        1. hobbit-slides-developer.pdf 
#        2. hobbit-slides-sysadmin.pdf 
#        3. hobbit-slides-intro.pdf 
#       It will upload the tex and generated pdf files to hobbitmon sourceforge.net site.
#
# ---------------------------- CONSTANT DECLARATION ---------------------------
#
# WHAT: Define exit status flags.
# WHY:  Easier to program with a mnemonic than with the numbers.
# 
#

SUCCESS:=0
ERROR:=1
WARNING:=2
PDFLATEX:=/usr/bin/pdflatex
PS2TXT:=/usr/bin/ps2ascii
## ---------------------------------- SCRIPT -----------------------------------
##

# WHAT: make slides and check in the modified files.

all: txt
#all: slides checkin


# WHAT: slides in text format
txt: pdf hobbit-slides-developer.txt  hobbit-slides-intro.txt \
        hobbit-slides-sysadmin.txt


# WHAT: slides in pdf format
pdf: hobbit-slides-developer.pdf  hobbit-slides-intro.pdf \
        hobbit-slides-sysadmin.pdf

# WHAT: Hobbit Introduction Slides
hobbit-slides-intro.pdf: hobbit-slides-intro.tex
	${PDFLATEX}  hobbit-slides-intro.tex


# WHAT: 
hobbit-slides-intro.txt: hobbit-slides-intro.pdf
	${PS2TXT}  hobbit-slides-intro.pdf  hobbit-slides-intro.txt


# WHAT: 
hobbit-slides-developer.txt: hobbit-slides-developer.pdf
	${PS2TXT}  hobbit-slides-developer.pdf  hobbit-slides-developer.txt

# WHAT: 
hobbit-slides-sysadmin.txt: hobbit-slides-sysadmin.pdf
	${PS2TXT}  hobbit-slides-sysadmin.pdf  hobbit-slides-sysadmin.txt


# WHAT: Hobbit Slides for Developer
hobbit-slides-developer.pdf: hobbit-slides-developer.tex
	${PDFLATEX}  hobbit-slides-developer.tex

# WHAT: Hobbit Slides for System Administrators
hobbit-slides-sysadmin.pdf: hobbit-slides-sysadmin.tex
	${PDFLATEX}  hobbit-slides-sysadmin.tex

# WHAT: clean up all the unwanted files.
clean:
	rm -rf 	*.{aux,log,nav,out,snm,toc,vrb} *~

# WHAT: use svn to checkin the changes and updated pdf file.
svncheckin: all
	svn commit -m "automatic checkin by Makefile,sorry for less informative reasons."

# WHAT:  use "svn status" command  to display files are not committed.
svnstatus: clean
	svn status
