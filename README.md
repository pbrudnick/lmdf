Liga Mundial de Futbol

Grupo KARMA

Trabajo para cátedra Sistemas Operativos UTN-FRBA 2005

## Compilar
En el directorio raiz del programa.
`make`
Los archivos binarios serán creados dentro del directorio "bin".
El archivo Log será borrado, durante la ejecución del programa se creará uno nuevo.

## Ejecutar
Ubicarse dentro del directorio /bin y ejecutar `./lmdf`
(Si se estuviese situado dentro de otro directorio no encontrara el archivo de configuracion)

## Archivo de configuración
Primordialmente debe configurar la IP y PUERTO propios (locales) y a los cuales quiere conectarse (remotos).

## Lista de comandos
* new: Creará un nuevo juego.
* join: Se unirá a un juego existente (utilizando la dirección remota del archivo de configuración)
* addcity: Creará una Ciudad (no se podrá crear más de una)
* addteam: Creará un nuevo equipo de la Ciudad creada anteriormente.
* startgame: Iniciará la simulación del juego.
* endgame: Finaliza el juego.
* exit: Sale del juego.
* help: Muestra la ayuda en pantalla.

## Secuencia de ejecución estandar
1. new o join (crear o unirse a un juego)
2. addcity
3. addteam (n veces, acotadas por el archivo de configuración)
4. startgame (da la orden de iniciar el juego, todos los participantes de la red deben ejecutar el comando)
5. endgame
6. exit
