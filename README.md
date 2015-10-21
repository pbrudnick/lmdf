Liga Mundial de Futbol

Grupo KARMA

Trabajo para c�tedra Sistemas Operativos UTN-FRBA 2005

## Compilar
En el directorio raiz del programa.
`make`
Los archivos binarios ser�n creados dentro del directorio "bin".
El archivo Log ser� borrado, durante la ejecuci�n del programa se crear� uno nuevo.

## Ejecutar
Ubicarse dentro del directorio /bin y ejecutar `./lmdf`
(Si se estuviese situado dentro de otro directorio no encontrara el archivo de configuracion)

## Archivo de configuraci�n
Primordialmente debe configurar la IP y PUERTO propios (locales) y a los cuales quiere conectarse (remotos).

## Lista de comandos
* new: Crear� un nuevo juego.
* join: Se unir� a un juego existente (utilizando la direcci�n remota del archivo de configuraci�n)
* addcity: Crear� una Ciudad (no se podr� crear m�s de una)
* addteam: Crear� un nuevo equipo de la Ciudad creada anteriormente.
* startgame: Iniciar� la simulaci�n del juego.
* endgame: Finaliza el juego.
* exit: Sale del juego.
* help: Muestra la ayuda en pantalla.

## Secuencia de ejecuci�n estandar
1. new o join (crear o unirse a un juego)
2. addcity
3. addteam (n veces, acotadas por el archivo de configuraci�n)
4. startgame (da la orden de iniciar el juego, todos los participantes de la red deben ejecutar el comando)
5. endgame
6. exit
