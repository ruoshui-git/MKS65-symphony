all:
	cd server && make
	cd client && make
.PHONY: clean all runserver

runserver:
	cd server && make autorun

clean:
	cd server && make clean
	cd client && make clean
