all : main router ciudad equipo cde movimientos

LIBSISOP = ./
main : ./main.c
	cc ./main.c -Wall -pedantic -I$(LIBSISOP) -L$(LIBSISOP) -o lmdf
router : ./inc/router.c
	cc ./inc/router.c -Wall -pedantic -I$(LIBSISOP) -L$(LIBSISOP) -o router
ciudad : ./inc/ciudad.c
	cc ./inc/ciudad.c -Wall -pedantic -I$(LIBSISOP) -L$(LIBSISOP) -o ciudad
equipo : ./inc/equipo.c
	cc ./inc/equipo.c -Wall -pedantic -I$(LIBSISOP) -L$(LIBSISOP) -o equipo
cde : ./inc/cde.c
	cc ./inc/cde.c -Wall -pedantic -I$(LIBSISOP) -L$(LIBSISOP) -o cde

movimientos:
	#--------
	# Se termino de compilar, realizando movimientos de archivos...
	#--------
	mv lmdf bin
	mv router bin
	mv ciudad bin
	mv equipo bin
	mv cde bin
	# Copiado del CFG dentro del directorio de binarios
	cp config.cfg ./bin/
	# Borrado del antiguo archivo de LOG
	rm -f logfile.log
