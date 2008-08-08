# -----------------------------------------------------------------------------
#     M A K E F I L E     S C R I P T   S P E C I F I C A T I O N
# -----------------------------------------------------------------------------
# NAME
#       Makefile 
#
# REVISION HISTORY
#     08/04 2008 T.J. Yang Init
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

## ---------------------------------- SCRIPT -----------------------------------
##

# WHAT: make slides and check in the modified files.

all: slides 
#all: slides checkin

# WHAT: 
slides: hobbit-slides-developer.pdf  hobbit-slides-intro.pdf \
        hobbit-slides-sysadmin.pdf

# WHAT: Hobbit Introduction Slides
hobbit-slides-intro.pdf: hobbit-slides-intro.tex
	pdflatex  hobbit-slides-intro.tex

# WHAT: Hobbit Slides for Developer
hobbit-slides-developer.pdf: hobbit-slides-developer.tex
	pdflatex  hobbit-slides-developer.tex

# WHAT: Hobbit Slides for System Administrators
hobbit-slides-sysadmin.pdf: hobbit-slides-sysadmin.tex
	pdflatex  hobbit-slides-sysadmin.tex

# WHAT: clean up all the unwanted files.
clean:
	rm -rf 	*.{aux,log,nav,out,snm,toc,vrb} *~

# WHAT: use svn to checkin the changes and updated pdf file.
checkin: 
	svn commit -m "automatic checkin by Makefile,sorry, I know it less infomative about why the changes."

# WHAT:  use "svn status" command  to display files are not committed.
svnstatus: clean
	svn status
