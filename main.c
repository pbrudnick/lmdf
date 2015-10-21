/*
	Ultima modificacion: 15/05 - 05:30
	Modificado por: Diego
*/

#include "./headers/global_includes.h"
#include "./headers/defines.h"
#include "./inc/config.c"
#include "./inc/log.c"
#include "./inc/socketservidor.c"
#include "./inc/socketcliente.c"
#include "./inc/varias.c"


int iFdRouter[2];


int main ( int iArgs,char *aArgs[] )
{
	/* Cantidad de argumentos leidos (en 1 para obviar el nombre de este binario) */
	int iArgsReaded = 1;

	/* FD de CIUDAD para enviar y recibir desde MAIN */
	int iFdCiudad;
	/* FD de CIUDAD que sera enviado por parametro para comunicarse con MAIN */
	char sFdCiudad[6];

	/* Toma el string ingresado por linea de comando */
	char sLineaComando[COMLEN];
	/* Retorna el resultado de la función Comandos() */
	int iResultadoComando;

	/* Toma por linea de comando el nombre del proceso CIUDAD a crear */
	char sNombreCiudad[COMLEN];
	/* Toma por linea de comando el nombre del proceso EQUIPO a crear */
	char sNombreEquipo[COMLEN];

	/* Para controlar y validar la cantidad creada de equipos y ciudades */
	int iCiudad = 0, iEquipos = 0;

	/* Estado inicial y final de la simulacion */
	int iGameStarted = FALSE;
	int iGameOver = FALSE;

	/* Estado del Router */
	int iTokenStarted = FALSE;

	/* PID de CIUDAD y ROUTER */
	pid_t pidCiudad, pidRouter;

	char sPortRouter[5+1];

	char sFdRouter_Ciudad[10];
	char sFdCiudad_Router[10];

	/*int iFdRouter[2];*/
	int iFdRouterCiudad[2];

	/* Para terminar el ciclo principal */
	int iTerminar = FALSE;

	/* Incremental para IdMensaje de cabecera */
	int iIdMensaje = 0;

	/* Estructura de datos para leer el archivo de configuracion */
	CFG Cfg;

	/* Cabeceras y Mensajes de comunicación */
	CABECERA_CIUDADEQUIPO CABECERA_CiudadEquipo;
	char iConfirmaCiudad;
	TIPO_11 strEquipo;
	char iConfirmaEquipo;
	char iConfirmaStartGame;

	/* ########################## SEÑALES BEGIN ########################## */

	/* Evitar <defunct> */
	struct sigaction strSa;

	signal (SIGHUP,ctrl_c);
	signal (SIGINT,ctrl_c);
	signal (SIGPIPE, SIG_IGN);
	signal (SIGALRM,ctrl_c);
	signal (SIGTERM,ctrl_c);
	signal (SIGTSTP,ctrl_c);
	signal (SIGTTIN,ctrl_c);
	signal (SIGTTOU,ctrl_c);
	signal (SIGXCPU,ctrl_c);
	signal (SIGXFSZ,ctrl_c);
	signal (SIGVTALRM,ctrl_c);
	signal (SIGPROF,ctrl_c);
	signal (SIGUSR1,SIG_IGN);
	signal (SIGUSR2,SIG_IGN);
	signal (SIGCHLD,muerte_hijo);

	/* Evita zombies */
	strSa.sa_handler = SIG_IGN;
	strSa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &strSa, NULL);

	/* ########################## SEÑALES END ########################## */

	/* ########################## INICIALIZACION BEGIN ########################## */

	system("clear");

	/*setcolor(TXT_BRIGHT, TXT_RED, TXT_BLACK);*/

	/* llamado a la función que carga el archivo de configuración*/
	if ( CargaConfig(&Cfg) == FAIL ) {
		printf( "\n\nERROR FATAL: No se puede cargar el archivo de configuracion\n\n");
		WriteLog( ERROR, "Main", "Error:1:No se puede cargar el archivo de configuracion" );
		return FAIL;
	}

	/* Mensaje de bienvenida */
	printf("\nBienvenido a LMDF -2.69 ultrabeta\n\nEscriba help para un listado de comandos\n\n");

	WriteLog( INFO, "Main", "Programa principal iniciado!" );

	/* ########################## INICIALIZACION END ########################## */


	/* ########################## ROUTER BEGIN ########################## */

	/*while ( (iFdEscucha = AbreSocket(++iPortRouter)) == -1 );*/
	/* Obtiene los FD para ROUTER */
	/*if ( GetFD_socketpair(&iFdRouter,sFdRouter) != OK ) {
		printf("ERROR: No se pudo crear los FD para ROUTER");
}*/

	/* Obtiene los FD para ROUTER */
	if ( socketpair(AF_UNIX,SOCK_STREAM,0,iFdRouter) == -1 ) {
		printf("ERROR: No se pudo crear los FD para ROUTER\n");
	}

	/* Obtiene los FD para ROUTER-CIUDAD */
	if ( socketpair(AF_UNIX,SOCK_STREAM,0,iFdRouterCiudad) == -1 ) {
		printf("ERROR: No se pudo crear los FD para ROUTER-CIUDAD\n");
	}

	/* Guarda el puerto del ROUTER como string para enviarselo a CIUDAD */
	sprintf( sPortRouter, "%d", Cfg.RouterLocalPort);
	sprintf( sFdRouter_Ciudad, "%d", iFdRouterCiudad[0]);

	/*sprintf( sFdRouterTmp, "%s", sFdRouter);*/

	/* Creo nuevo proceso ROUTER */
	pidRouter = fork();
	if ( ! pidRouter ) /* Estoy en el hijo (router) */
	{
		/* Parametros para ROUTER:
		1) FD de ROUTER
		2) El nombre de la router
		*/


		dup2(iFdRouter[0], FD_MAIN);

		/*
		dup2(iFdRouterCiudad[0], FD_CIUDAD);
		*/

		close(iFdRouter[1]);
		close(iFdRouterCiudad[1]);

		execl( "router", "Router", sPortRouter,sFdRouter_Ciudad, (char*)NULL );
		exit(0);
	}

	/* Estoy en el PADRE (MAIN) */
	/*close(iFdEscucha);*/
	close(iFdRouter[0]);
	close(iFdRouterCiudad[0]);

	/* ########################## ROUTER END ########################## */


	/* Entra en ciclo principal */
	while ( iTerminar == FALSE )
	{
		printf("LMDF$ ");

		if ( iArgsReaded < iArgs )
		{
			if ( iArgsReaded == 1 )
			{
				if ( strcmp(strtoupper(aArgs[1]),"NEW") == 0 ) {
					iResultadoComando = NEW;
					printf("new\n");
				} else {
					iResultadoComando = JOIN;
					printf("join\n");
				}

				iArgsReaded++;
			}
			else if ( iArgsReaded == 2 )
			{
				iResultadoComando = ADDCITY;
				printf("addcity\n");
			}
			else
			{
				iResultadoComando = ADDTEAM;
				printf("addteam\n");
			}
		}
		else
		{
			fgets( sLineaComando, COMLEN, stdin ); /* Se lee el comando */
			fflush( stdin );
			/* Se elimina el \n reemplazandolo por un \0 */
			Strip_newline ( sLineaComando );

			iResultadoComando = Comandos( sLineaComando );
		}

		switch ( iResultadoComando )
		{
			case EXIT: /* SALIDA DEL PROGRAMA */

				iTerminar = TRUE;
				WriteLog( INFO, "Main", "Programa principal finalizado!" );
				/* Se matan todos los procesos hijos */
				ctrl_c();
				/*esto mata al padre tambien.. revisar, no esta bien esto,
				no hace el clear y por consiguiente no hace el return 0*/

			break;


			case HELP: /* SE MUESTRA UN LISTADO DE PARAMETROS EN PANTALLA */

				Comandos_Help ( );

			break;

			case NEW: /* SE CREA UN JUEGO NUEVO */

				if ( iTokenStarted == FALSE )
				{

					WriteLog( INFO, "Main", "Se creo el juego!" );

					EnviaCabecera( iFdRouter[1], &iIdMensaje, TKN_NEW, 0 );

					iTokenStarted = TRUE;

				}
				else
				{
					printf("Ya existe el juego\n");
				}


			break;

			case JOIN: /* SE CONECTA A UN JUEGO EXISTENTE */

				if ( iTokenStarted == FALSE )
				{
					/****** Envia cabecera a ROUTER ******/
					if ( EnviaCabecera( iFdRouter[1], &iIdMensaje, TKN_JOIN, 0 ) == -1 )
					{
						WriteLog( ERROR, "Main", "Error:4: Error comando JOIN" );
						return FAIL;
					}

					WriteLog( INFO, "Main", "Se unio al juego!" );

					iTokenStarted = TRUE;
				}
				else
				{
					printf("Ya existe un juego\n");
				}

			break;


			case ADDCITY: /* SE AGREGA UNA CIUDAD NUEVA */

				if ( iTokenStarted == TRUE )
				{
					if ( iCiudad == 1 )
					{
						/* Solo se puede tener una ciudad */
						printf("Ya se ha creado una ciudad\n");
					}
					else
					{ /* Se agrega una nueva ciudad */

						printf("Nombre de ciudad: ");

						if ( iArgsReaded == 2 )
						{
							sprintf( sNombreCiudad, "%s", aArgs[2] );
							printf( "%s\n", sNombreCiudad );
							iArgsReaded++;
							sleep(1);
						}
						else
						{
							fgets( sNombreCiudad, COMLEN, stdin ); /* Se ingresa el nombre de la ciudad */
							/*scanf("%s", sNombreCiudad);*/
							fflush( stdin );
							/* Se elimina el \n reemplazandolo por un \0 */
							Strip_newline ( sNombreCiudad );
						}

						/* Obtiene los FD para CIUDAD */
						if ( GetFD_socketpair(&iFdCiudad,sFdCiudad) != OK ) {
							printf("ERROR: No se pudo crear los FD para CIUDAD");
						}

						sprintf( sFdCiudad_Router, "%d", iFdRouterCiudad[1]);

						/* Creo nuevo proceso CIUDAD */
						pidCiudad = fork();
						if ( ! pidCiudad ) /* Estoy en el hijo (ciudad) */
						{
							/* Parametros para CIUDAD:
									1) FD de CIUDAD
									2) El nombre de la ciudad
							*/
							/*execl( "ciudad", sNombreCiudad, sFdCiudad, sNombreCiudad, sPortRouter, (char*)NULL );*/

							/*dup2(iFdRouterCiudad[1], FD_ROUTER);*/
							/*dup2(iFdCiudad[1], FD_ROUTER); Cuando redireccionemos el Fd de Main a Ciudad */

							execl( "ciudad", sNombreCiudad, sFdCiudad, sFdCiudad_Router, (char*)NULL );

							exit(0);
						}

						/* Estoy en el padre (main) */
						close(iFdRouterCiudad[1]);

						/*sprintf( sFdRouter, "%s", sFdRouterTmp);*/

						/****** Recibe cabecera desde CIUDAD ******/
						if ( RecibeCabecera ( iFdCiudad, &CABECERA_CiudadEquipo ) == -1 )
						{
							WriteLog( ERROR, "Main", "Error:2:Error al recibir cabecera de Ciudad" );
							printf("Error fatal. Se finalizará la ejecución del programa\n");
							return FAIL;
						}
						else
						{
							/* Recibe confimacion de creacion de CIUDAD */

							if ( recv( iFdCiudad, &iConfirmaCiudad, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
							{
								WriteLog( ERROR, "Main", "Error:3:Error al recibir cabecera de Ciudad" );
								printf("Error fatal. Se finalizará la ejecución del programa\n");
								return FAIL;
							}
							else
							{
								if ( iConfirmaCiudad == OK )
								{ /* Se creo OK */
									printf("Se creo la ciudad\n");
									iCiudad++; /* Incrementa el contador de ciudades creadas */
								}
								else
								{ /* Error al crear */
									printf("No se pudo crear la ciudad\n");
									WriteLog( INFO, "Main", "No se pudo crear la ciudad" );
								}
							}
						}

						EnviaCabecera( iFdRouter[1], &iIdMensaje, ADDCITY, 0 );

					}
				}
				else
				{
					printf("Primero debe crear o unirse a un juego\n");
				}

			break;


			case ADDTEAM: /* SE AGREGA UN NUEVO EQUIPO */

				if ( iTokenStarted == TRUE )
				{

					if ( iCiudad != 1 )
					{ /* Aun no hay una ciudad creada */
						printf("Primero se debe crear una ciudad\n");
					}
					else if ( iEquipos >= Cfg.CantMaxEquipos )
					{ /* Ya estan creados todos los equipos */
						printf("Se alcanzo el maximo de equipos: %d\n", iEquipos);
					}
					else /* Se crea un nuevo equipo para la ciudad */
					{
						printf("Nombre de equipo: ");

						if ( iArgsReaded < iArgs && iArgsReaded > 2 )
						{
							sprintf( sNombreEquipo, "%s", aArgs[iArgsReaded] );
							printf( "%s\n", sNombreEquipo );
							sleep(1);

							iArgsReaded++;
						}
						else
						{
							fgets( sNombreEquipo, COMLEN, stdin ); /* Se ingresa el nombre del equipo */
							fflush( stdin );
							/* Se elimina el \n reemplazandolo por un \0 */
							Strip_newline ( sNombreEquipo );
						}

						/* Arma mensaje para CIUDAD de nuevo equipo a crear */
						sprintf( strEquipo.Equipo, "%s", sNombreEquipo );
						sprintf( strEquipo.KI, "0" );
						/*strEquipo.KI = 0;*/
						sprintf( strEquipo.Cansancio, "0" );
						/*strEquipo.Cansancio = 0;*/
						strEquipo.Nuevo = 1;
						/* La duracion es un random entre 2 valores predefinidos en el config */
						sprintf( strEquipo.DuracionPartidos, "%d", GetRand(Cfg.DuracionPartidosMIN, Cfg.DuracionPartidosMAX) );
						/*strEquipo.DuracionPartidos = GetRand ( Cfg.DuracionPartidosMIN, Cfg.DuracionPartidosMAX );*/

						/****** Envia cabecera a CIUDAD ******/
						if ( EnviaCabecera( iFdCiudad, &iIdMensaje, CME_EQUIPO, sizeof(strEquipo) ) == -1 )
						{
							WriteLog( ERROR, "Main", "Error:4:Error al enviar cabecera a Ciudad" );
							printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
							return FAIL;
						}
						else
						{
							/* Envia a CIUDAD los datos del nuevo equipo */
							if ( send( iFdCiudad, &strEquipo, sizeof(strEquipo), 0 ) == -1 )
							{
								WriteLog( ERROR, "Main", "Error:5:Error al enviar cabecera a Ciudad" );
								printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
								return FAIL;
							}
							else
							{
								/****** Recibe cabecera desde CIUDAD ******/
								if ( RecibeCabecera ( iFdCiudad, &CABECERA_CiudadEquipo ) == -1 )
								{
									WriteLog( ERROR, "Main", "Error:6:Error al recibir cabecera de Ciudad" );
									printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
									return FAIL;
								}
								else
								{
									/* Recibe confimacion de EQUIPO creado */
									if ( recv( iFdCiudad, &iConfirmaEquipo, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
									{
										WriteLog( ERROR, "Main", "Error:7:Error al recibir cabecera de Ciudad" );
										printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
										return FAIL;
									}
									else
									{
										if ( iConfirmaEquipo == OK )
										{ /* El EQUIPO se creo OK */
											printf("Se creo el equipo\n");
											iEquipos++; /* Incrementa el contador de equipos creados */
										}
										else
										{
											WriteLog( INFO, "Main", "No pudo crearse el equipo" );
											printf("No pudo crearse el equipo\n");
										}
									}
								}
							}
						}
					}
				}
				else
				{
					printf("Primero debe crear o unirse a un juego\n");
				}

			break;


			case STARTGAME: /* SE INICIA EL JUEGO */

				if ( iTokenStarted == TRUE )
				{

					if ( iGameStarted == FALSE && iCiudad && iEquipos ) { /* Si esta en condiciones de comenzar */

						/****** Envia cabecera a CIUDAD ******/
						if ( EnviaCabecera( iFdCiudad, &iIdMensaje, LEV_ENVIAR, 0 ) == -1 )
						{
							WriteLog( ERROR, "Main", "Error:8:Error al enviar cabecera a Ciudad" );
							printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
							return FAIL;
						}

						/****** Recibe cabecera desde CIUDAD ******/
						if ( RecibeCabecera ( iFdCiudad, &CABECERA_CiudadEquipo ) == -1 )
						{
							WriteLog( ERROR, "Main", "Error:9:Error al recibir cabecera de Ciudad" );
							printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
							return FAIL;
						}

						/* Recibe confimacion de CIUDAD de juego iniciado */
						if ( recv( iFdCiudad, &iConfirmaStartGame, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
						{
							WriteLog( ERROR, "Main", "Error:10:Error al recibir confirmación de Ciudad" );
							printf("Error fatal. La ciudad ha muerto!!. No se puede continuar la ejecución del programa\n");
							return FAIL;
						}

						if ( iConfirmaStartGame == OK )
						{ /* El juego comenzo OK */
							printf("\n\nHa comenzado el juego!\n\n");
							WriteLog( INFO, "Main", "El juego ha comenzado!!" );
							iGameStarted = TRUE;
						}
						else
						{ /* Error al comenzar el juego */
							printf("ERROR: No se pudo comenzar el juego\n");

							WriteLog( ERROR, "Main", "No se pudo comenzar el juego" );
						}

					}
					else if ( iGameStarted == TRUE )
					{
						printf("El juego ya ha comenzado!\n");
					}
					else if ( iCiudad != 1 )
					{
						printf("Se necesita crear una ciudad para comenzar el juego\n");
					}
					else
					{
						printf("Se necesitan equipos para comenzar el juego\n");
					}
				}
				else
				{
					printf("Primero debe crear o unirse a un juego\n");
				}

			break;



			case ENDGAME: /* SE TERMINA EL JUEGO */

				if ( iGameOver == FALSE )
				{
					if ( iTokenStarted == TRUE && iGameStarted == TRUE )
					{
						/****** Envia cabecera a ROUTER ******/
						if ( EnviaCabecera( iFdRouter[1], &iIdMensaje, TKN_END, 0 ) < 0 )
						{
							WriteLog( ERROR, "Main", "34324: No se pudo finalizar el juego" );
							return FAIL;
						}

						printf("\nFinalizando el juego..\n");
					}
					else
					{
						printf("Aun no se ha comenzado un juego!\n");
					}
				}
				else
					printf("El juego ya ha finalizado!\n");

				break;


			default: /* COMANDO INEXISTENTE */

				printf("Comando no encontrado\n");

			break;

		}
	}

	return 0;
}
