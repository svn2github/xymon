all: slides

#slides: hobbit-slides-intro.pdf hobbit-slides-developer.pdf hobbit-slides-sysadmin.pdf
slides: hobbit-slides-developer.pdf 

hobbit-slides-intro.pdf: hobbit-slides-intro.tex
	pdflatex  hobbit-slides-intro.tex

hobbit-slides-developer.pdf: hobbit-slides-developer.tex
	pdflatex  hobbit-slides-developer.tex

hobbit-slides-sysadmin.pdf: hobbit-slides-sysadmin.tex
	pdflatex  hobbit-slides-sysadmin.tex

clean:
	rm -rf 	*.{aux,log,nav,out,snm,toc,pdf} *~

upload: all 
	sudo rsync -avz *.pdf  /var/www/html/pdf

checkin: 
	svn commit 


