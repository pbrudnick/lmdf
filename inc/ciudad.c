/*
	Ultima modificacion: 21/06 - 02:00
	Modificado por: Astor Piazzola
*/

#include "./headers/global_includes.h"
#include "./headers/defines.h"
#include "./inc/config.c"
#include "./inc/log.c"
#include "./inc/socketservidor.c"
#include "./inc/socketcliente.c"
#include "./inc/varias.c"

int main ( int iArgs,char *aArgs[] )
{
	/* FD de EQUIPO para enviar y recibir desde CIUDAD */
	int iFdEquipo = -1;

	/*
			Cantidad de Equipos que posee esta Ciudad.
			Se utiliza para saber que cantidad de Equipos deben retornar luego de haber migrado.
	*/
	int iCantEquipos = 30000, iCantEquiposTmp = 0;

	/* FD Escucha para los Equipos que se quieran conectar para migrar */
	int iFdEscucha = 2;

	/* Equipo al cual se abre conexion */
	int iFdEquipoExtranjero = -1;

	/* FD de EQUIPO que sera enviado por parametro para comunicarse con CIUDAD */
	char sFdEquipo[6];

	/* FD de CDE */
	int iFdCDE = -1;

	/* FD de CDE que sera enviado por parametro para comunicarse con CIUDAD */
	char sFdCDE[6];

	/* FD de MAIN (sFdCiudad en main.c) */
	int iFdMain;

	/* Nombre de la ciudad creada */
	char *sNombreCiudad = NULL;

	/* Lev Resultados temporal recibido por un Equipo que migra */
	char *sLevResultados = NULL;

	/* PID de equipo y CDE */
	pid_t pidEquipo, pidCDE;

	/* Estado inicial de la simulacion */
	int iGameStarted = FALSE, iGameOverLEVinterna = FALSE, iGameOverLEVexterna = FALSE;

	/* Incremental para IdMensaje de Cabecera */
	int iIdMensaje = 0, iIdMensajeAux = 0;

	int iAlPedo = 0;

	/* Inicializa puntero a lista LEV */
	LEV *lstLevInterna = NULL, *lstLevExterna = NULL;
	LEVinfo lstLevInfo;

	/* Cadena de Lev encriptada */
	char *sLev = NULL;

	/* Puntero a lista de EQUIPOS */
	FDS *lstFdsAux = NULL, *lstFdsBeAux = NULL, *lstFdsEstAux = NULL, *lstFds = NULL, *lstFdTieneLEV = NULL;
	FDSinfo lstFdsInfo;

	/* ##### Cabeceras y Mensajes de comunicación ##### */
	CABECERA_CIUDADEQUIPO CABECERA_CiudadEquipo;	/* Cabecera CIUDAD-EQUIPO */
	CABECERA_ROUTERROUTER CABECERA_RouterRouter;	/* Cabecera ROUTER-ROUTER / CIUDAD-ROUTER */

	/* Cabecera Router_Router para mandar => 7 bytes */
	char *sCabeceraRouter = NULL;

	/* Tipos de mensajes */
	FOUND strDireccionCiudad;
	TIPO_4 strDireccionCDE;
	TIPO_7 strDireccionEquipo;
	char iConfirmaCiudad;	/* Tipo 16 */
	TIPO_11 strEquipo;
	char iConfirmaEquipo;	/* Tipo 12 */
	char iConfirmaStartGame = OK; /* Mensaje de confirmacion enviado a MAIN */
	DISCOVER strDiscover;

	/* Datos de Ubicacion de la CIUDAD */
	FOUND strAddrCiudad;

	/* FD Escucha Auxiliar/Temporal para los equipos y el CDE */
	int iFdEscuchaAux;

	/* Bolsa de descriptores */
	fd_set BolsaDescriptores;

	/* El valor del FD mas alto */
	int iMaximo;

	/* Bandera para envio de LEV */
	/*int iEnvioCorrecto;*/

	/* Flag para indicar si la LEV se envio o no */
	/*int iLevEnviada;*/

	/* El valor que devuelve el Select */
	int iSelectValRet;

	/* El resultado (exito/fallo) que devuelven algunas funciones */
	int iResult;

	/* El Puerto inicial para los equipos hijos y la Ciudad */
	int iPort = 20000, iPuertoCiudad;

	/* FD del Router */
	int iFdRouter;

	/* Cadena del Token Aplanado */
	char *sToken = NULL;

	/* Estructura del token */
	TOKEN strToken;

	/* Largo del Token */
	long int iLargoToken;

	/* Para recibir el nuevo estado de AE_ESTADO */
	int iEstadoRecibido;

	/* Datos del Equipo Campeon */
	int iEquipoCampeonPuntaje = -1;
	TKN_LISTATDP *pEquipoCampeon = NULL;

	/* Listas del TOKEN */

	/* Infos de nodos TOKEN */
	TKN_LISTACIUDADESinfo strInfoCiudades;
	/*TKN_LISTATDPinfo strInfoTdp;*/

	/* Punteros a TOKEN */
	/*
	TKN_LISTACIUDADES *lstTokenCiudades = NULL;
	*/
	TKN_LISTATDP *lstTokenTdp = NULL;

	/* Datos de la Ciudad a migrar */
	TKN_LISTACIUDADES *pCiudadMigra = NULL;

	/* Flag que indica si ya ha llegado el TOKEN a esta ciudad */
	int iTokenRecv = FALSE;

	int iHayEquiposNoMigrados = FALSE;

	/* Nombre de equipo a buscar */
	char sEquipo[30];

	/* String para guardar en el LOG */
	char sLog[250];

	/* KI para pasar a Equipo */
	char sKI[3+1];

	/* Cansancio para pasar a Equipo */
	char sCansancio[3+1];

	/* Duracion de partidos para pasar a Equipo */
	char sDuracionPartidos[5+1];

	/*lista de resultados separadas por pipes*/
	char *sListaResultados = NULL;

	/* Direccion de esta Ciudad para pasar a Equipo */
	char sIPCiudad[ 20 ];
	char sPortCiudad[ 7 ];

	/*para "recibir" el mensaje del cde muerto*/
	char sAlPedoCDE[1];

	/* Datos temporales de la Ciudad Origen para pasar a Equipo */
	char *sNombreCiudadOrigen = NULL;
	char *sIPCiudadOrigen = NULL;
	char *sPortCiudadOrigen = NULL;

	/* Lista de resultados del equipo */
	LEVRESULTADOS *lstLevResultados=NULL, *lstLevResultadosAux=NULL;

	/* Timeout para el Select */
	struct timeval strTimeOut;

	/* Timestamp */
	time_t tTimestamp;

	/* Lista de mensajes Discover enviados */
	DISCOVERSENT *lstDISCOVERsentAux, *lstDISCOVERsent = NULL;

	/* Estructura de datos para leer el archivo de configuracion */
	CFG Cfg;

	/* Evitar <defunct> */
	struct sigaction strSa;

	/* Para ignorar el SIGPIPE (si el send da -1) */
	signal(SIGPIPE, SIG_IGN);

	/* Evita zombies */
	strSa.sa_handler = SIG_IGN;
	strSa.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &strSa, NULL);


	/* ########################## INICIALIZACION START ########################## */

	/* FdEscucha de la CIUDAD */
	while ( (iFdEscucha = AbreSocket(++iPort)) < 1 );

	iPuertoCiudad = iPort;


	/* Obtiene el FD de MAIN pasado por parametro */
	sscanf( aArgs[1], "%d" , &iFdMain );
	/* Obtiene el nombre de la ciudad pasado por parametro */
	sNombreCiudad = malloc( strlen(aArgs[0]) + 1);
	strcpy( sNombreCiudad, aArgs[0] );

	sscanf( aArgs[2], "%d" , &iFdRouter );

	/* Carga de Configuracion */
	if ( CargaConfig(&Cfg) == FAIL ) {
		printf( "\n\nERROR FATAL: No se pudo cargar el archivo de configuracion\n\n");
		WriteLog( ERROR, "Ciudad", "-1- No se pudo cargar el archivo de configuracion" );
		return FAIL;
	}

	/* Reserva lugar para Cabecera de 7 bytes */
	sCabeceraRouter = malloc(LARGO_CABECERA_ROUTER);

	strAddrCiudad.IPCiudad = Cfg.RouterLocalIP;
	strAddrCiudad.PortCiudad = iPuertoCiudad;

	sprintf( sIPCiudad, "%s", Cfg.CiudadLocalIP );
	sprintf( sPortCiudad, "%d", iPuertoCiudad );

	/* ##### Se conecta al Router ##### */

	EnviaCabeceraRouter( iFdRouter, &iIdMensaje, CR_CONECTA, 0, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

	if ( send( iFdRouter, sNombreCiudad, strlen(sNombreCiudad)+1, 0 ) == -1 )
		WriteLog(ERROR, sNombreCiudad, "-2- No se pudo enviar el nombre de la ciudad al Router");

	/****** Envia cabecera a MAIN ******/
	if ( EnviaCabecera( iFdMain, &iIdMensaje, CC_CONFIRMACION, sizeof(iConfirmaCiudad) ) == -1 )
		WriteLog(ERROR, sNombreCiudad, "-3- No se pudo enviar cabecera a Main");
	else
	{
		/* Envía a MAIN el mensaje de confirmacion */
		iConfirmaCiudad = OK;

		if ( send( iFdMain, &iConfirmaCiudad, sizeof(iConfirmaCiudad), 0 ) == -1 )
			WriteLog(ERROR, sNombreCiudad, "-4- No se pudo enviar confirmacion a Main");
		else
		{

			WriteLog( INFO, sNombreCiudad, "Ciudad iniciada" );

			/* ########################## INICIALIZACION END ########################## */



			/* ########################## CAMPO DE ENTRENAMIENTOS :: BEGIN ##################### */

			/* Obtiene los FD para el CDE */
			if ( GetFD_socketpair( &iFdCDE, sFdCDE ) != OK ) {
				printf("-- CIUDAD: ERROR - No se pudo crear el FD para CDE\n");
				WriteLog(ERROR, sNombreCiudad, "-5- No se pudo crear el FD para CDE");
			}

			while ( (iFdEscuchaAux = AbreSocket(++iPort)) < 1 );


			/* Llena Direccion del CDE */
			strDireccionCDE.IP = 0;
			strDireccionCDE.Port = iPort;

			/* Creo nuevo proceso CDE */
			pidCDE = fork();

			if ( ! pidCDE ) /* Estoy en el hijo (CDE) */
			{
				/* Parametros para CDE:
				1) FD de CDE
				*/
				dup2(iFdEscuchaAux, FD_CDE);

				/*cambio FH*/
				close ( iFdCDE );

				execl( "cde", "CDE", sFdCDE, (char*)NULL );
				exit(0);
			}

			/* Estoy en el padre (ciudad) */

			close(iFdEscuchaAux);
			/*cambio FH--*/
			close( atoi(sFdCDE) );

			/* ########################## CAMPO DE ENTRENAMIENTOS :: END ##################### */

			/* Entra en ciclo principal */
			while ( 1 )
			{

				/* Reinicia bolsa descriptores */
				FD_ZERO(&BolsaDescriptores);

				/* Agrega los FD de Escucha, Main y Router a la bolsa */
				FD_SET(iFdMain, &BolsaDescriptores);
				FD_SET(iFdRouter, &BolsaDescriptores);
				FD_SET(iFdEscucha, &BolsaDescriptores);

				if (iFdCDE != -1 )
					FD_SET(iFdCDE, &BolsaDescriptores);

				iMaximo = 0;

				/* Carga FDs de Equipos a la bolsa */
				lstFdsAux = lstFds;
				while ( lstFdsAux != NULL )
				{
					if ( lstFdsAux->info.FdEquipo > iMaximo )
						iMaximo = lstFdsAux->info.FdEquipo;

					if ( lstFdsAux->info.FdEquipo != -1 )
						FD_SET ( lstFdsAux->info.FdEquipo, &BolsaDescriptores );

					lstFdsAux = (FDS *) lstFdsAux->sgte;
				}


				/* Setea el maximo FD */
				if ( iMaximo < iFdMain )
					iMaximo = iFdMain;

				if (iMaximo < iFdRouter)
					iMaximo = iFdRouter;

				if ( iMaximo < iFdEscucha ) {
					iMaximo = iFdEscucha;
				}

				/* Seteado del TIMEOUT para el Select */
				strTimeOut.tv_sec = 1; /* 1 segundo */
				strTimeOut.tv_usec = 0;

				/* SELECT */
				iSelectValRet = select( iMaximo+1, &BolsaDescriptores, NULL, NULL, &strTimeOut );


				/* Mensaje recibido desde algun FD de la bolsa */
				if ( iSelectValRet > -1 )
				{

			/**************************************************************************************/
			/* #################### MURIO CDE :: BEGIN #################### */
			/**************************************************************************************/
					if ( iFdCDE!=-1 &&  FD_ISSET(iFdCDE,&BolsaDescriptores) )
					{
						/*habla el cde, porque esta muerto, nos devuelve -1*/
						if ( recv( iFdCDE, sAlPedoCDE, 1, 0 ) < 1 )
						{
							/*murio el CDE*/

							printf("\n\n++++++ MURIO EL CDE ++++++++\n\n");
							WriteLog( ERROR, sNombreCiudad, "-143423- Murio el CDE" );

							close(iFdCDE);
							iFdCDE = -1;

							lstFdsBeAux = lstFds;

							while ( lstFdsBeAux != NULL )
							{

								if ( lstFdsBeAux->info.Estado == ENTRENANDO || lstFdsBeAux->info.Estado == DESCANSANDO )
									lstFdsBeAux->info.Estado = INACTIVO;

								EnviaCabecera( lstFdsBeAux->info.FdEquipo, &iIdMensaje, BE_CDE_MUERTO, 0 );

								lstFdsBeAux = (FDS *) lstFdsBeAux->sgte;

							}
						}
					}
			/**************************************************************************************/
			/* #################### MURIO CDE :: END #################### */
			/**************************************************************************************/

			/**************************************************************************************/
			/* #################### HABLO ROUTER :: BEGIN #################### */
			/**************************************************************************************/
					if ( FD_ISSET(iFdRouter,&BolsaDescriptores) )
					{
						/****** Recibe cabecera desde MAIN ******/
						if ( RecibeCabeceraRouter(iFdRouter, &sCabeceraRouter, &CABECERA_RouterRouter) > 0 )
						{
							/* Acciones que debe realizar segun TipoMensaje */
							switch ( CABECERA_RouterRouter.TipoMensaje )
							{

						/**************************************************************************************/
						/**/case RRC_END:		/****************** IMPRIME RESULTADOS DE LA LMDF :: BEGIN ****/
						/**************************************************************************************/

								sToken = (char *) malloc ( CABECERA_RouterRouter.LargoMensaje );
								if ( recv( iFdRouter, sToken, CABECERA_RouterRouter.LargoMensaje, 0 ) < 1 )
									printf("-- CIUDAD: Token no recibido\n");

								/* Obtiene la estructura TOKEN */
								DesaplanaToken ( &strToken, sToken );

								printf("\n--- EL JUEGO HA FINALIZADO! ---\n");
								sleep(2);

								printf("\n########## RESULTADOS DE LA LIGA MUNDIAL DE FUTBOL ##########\n\n");

								/*
								printf("Lita e Ciudae:\n");
								lstTokenCiudades = (TKN_LISTACIUDADES *) strToken.lstCiudades;
								while ( lstTokenCiudades != NULL )
								{
									printf("%d) \"%s\" ( %ld : %d )\n",
									lstTokenCiudades->info.Orden,
									lstTokenCiudades->info.Ciudad,
									lstTokenCiudades->info.IP,
									lstTokenCiudades->info.Port );

									lstTokenCiudades = (TKN_LISTACIUDADES *) lstTokenCiudades->sgte;
								}
								*/

								printf("\n:: EQUIPO ::\t:: CIUDAD ::\t:: PUNTAJE ::\t:: PARTIDOS ::\n\n");
								lstTokenTdp = (TKN_LISTATDP *) strToken.lstTdp;
								while ( lstTokenTdp != NULL )
								{
									printf("%s\t%s\t%d\tG:%d-E:%d-P:%d\n",
														lstTokenTdp->info.Equipo,
														lstTokenTdp->info.Ciudad,
														lstTokenTdp->info.Puntaje,
														lstTokenTdp->info.PartidosGanados,
														lstTokenTdp->info.PartidosEmpatados,
														lstTokenTdp->info.PartidosPerdidos );

									if ( lstTokenTdp->info.Puntaje > iEquipoCampeonPuntaje )
									{
										iEquipoCampeonPuntaje = lstTokenTdp->info.Puntaje;
										pEquipoCampeon = (TKN_LISTATDP *) lstTokenTdp;
									}

									lstTokenTdp = (TKN_LISTATDP *) lstTokenTdp->sgte;
								}

								printf("\n------- CAMPEON -------\n\n");
								printf("Equipo: \"%s\"\nCiudad: %s\nPuntos: %d\nPartidos Ganados: %d\nPartidos Empatados: %d\nPartidos Perdidos: %d\n",
														pEquipoCampeon->info.Equipo,
														pEquipoCampeon->info.Ciudad,
														pEquipoCampeon->info.Puntaje,
														pEquipoCampeon->info.PartidosGanados,
														pEquipoCampeon->info.PartidosEmpatados,
														pEquipoCampeon->info.PartidosPerdidos );


						/**************************************************************************************/
						/**/break;	/******************** IMPRIME RESULTADOS DE LA LMDF :: END *********************************/
						/**************************************************************************************/


						/**************************************************************************************/
						/**/case RRC_TOKEN:		/****************** RECIBE TOKEN :: BEGIN ***********************/
						/**************************************************************************************/

								sToken = malloc ( CABECERA_RouterRouter.LargoMensaje );
								if ( recv( iFdRouter, sToken, CABECERA_RouterRouter.LargoMensaje, 0 ) < 1 )
									printf("-- CIUDAD: Token no recibido\n");
								else
									printf("-- CIUDAD: Recibe TOKEN!\n");

								/* Obtiene la estructura TOKEN */
								DesaplanaToken ( &strToken, sToken );

								/*
								lstTokenTdp = (TKN_LISTATDP *) strToken.lstTdp;
								while ( lstTokenTdp != NULL )
								{
									printf("%s\t%s\t%d\tG:%d-E:%d-P:%d\n",
												 lstTokenTdp->info.Equipo,
												 lstTokenTdp->info.Ciudad,
												 lstTokenTdp->info.Puntaje,
												 lstTokenTdp->info.PartidosGanados,
												 lstTokenTdp->info.PartidosEmpatados,
												 lstTokenTdp->info.PartidosPerdidos );

									if ( lstTokenTdp->info.Puntaje > iEquipoCampeonPuntaje )
									{
										iEquipoCampeonPuntaje = lstTokenTdp->info.Puntaje;
										pEquipoCampeon = (TKN_LISTATDP *) lstTokenTdp;
									}

									lstTokenTdp = (TKN_LISTATDP *) lstTokenTdp->sgte;
								}
								*/

								/******* Elimina posibles Equipos MIGRADOS en esta Ciudad ********/
								lstFdsAux = lstFds;
								while ( lstFdsAux != NULL )
								{
									/* Se encontro un Equipo MIGRADO */
									if ( lstFdsAux->info.Migrado == TRUE )
									{
										/* Llama a la funcion para que procese la muerte del equipo */
										/*CIUDAD_MuereEquipo ( &lstFds, &lstLevInterna, &sLev, lstFdsAux, &iFdCDE, sNombreCiudad );*/

										/* Pide al CDE que expulse al equipo que ha muerto (sin chequear si esta o no) */
										CIUDAD_CDE_ExpulsaEquipo ( &iFdCDE, lstFdsAux->info.Equipo, sNombreCiudad );
										sleep(1);

										close( lstFdsAux->info.FdEquipo );

										/* Elimina el Equipo de la lista FDSs */
										lstFds = SuprimeNodoFDS( lstFds, lstFdsAux->info.Equipo);

										lstFdsAux = lstFds;
									}
									else
										lstFdsAux = (FDS *) lstFdsAux->sgte;
								}

								if ( iTokenRecv == TRUE ) /* Si la Ciudad ya esta cargada al Token.. */
								{

									/* Se setea el contador de nuevos Equipos */
									/*iCantEquipos = 0;*/

									/* Elimina LEVinterna vieja */
									while ( lstLevInterna != NULL )
										lstLevInterna = SuprimeNodoLEV( lstLevInterna, lstLevInterna->info.Equipo );

									/************ Crea LEV INTERNA *************/
									lstFdsAux = lstFds;
									while ( lstFdsAux != NULL )
									{
										if ( lstFdsAux->info.TokenSet == FALSE ) /* Se debe agregar al Equipo nuevo */
										{
											/* Hay nuevos Equipos para jugar => Se debe habilitar ambos torneos (interno y externo) */
											iGameOverLEVinterna = FALSE;
											iGameOverLEVexterna = FALSE;

										}

										/* Guarda los datos en el INFO del NODO de la LEV */
										sprintf( lstLevInfo.Equipo, "%s", lstFdsAux->info.Equipo );
										sprintf( lstLevInfo.CiudadOrigen, "%s", sNombreCiudad );

										/* Actualiza la lista de equipos de la CIUDAD */
										lstLevInterna = InsertaNodoLev( lstLevInterna, lstLevInfo );

										lstFdsAux = (FDS *) lstFdsAux->sgte;
									}

								}

								/* Agrega Equipos nuevos creados que aun no se ingresaron al TOKEN (si los hay) */
								AgregaEquiposToken( lstFds, &strToken, sNombreCiudad );

								/* ########## Verifico si hay que ingresar esta CIUDAD al TOKEN ########## */
								if ( iTokenRecv == FALSE )
								{
									/* Ingresamos esta ciudad a la lista de ciudades del TOKEN */
									sprintf( strInfoCiudades.Ciudad, "%s" , sNombreCiudad);

									/* OJO: La lectura del CFG aca puede causar datos "desincronizados" */
									strInfoCiudades.IP = Cfg.RouterLocalIP;
									strInfoCiudades.Port = Cfg.RouterLocalPort;

									strToken.lstCiudades = (struct TKN_LISTACIUDADES *) InsertaNodoTokenCiudades( (TKN_LISTACIUDADES *)strToken.lstCiudades, strInfoCiudades );

									/* Si ya se hizo un "startgame".. */
									if ( iGameStarted == TRUE )
									{
										/* Convierte TOKEN a cadena */
										iLargoToken = AplanaToken ( strToken, &sToken );

										/* Devuelve el TOKEN al Router */
										EnviaCabeceraRouter( iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

										if ( send( iFdRouter, sToken, iLargoToken, 0 ) < 1 )
											WriteLog( ERROR, sNombreCiudad, "-6- No se pudo enviar el Token" );
									}

									/* Se actualiza el estado de TOKEN recibido */
									iTokenRecv = TRUE;
								}

								/* ########## Se juega la LEV INTERNA ########## */
								else if ( iGameOverLEVinterna != TRUE )
								{
									printf("\n-- CIUDAD: Iniciando juego con LEV INTERNA en 5 segundos\n");
									sleep( 5 );



									/* ENVIO DE LEV AL PRIMER EQUIPO DE LA LISTA */
									lstFds = CIUDAD_EnviaLevEquipo (
																	lstFds, &lstFdTieneLEV, &lstLevInterna,
																	&iFdCDE, &iIdMensaje, sNombreCiudad, &iResult, ES_LEV_INTERNA,&iAlPedo
																	);

									if ( iResult == FAIL )
									{
										/* Ocurrio un error GRAVE (como la muerte del CDE) */
										/*
										ACA DEBERIA ENVIARLE UN MENSAJE A MAIN DICIENDO QUE NO SE PUEDE CONTINUAR, ASI MAIN CIERRA TODITO
										*/
										ctrl_c();

										return FAIL;
									}

									if ( lstFdTieneLEV == NULL ) /* No se envio a LEV a ningun Equipo */
									{
										/* No hay equipos, reenvio de TOKEN al Router */

										printf("\n\nSE TERMINO DE JUGAR EL TORNEO INTERNO/EXTERNO (no hay Equipos)\n\n");
										WriteLog(INFO, sNombreCiudad, "Se termino el torneo interno/externo (no hay Equipos)");

										EnviaCabeceraRouter( iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

										if ( send( iFdRouter, sToken, iLargoToken, 0 ) < 1 )
											WriteLog( ERROR, sNombreCiudad, "-6- No se pudo enviar el Token" );

										/* Como no hay Equipos.. => Se finalizan ambos torneos */
										iGameOverLEVinterna = TRUE;
										iGameOverLEVexterna = TRUE;

										Setea_FDS_FinPasadaToken( &lstFds );
									}

								}

								/* ########## Se juega la LEV EXTERNA ########## */
								else if ( iGameOverLEVexterna != TRUE )
								{

								}

								/* ########## Termino el uso del TOKEN en esta Ciudad ########## */
								else
								{
									/* Vuelve al Router porque no hay nuevos equipos */
									iLargoToken = AplanaToken ( strToken, &sToken );

									EnviaCabeceraRouter(iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD);
									if ( send(iFdRouter, sToken, iLargoToken, 0) < 1 )
										WriteLog(ERROR, sNombreCiudad, "-36- No se pudo enviar el Token");
								}


						/**************************************************************************************/
						/**/break;	/******************** RECIBE TOKEN :: END *********************************/
						/**************************************************************************************/

						/**************************************************************************************/
						/**/case BE_DISCOVER:	/*********** MENSAJE DISCOVER :: BEGIN ***************/
						/**************************************************************************************/

							/* Recibe DISCOVER del Router, chequea si el Equipo buscado existe en esta Ciudad */
							if ( recv( iFdRouter, &strDiscover, CABECERA_RouterRouter.LargoMensaje, 0 ) == -1 )
							{
								WriteLog( ERROR, "Ciudad", "Error:1228: Error al recibir Discover de Router" );
							}
							else
							{
								/* Busca entre sus equipos (FDS) si lo tiene */
								lstFdsAux = lstFds;

								while ( lstFdsAux != NULL && strcmp(lstFdsAux->info.Equipo, strDiscover.Equipo) )
								{
									lstFdsAux = (FDS *) lstFdsAux->sgte;
								}

								if (lstFdsAux != NULL)
								{
									printf("-- CIUDAD: Enviando FOUND a Router..\n");
									/* Devuelve el FOUND al Router */

									/* Guarda la IP y Puerto de esta Ciudad */
									strDireccionCiudad = strAddrCiudad;

									EnviaCabeceraRouter( iFdRouter, &(CABECERA_RouterRouter.IdMensaje), BE_FOUND, sizeof(strDireccionCiudad), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

									if ( send( iFdRouter, &strDireccionCiudad, sizeof(strDireccionCiudad), 0 ) < 1 )
										WriteLog( ERROR, sNombreCiudad, "-6- No se pudo enviar el Token" );
								}
							}

						/**************************************************************************************/
								/**/break;		/*					MENSAJE DISCOVER :: END				 **/
						/**************************************************************************************/



						/**************************************************************************************/
						/**/case BE_FOUND:	/*********** MENSAJE FOUND :: BEGIN ***************/
						/**************************************************************************************/

								/* Recibe FOUND del Router. Avisamos a nuestro Equipo para que migre.. */
								if ( recv( iFdRouter, &strDireccionCiudad, CABECERA_RouterRouter.LargoMensaje, 0 ) == -1 )
								{
									WriteLog( ERROR, "Ciudad", "Error:1228: Error al recibir Found de Router" );
								}
								else
								{
									if ( strDireccionCiudad.IPCiudad != 0 )
									{
										printf("-- CIUDAD: FOUND recibido.. Se encontro Ciudad/Equipo\n");

										lstDISCOVERsentAux = lstDISCOVERsent;
										/* Recorre la lista en busqueda de mensajes DISCOVER buscando por IDMENSAJE */
										while ( lstDISCOVERsentAux != NULL && lstDISCOVERsentAux->info.IdMensaje != CABECERA_RouterRouter.IdMensaje )
										{
											lstDISCOVERsentAux = (DISCOVERSENT *) lstDISCOVERsentAux->sgte;
										}


										if ( lstDISCOVERsentAux != NULL ) /* Se encontro a que Equipo pertenece el Mensaje */
										{
											/* Se envian los datos de Ubicacion de la Ciudad del Equipo buscado */

											EnviaCabecera( lstDISCOVERsentAux->info.FdOrigen, &iIdMensaje, BE_DIRECCION_CIUDAD, sizeof(strDireccionCiudad) );
											/* Envia un mensaje al Equipo Origen del mensaje avisando que se encontro */
											if ( send( lstDISCOVERsentAux->info.FdOrigen, &strDireccionCiudad, sizeof(strDireccionCiudad), 0) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-1456- No se pudo enviar mensaje DIRECCION CIUDAD a Equipo");

											/* Se elimina el mensaje de la lista */
											lstDISCOVERsent = SuprimeNodoDiscoverSent( lstDISCOVERsent, lstDISCOVERsentAux );
										}

									}
									else
									{
										/* No la encontro, entonces va al BE_NO */
										WriteLog(ERROR, sNombreCiudad, "-18- No se pudo enviar cabecera a Equipo");

										if ( send( lstFdsAux->info.FdEquipo, "0", 3, 0) < 1 )
											WriteLog(ERROR, sNombreCiudad, "-19- No se pudo enviar la informacion del equipo a buscar");

									}
								}

						/**************************************************************************************/
						/**/break;		/*					MENSAJE DISCOVER :: END				 **/
							/**************************************************************************************/
						} /* END :: switch ( CABECERA_RouterRouter.TipoMensaje ) */
					} /* END :: if ( RecibeCabeceraRouter(iFdRouter,&CABECERA_RouterRouter) > 0 ) */
				} /* END :: if ( FD_ISSET(iFdRouter,&BolsaDescriptores) ) */


			/**************************************************************************************/
			/* #################### HABLO ROUTER :: END #################### */
			/**************************************************************************************/


			/**************************************************************************************/
			/* #################### HABLO MAIN :: BEGIN #################### */
			/**************************************************************************************/
					if ( FD_ISSET(iFdMain,&BolsaDescriptores) )
					{
						/****** Recibe cabecera desde MAIN ******/
						if ( RecibeCabecera(iFdMain,&CABECERA_CiudadEquipo) > 0 )
						{
							/* Acciones que debe realizar segun TipoMensaje */
							switch ( CABECERA_CiudadEquipo.TipoMensaje )
							{

						/**************************************************************************************/
						/**/case CME_EQUIPO:	/*************** CREAR NUEVO EQUIPO :: BEGIN ******************/
						/**************************************************************************************/

									/* Redibe desde MAIN los datos del nuevo equipo a crear */
									if ( recv( iFdMain, &strEquipo, CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1 )
										WriteLog(ERROR, sNombreCiudad, "-7- No se pudo recibir los datos del equipo a crear");

									/* Obtiene los FD para el nuevo EQUIPO */
									if ( GetFD_socketpair(&iFdEquipo,sFdEquipo) != OK ) {
										printf("-- CIUDAD: ERROR - No se pudo crear el FD para EQUIPO\n");
										WriteLog(ERROR, sNombreCiudad, "-8- No se pudo crear el FD para EQUIPO");
									}

									while ((iFdEscuchaAux = AbreSocket(++iPort)) < 1);

									sprintf(sKI, "%s" , strEquipo.KI);
									sprintf(sCansancio, "%s" , strEquipo.Cansancio);
									sprintf(sDuracionPartidos, "%s" , strEquipo.DuracionPartidos);

									/* Creo nuevo proceso EQUIPO */
									pidEquipo = fork();

									if ( ! pidEquipo ) /* Estoy en el hijo (equipo) */
									{
										/* Parametros para EQUIPO:
												1) FD de EQUIPO
												2) NOMBRE de EQUIPO
												3) Ki
												4) Cansancio
												5) Duracion de partidos
												6) Nombre de esta Ciudad
												7) Nombre de la Ciudad Origen (en este caso ambos nombres coinciden)
										*/
										dup2(iFdEscuchaAux, 2);
										/*cambio FH*/
										close ( iFdEquipo );

										execl( "equipo", strEquipo.Equipo, sFdEquipo, sKI, sCansancio, sDuracionPartidos, sNombreCiudad, sNombreCiudad, sIPCiudad, sPortCiudad, (char*)NULL );
										exit(0);
									}

									/* Estoy en el padre (ciudad) */

									/* Escribe en el LOG */
									sprintf( sLog, "Crea nuevo equipo \"%s\"",strEquipo.Equipo );
									WriteLog( INFO, sNombreCiudad, sLog );

									/* IMPORTANTE (por diego):
											Aca habria que usar otra variable, ya que iTokenRecv la usamos para verificar si el TOKEN
											fue enviado al menos UNA vez a esta Ciudad. Una vez que el TOKEN es recibido queda en TRUE para
											siempre. Por lo tanto, habria que usar un flag DISTINTO que se ponga en TRUE mientras el TOKEN este
											en esta Ciudad (cuando se recibe del Router) y en FALSE mientras no este (cuando se envia al Router)
									*/
/* CREO QUE NO HARIA FALTA CARGAR LOS DATOS ACA... (por diego) */
									/*if ( iTokenRecv == TRUE )
									{*/
										/* Cargo la info */
										/*sprintf( strInfoTdp.Ciudad, "%s", sNombreCiudad );
										sprintf( strInfoTdp.Equipo, "%s", strEquipo.Equipo );
										strInfoTdp.Puntaje = 0;
										strInfoTdp.PartidosGanados = 0;
										strInfoTdp.PartidosEmpatados = 0;
										strInfoTdp.PartidosPerdidos = 0;

										strToken.lstTdp = (struct TKN_LISTATDP *) InsertaNodoTokenTDP( (TKN_LISTATDP *)strToken.lstTdp, strInfoTdp );
									}*/

									/*cambio FH*/
									close ( atoi(sFdEquipo) );
									close(iFdEscuchaAux);

									/****** Envia cabecera a MAIN ******/
									if ( EnviaCabecera( iFdMain, &iIdMensaje, CME_CONFIRMACION, sizeof(iConfirmaEquipo) ) < 1 )
										WriteLog(ERROR, sNombreCiudad, "-9- No se pudo enviar cabecera a Main");

									/* Envia a MAIN la confirmacion de la creacion de nuevo equipo */
									iConfirmaEquipo = OK;

									if( send( iFdMain, &iConfirmaEquipo, sizeof(iConfirmaEquipo), 0 ) < 1)
										WriteLog(ERROR, sNombreCiudad, "-10- No se pudo enviar confirmacion de creacion de equipo");

									/* Guarda los datos en el INFO del NODO de la LEV */
									/*sprintf( lstLevInfo.Equipo, "%s", strEquipo.Equipo );
									sprintf( lstLevInfo.CiudadOrigen, "%s", sNombreCiudad );*/

									/* Actualiza la lista de equipos de la CIUDAD */
									/*lstLevInterna = InsertaNodoLev( lstLevInterna, lstLevInfo );*/

									/* Guarda los datos en el INFO del NODO de lista FDS */
									sprintf( lstFdsInfo.Equipo, "%s", strEquipo.Equipo );
									lstFdsInfo.FdEquipo = iFdEquipo;
									lstFdsInfo.Estado = INACTIVO;
									lstFdsInfo.Port = iPort;
									lstFdsInfo.LevSent = FALSE;
									lstFdsInfo.LevSentExt = FALSE;
									lstFdsInfo.TokenSet = FALSE;
									lstFdsInfo.Puntaje = 0;
									lstFdsInfo.Migrado = FALSE;

									/* Actualiza la lista FDS de Equipos de la CIUDAD */
									lstFds = InsertaNodoFDS( lstFds, lstFdsInfo );

						/**************************************************************************************/
						/**/break;		/*					CREAR NUEVO EQUIPO :: END					 **/
						/**************************************************************************************/


						/**************************************************************************************/
						/**/case LEV_ENVIAR:	/*********** STARTGAME :: BEGIN ***************/
						/**************************************************************************************/

									/* Comando "STARTGAME" en Main */

									/****** Envia cabecera a MAIN ******/
									if ( EnviaCabecera( iFdMain, &iIdMensaje, 0, sizeof(iConfirmaStartGame) ) < 1 )
									WriteLog(ERROR, sNombreCiudad, "-11- No se pudo enviar cabecera a Main");

									/* Envia a MAIN la confirmacion del juego comenzado */
									if ( send( iFdMain, &iConfirmaStartGame, sizeof(iConfirmaStartGame), 0 ) < 1)
										WriteLog(ERROR, sNombreCiudad, "-12- No se pudo enviar confirmacion de juego comenzado");

									sleep(1); /* Para asegurar que aparece "Ha comenzado el juego!" primero que nada =P */


									/************ Crea LEV INTERNA *************/
									lstFdsAux = lstFds;
									while ( lstFdsAux != NULL )
									{
										if (lstFdsAux->info.TokenSet == FALSE)
										{
											/* Guarda los datos en el INFO del NODO de la LEV */
											sprintf( lstLevInfo.Equipo, "%s", lstFdsAux->info.Equipo );
											sprintf( lstLevInfo.CiudadOrigen, "%s", sNombreCiudad );

											/* Actualiza la lista de equipos de la CIUDAD */
											lstLevInterna = InsertaNodoLev( lstLevInterna, lstLevInfo );

										}

										lstFdsAux = (FDS *) lstFdsAux->sgte;
									}

									iCantEquiposTmp = -1;

									if ( iTokenRecv == TRUE && iGameStarted == FALSE )
									{
										/* Agrega Equipos nuevos creados que aun no se ingresaron al TOKEN (si los hay) */
										AgregaEquiposToken( lstFds, &strToken, sNombreCiudad );

										/* Se convierte la estructura TOKEN a cadena */
										iLargoToken = AplanaToken ( strToken, &sToken );

										/* Envia el Token al Router para que lo pase al proximo */
										EnviaCabeceraRouter( iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

										if ( send( iFdRouter, sToken, iLargoToken, 0 ) == -1 )
											WriteLog(ERROR, sNombreCiudad, "-13- No se pudo enviar el Token");

									}

									/* Se actualiza el FLAG de comienzo de juego */
									iGameStarted = TRUE;

						/**************************************************************************************/
						/**/break;		/*					STARTGAME :: END				 **/
						/**************************************************************************************/

							}/* END :: switch ( CABECERA_CiudadEquipo.TipoMensaje ) */
						} /* END :: if ( RecibeCabecera(iFdMain,&CABECERA_CiudadEquipo) > 0 ) */
						else
						{
							WriteLog(ERROR, sNombreCiudad, "-14- No se pudo recibir cabecera de Main");
						}
					} /* END :: if ( FD_ISSET(iFdMain,&BolsaDescriptores) ) */
			/**************************************************************************************/
			/* #################### HABLO MAIN :: END #################### */
			/**************************************************************************************/


			/**************************************************************************************/
					/* ################ HABLO EQUIPO DE LA LISTA :: BEGIN ############# */
			/**************************************************************************************/

					lstFdsAux = lstFds;

					while ( lstFdsAux != NULL )
					{

						if ( lstFdsAux->info.FdEquipo != -1 && FD_ISSET(lstFdsAux->info.FdEquipo,&BolsaDescriptores) )
						{


							/****** Recibe cabecera desde EQUIPO ******/
							if ( RecibeCabecera(lstFdsAux->info.FdEquipo,&CABECERA_CiudadEquipo) > 0 )
							{

								/* Acciones que debe realizar segun TipoMensaje */
								switch (CABECERA_CiudadEquipo.TipoMensaje)
								{

							/**************************************************************************************/
							/**/case BE_BUSCAR:		/****************** BUSCAR EQUIPO :: BEGIN ****************/
							/**************************************************************************************/

										/* Recibe desde EQUIPO el nombre de equipo contrincante a buscar */
										if (recv( lstFdsAux->info.FdEquipo, sEquipo, CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1)
											WriteLog(ERROR, sNombreCiudad, "-15- No se pudo recibir nombre de equipo a buscar");

										/* Lista de Equipos Auxiliar para BUSCAR EQUIPO */
										lstFdsBeAux = lstFds;

										/* Busca FD y estado del equipo contrincante en lista de equipos LOCALES */
										while ( lstFdsBeAux != NULL && strcmp(sEquipo,lstFdsBeAux->info.Equipo) != 0 )
										{
											lstFdsBeAux = (FDS *)lstFdsBeAux->sgte;
										}

										if ( lstFdsBeAux != NULL ) /* Se encontro el equipo buscado (LOCALMENTE) */
										{

											/* Arma mensaje para EQUIPO, avisando que el equipo buscado es LOCAL */

											strDireccionCiudad.IPCiudad = 0; /* de Ciudad */
											strDireccionCiudad.PortCiudad = 0;

											/* Escribe en el LOG */
											/*
											COMENTO PORQUE ESCRIBE 34573485 LINEAS
											sprintf( sLog, "Equipo \"%s\" ENCUENTRA a \"%s\"", lstFdsAux->info.Equipo, sEquipo );
											WriteLog( INFO, sNombreCiudad, sLog );
											*/

											/****** Envia cabecera a EQUIPO ******/
											EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, BE_DIRECCION_CIUDAD, sizeof(strDireccionCiudad) );

											/* Envia a EQUIPO los datos del equipo buscado y encontrado (en este caso LOCAL) */
											if (send( lstFdsAux->info.FdEquipo, &strDireccionCiudad, sizeof(strDireccionCiudad), 0 ) < 1)
												WriteLog(ERROR, sNombreCiudad, "-17- No se pudo enviar al equipo solicitante los datos del equipo solicitado");

										}
										else /* NO se encontro el equipo buscado (LOCALMENTE) => Buscamos afuera.. */
										{
											/* Busca la Ciudad en el TOKEN */
											pCiudadMigra = BuscaCiudadEnToken(strToken, sEquipo);

											if ( pCiudadMigra != NULL )
											{
												/* Arma mensaje DISCOVER */
												strcpy(strDiscover.Equipo, sEquipo);
												strcpy(strDiscover.Ciudad, pCiudadMigra->info.Ciudad);

												/* Nuevo IDMENSAJE para la cabecera (cambio Pablo)*/
												/*iIdMensajeAux = getpid() * 1000 + iIdMensaje++;*/
												iIdMensajeAux = GetRand(1, getpid()) + iIdMensaje++;

												/* Seteamos los valores iniciales para TTL y HOPS */
												CABECERA_RouterRouter.TTL = Cfg.TTL;
												CABECERA_RouterRouter.Hops = 0;

												EnviaCabeceraRouter( iFdRouter, &iIdMensajeAux, BE_DISCOVER, sizeof(DISCOVER), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

												if ( send( iFdRouter, &strDiscover, sizeof(DISCOVER), 0 ) < 1 )
													WriteLog(ERROR, sNombreCiudad, "-253- No se pudo enviar mensaje DISCOVER al Router");

												/* Inserta el mensaje en la lista de espera */
												lstDISCOVERsent = InsertaNodoDiscoverSent( lstDISCOVERsent, lstFdsAux->info.FdEquipo, iIdMensajeAux );

												printf("-- CIUDAD: DISCOVER enviado\n");

											}
											else
											{

												DesencriptaLevResultados( &lstLevResultados, sLevResultados );

												lstLevResultadosAux = lstLevResultados;

												while ( lstLevResultadosAux != NULL )
												{
													sprintf( sLog, "LEvResultados: Equipo: \"%s\" ", lstLevResultadosAux->info.Equipo );
													WriteLog( INFO, sNombreCiudad, sLog );

													lstLevResultadosAux = (LEVRESULTADOS *)lstLevResultadosAux->sgte;

												}


												lstLevResultadosAux = lstLevResultados;

												while ( lstLevResultadosAux != NULL && strcmp(sEquipo,lstLevResultadosAux->info.Equipo) != 0 )
												{

													lstLevResultadosAux = (LEVRESULTADOS *)lstLevResultadosAux->sgte;

												}

												if ( lstLevResultadosAux != NULL )
												{
													/* Arma mensaje DISCOVER */
													strcpy(strDiscover.Equipo, sEquipo);
													strcpy(strDiscover.Ciudad, lstLevResultadosAux->info.CiudadOrigen);

													/* Nuevo IDMENSAJE para la cabecera */
													/*iIdMensajeAux = getpid() * 1000 + iIdMensaje++;*/
													iIdMensajeAux = GetRand(1, getpid()) + iIdMensaje++;

													/* Seteamos los valores iniciales para TTL y HOPS */
													CABECERA_RouterRouter.TTL = Cfg.TTL;
													CABECERA_RouterRouter.Hops = 0;

													EnviaCabeceraRouter( iFdRouter, &iIdMensajeAux, BE_DISCOVER, sizeof(DISCOVER), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

													if ( send( iFdRouter, &strDiscover, sizeof(DISCOVER), 0 ) < 1 )
														WriteLog(ERROR, sNombreCiudad, "-253- No se pudo enviar mensaje DISCOVER al Router");

													/* Inserta el mensaje en la lista de espera */
													lstDISCOVERsent = InsertaNodoDiscoverSent( lstDISCOVERsent, lstFdsAux->info.FdEquipo, iIdMensajeAux );

													WriteLog(INFO, sNombreCiudad, "DISCOVER enviado");

													printf("-- CIUDAD: DISCOVER enviado\n");
												}
												else
												{

													/* No se encontro la Ciudad <=> el Equipo */
													EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, BE_NO, 0 );

													if ( send( lstFdsAux->info.FdEquipo, "0", 3, 0) < 1 )
														WriteLog(ERROR, sNombreCiudad, "-19- No se pudo enviar la informacion del equipo a buscar");
												}
											}


										}

							/**************************************************************************************/
							/**/break;		/*						BUSCAR EQUIPO :: END						 **/
							/**************************************************************************************/


							/**************************************************************************************/
							/**/case CDE_PEDIR:		/**************************************************************/
							/**************************************************************************************/
									/* Recibe desde EQUIPO el pedido de CDE */

									/* Primero se chequea si hay Equipos migrados en esta Ciudad */
									lstFdsEstAux = lstFds;
									while ( lstFdsEstAux != NULL && lstFdsEstAux->info.Migrado != TRUE )
									{
										lstFdsEstAux = (FDS *) lstFdsEstAux->sgte;
									}

									/* Si se encontraron Equipos migrados se debe habilitar el CDE para todos... */

									if ( lstFdsEstAux != NULL || (iGameStarted == TRUE && iGameOverLEVexterna == FALSE) )
									{
										if ( iFdCDE != -1 ) /* Si ESTA EL CDE */
										{
											if ( EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, CDE_DIRECCION, sizeof(strDireccionCDE) ) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-20- No se pudo enviar cabecera a Equipo");

											if ( send( lstFdsAux->info.FdEquipo, &strDireccionCDE, sizeof(strDireccionCDE), 0) < 1)
												WriteLog(ERROR, sNombreCiudad, "-21- No se pudo enviar al equipo la direccion del CDE");
											else
											{
												/* Escribe en el LOG */
												sprintf( sLog, "Equipo \"%s\" pide CDE", lstFdsAux->info.Equipo );
												WriteLog( INFO, sNombreCiudad, sLog );
											}
										}
										else /* SI SE CAYO CDE */
										{
											EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, CDE_NO, 0 );
										}
									}

							/**************************************************************************************/
							/**/break;		/*																	 **/
							/**************************************************************************************/


							/**************************************************************************************/
							/**/case CDE_NO:		/************ SE CAGO EL CDE :: BEGIN **************/
							/**************************************************************************************/
											/* Un EQUIPO nos avisa que el CDE cago fuego (no se pudo conectar) */

											/* Acciones a tomar cuando el CDE murio */
											CIUDAD_CDE_EstaMuerto( &iFdCDE, sNombreCiudad );

							/**************************************************************************************/
							/**/break;		/*																	 **/
							/**************************************************************************************/



							/**************************************************************************************/
							/**/case JE_JUGAR_CON:		/************ PIDE JUGAR CON EQUIPO :: BEGIN **************/
							/**************************************************************************************/
									/* Busca el estado del equipo y envia direccion TIPO_7 */

									/* Recibe desde EQUIPO el nombre de equipo contrincante a buscar ESTADO */
									if ( recv( lstFdsAux->info.FdEquipo, sEquipo, CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1)
										WriteLog(ERROR, sNombreCiudad, "-22- No se pudo recibir la informacion del equipo a buscar");

									/* Lista de Equipos Auxiliar para buscar Estado */
									lstFdsEstAux = lstFds;

									/* Escribe en el LOG */
									/*
									sprintf( sLog, "BUSCAMOS A \"%s\" ", sEquipo );
									WriteLog( INFO, sNombreCiudad, sLog );
									*/

									/* Busca ESTADO del equipo contrincante en lista de equipos LOCALES */
									while ( lstFdsEstAux != NULL && (strcmp(sEquipo,lstFdsEstAux->info.Equipo) != 0 || lstFdsEstAux->info.Estado == MUERTO ) ) {
										lstFdsEstAux = (FDS *)lstFdsEstAux->sgte;
									}

									if (lstFdsEstAux != NULL)
									{
										/* Escribe en el LOG */
										/*sprintf( sLog, "ENCONTRAMOS A \"%s\" ", lstFdsEstAux->info.Equipo );
										WriteLog( INFO, sNombreCiudad, sLog );*/

										if (lstFdsEstAux->info.Estado == INACTIVO)
										{
											/****************** JUEGA -> BEGIN *****************/

											/*
												Nos aseguramos que efectivamente esta inactivo..
													Esto puede arreglar algunas desincronizaciones
											*/
											/* Pide al CDE que expulse al equipo que esta inactivo */
											CIUDAD_CDE_ExpulsaEquipo ( &iFdCDE, lstFdsEstAux->info.Equipo, sNombreCiudad );
											sleep(1);

											/****** Envia cabecera a EQUIPO ******/
											if ( EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, JE_DIRECCION_EQUIPO, sizeof(strDireccionEquipo) ) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-23- No se pudo enviar cabecera a Equipo");

											strDireccionEquipo.IP = 0;
											strDireccionEquipo.Port = lstFdsEstAux->info.Port;

											if ( send( lstFdsAux->info.FdEquipo, &strDireccionEquipo, sizeof(strDireccionEquipo), 0) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-24- No se pudo enviar la informacion del equipo a buscar");

											WriteLog(INFO, sNombreCiudad, "Se encontro el equipo y estaba inactivo");

											/*
													IMPORTANTE: El estado del Equipo Contrincante se setea como JUGANDO aca, para evitar que
													cuando muchos equiposCIUDAD_ProcesaLevResultados (migrados) al mismo tiempo quieren jugar con este, el estado ya
													este actualizado y no se produzca que juegue mas de 1 partido al mismo tiempo
											*/
											if ( lstFdsAux->info.Migrado == TRUE )
												lstFdsEstAux->info.Estado = JUGANDO;

											/****************** JUEGA -> END *****************/
										}
										else if (lstFdsEstAux->info.Estado == JUGANDO || lstFdsEstAux->info.Estado == DESCANSANDO)
										{
											/************** ENCOLA -> BEGIN ****************/

											/*Lo encola*/
											/*envia un mensaje a equipo indicandole que debe ENCOLAR el contrincante*/

											/*printf("El equipo %s esta descansando\n", lstFdsEstAux->info.Equipo);*/

											EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, JE_NO, 3 );

											/* Se envia un mensaje a Equipo:
														Si es 1, el Equipo se encola
														Sino, esta muerto
											*/
											if ( send( lstFdsAux->info.FdEquipo, "1", sizeof(int), 0) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-26- No se pudo enviar la informacion del equipo a buscar");

											/*
											Comentado :: Imprime muchas lineas en el LOG
											WriteLog(INFO, sNombreCiudad, "Se encontró el equipo, estaba ocupado y fue encolado");
											*/


											/****************** ENCOLA -> END *****************/
										}
										else if (lstFdsEstAux->info.Estado == ENTRENANDO)
										{

											/****************** QUITA DEL CDE -> BEGIN *****************/

											/* Pide al CDE que expulse al equipo que esta inactivo */
											CIUDAD_CDE_ExpulsaEquipo ( &iFdCDE, lstFdsEstAux->info.Equipo, sNombreCiudad );
											sleep(1);

											/****** Envia cabecera a EQUIPO ******/
											if ( EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, JE_DIRECCION_EQUIPO, sizeof(strDireccionEquipo) ) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-27- No se pudo enviar cabecera a Equipo");
											else
											{
												strDireccionEquipo.IP = 0;
												strDireccionEquipo.Port = lstFdsEstAux->info.Port;

												if ( send( lstFdsAux->info.FdEquipo, &strDireccionEquipo, sizeof(strDireccionEquipo), 0) < 1 )
													WriteLog(ERROR, sNombreCiudad, "-28- No se pudo enviar la informacion del equipo a buscar");

												/* Escribe en el LOG */
												sprintf( sLog, "Equipo \"%s\" ENCUENTRA a \"%s\" (en CDE)", lstFdsAux->info.Equipo, sEquipo );
												WriteLog( INFO, sNombreCiudad, sLog );
											}

											CIUDAD_CDE_ExpulsaEquipo ( &iFdCDE, lstFdsEstAux->info.Equipo, sNombreCiudad );

											/****************** QUITA DEL CDE -> END *****************/

										}
									}
									else	/* envía TIPO_8: No existe el equipo */
									{
										/* Escribe en el LOG */
										sprintf( sLog, "NO ENCUENTRA a \"%s\"",  sEquipo );
										WriteLog( INFO, sNombreCiudad, sLog );

										/****** Envia cabecera a EQUIPO ******/
										if ( EnviaCabecera( lstFdsAux->info.FdEquipo, &iIdMensaje, JE_NO, 3 ) < 1 )
											WriteLog(ERROR, sNombreCiudad, "-29- No se pudo enviar cabecera a Equipo");
										else
										{
											/*cambio FH--envia un 0 indicando que el equipo esta muerto*/
											if ( send( lstFdsAux->info.FdEquipo, "0", sizeof(int), 3) < 1 )
												WriteLog(ERROR, sNombreCiudad, "-30- No se pudo enviar la informacion del equipo a buscar");
										}
									}

							/**************************************************************************************/
							/**/break;		/*					PIDE JUGAR CON EQUIPO :: END				 **/
							/**************************************************************************************/


							/**************************************************************************************/
							/**/case LEV_RESULTADOS:	/******************************************************/
							/**************************************************************************************/

										/* Elimina la lista de resultados vieja */
										if ( sListaResultados != NULL )
											free( sListaResultados );

										sListaResultados = malloc ( CABECERA_CiudadEquipo.LargoMensaje );

										if ( recv(lstFdsAux->info.FdEquipo, sListaResultados, CABECERA_CiudadEquipo.LargoMensaje, 0) < 1 )
										{
											WriteLog(ERROR, sNombreCiudad, "-31- No se pudo recibir la lista de resultados");
											break;
										}
										else
										{
											/* Carga la lista con los nuevos resultados */
											DesencriptaLevResultados( &lstLevResultados, sListaResultados );

											/* Escribe en el LOG */
											sprintf( sLog, "Recibe LEV Resultados de \"%s\"", lstFdsAux->info.Equipo );
											WriteLog( INFO, sNombreCiudad, sLog );
										} /* Fin de condicion de recibir mal la LEV de resultados */

										/* Agrega Equipos nuevos creados que aun no se ingresaron al TOKEN (si los hay) */
										/*AgregaEquiposToken( lstFds, &strToken, sNombreCiudad );*/

										/* ############# FIN DE LA LEV EN EL EQUIPO #############  */



										/* Se esta jugando la LEV INTERNA */
										if ( iGameOverLEVinterna != TRUE )
										{
											/* Procesa los resultados, sumando los puntos correspondientes a cada Equipo */
											CIUDAD_ProcesaLevResultados ( lstLevResultados, lstFds, lstFdsAux );

											/* Actualiza TDP del TOKEN */
											if ( ActualizaTdp(strToken, lstLevResultados, lstFdsAux->info.Equipo) == FAIL )
												WriteLog( ERROR, "ERROR 354: No se pudo actualizar la Tabla del Token", sLog );

											/* Envio de LEV al siguiente Equipo de la lista */
											lstFds = CIUDAD_EnviaLevEquipo (
																lstFds, &lstFdTieneLEV, &lstLevInterna,
																&iFdCDE, &iIdMensaje, sNombreCiudad, &iResult, ES_LEV_INTERNA, &iAlPedo
																											);

											if ( iResult == FAIL )
											{
												/* Ocurrio un error GRAVE (como la muerte del CDE) */
												/*
													ACA DEBERIA ENVIARLE UN MENSAJE A MAIN DICIENDO QUE NO SE PUEDE CONTINUAR, ASI MAIN CIERRA TODITO
												*/
												/*ctrl_c();*/
												/*return FAIL;*/
											}

											if ( lstFdTieneLEV == NULL ) /* Ya se termino de enviar la LEV a todos los equipos */
											{
												/* SE TERMINO DE JUGAR EL TORNEO INTERNO */

												lstFds = CIUDAD_FinTorneoInterno( lstFds, &iFdCDE, sNombreCiudad );

												iGameOverLEVinterna = TRUE;

												/* Arma LEV EXTERNA (no tiene los Equipos de nuestra Ciudad) */
												ArmaLevExterna( &lstLevExterna, (TKN_LISTATDP *) strToken.lstTdp, sNombreCiudad );

												printf("\n-- CIUDAD: Iniciando juego con LEV EXTERNA en 2 segundos\n");
												sleep( 2 );

												/* LevSent en FALSE, asi comienza envío de Lev Externa */
												/*RestablecerFds(&lstFds);*/

												if ( lstLevExterna != NULL ) /* Hay Equipos "extranjeros" para jugar la LEV EXTERNA */
												{

													iCantEquipos = 0;
													/* DISTRIBUYE LEV EXTERNA a TODOS los Equipos a la vez */
													do
													{
														lstFds = CIUDAD_EnviaLevEquipo (
																lstFds, &lstFdTieneLEV, &lstLevExterna,
														&iFdCDE, &iIdMensaje, sNombreCiudad, &iResult, ES_LEV_EXTERNA, &iCantEquipos
																				);
														/* Como para "desincronizar" un poco a los Equipos */
														sleep( 1 );
													}
													while (lstFdTieneLEV != NULL);


										/*
													Se vuelve a poner LevSent en FALSE, lo vamos a usar para ir chequeado
													que Equipos ya nos devolvieron los resultados de la LEV EXTERNA, y asi
													saber cuando se da por terminado el Torneo Externo en esta Ciudad
										*/
													/*RestablecerFds( &lstFds );*/
												}
												else /* NO hay Equipos para jugar en la LEV EXTERNA */
												{
													/* No se envio a LEV a ningun Equipo, reenvio del TOKEN al Router */

													printf("\n\nSE TERMINO DE JUGAR EL TORNEO EXTERNO (no hay Equipos)\n\n");
													WriteLog(INFO, sNombreCiudad, "Se termino de enviar la LEV Externa, se reenvia el TOKEN");

													/* Convierte TOKEN a cadena */
													iLargoToken = AplanaToken ( strToken, &sToken );

													EnviaCabeceraRouter(iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD);
													if ( send(iFdRouter, sToken, iLargoToken, 0) < 1 )
														WriteLog(ERROR, sNombreCiudad, "-36- No se pudo enviar el Token");

													iGameOverLEVexterna = TRUE;

													Setea_FDS_FinPasadaToken( &lstFds );
												}

											}
										}

										/* Se esta jugando la LEV EXTERNA */
										else if ( iGameOverLEVexterna != TRUE )
										{

											/* Marcamos a este Equipo como que nos ha enviado sus resultados */
											lstFdsAux->info.LevSent = TRUE;

											/* Actualiza TDP del TOKEN */
											if ( ActualizaTdp(strToken, lstLevResultados, lstFdsAux->info.Equipo) == FAIL )
												WriteLog( ERROR, "ERROR 354: No se pudo actualizar la Tabla del Token", sLog );

											/* Chequea si se elimino un equipo. De ser asi, lo elimina del TOKEN */
											strToken.lstTdp = (struct TKN_LISTATDP *) CIUDAD_ChequeaEliminacionEquiposFD ( (TKN_LISTATDP *)strToken.lstTdp, lstFds, sNombreCiudad );

											/* Convierte TOKEN a cadena */
											/*iLargoToken = AplanaToken ( strToken, &sToken );*/

											/* Se chequea si ya hemos recibido TODOS los LEVresultados de cada Equipo */
											lstFdsEstAux = lstFds;

											iCantEquiposTmp = 0;

											/*while ( lstFdsEstAux != NULL && lstFdsEstAux->info.LevSent == TRUE )*/

											iHayEquiposNoMigrados = FALSE;

											lstFdsAux->info.LevSentExt = ENVIO_RESULTADOS;

											while ( lstFdsEstAux != NULL )
											{
												if (
																	lstFdsEstAux->info.Migrado == FALSE
																&&
																	lstFdsEstAux->info.TokenSet == TRUE
																&&
																	lstFdsEstAux->info.LevSentExt == ENVIO_RESULTADOS
																&&
																	lstFdsEstAux->info.Estado != MIGRADO
													 )
												{
														iCantEquiposTmp++;
												}
												/*if ( lstFdsEstAux->info.LevSentExt == NO_MIGRADO )
												iHayEquiposNoMigrados=TRUE;*/

												lstFdsEstAux = (FDS *) lstFdsEstAux->sgte;
											}


											if ( iCantEquipos <= iCantEquiposTmp )
											{

												/* Ya se termino de recibir LEVresultados de todos los Equipos */

												printf("\n\nSE TERMINO DE JUGAR EL TORNEO EXTERNO!!\n\n");

												/* Convierte TOKEN a cadena */
												iLargoToken = AplanaToken ( strToken, &sToken );

												/* Devolucion de TOKEN a Router */
												EnviaCabeceraRouter(iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD);
												if ( send(iFdRouter, sToken, iLargoToken, 0) < 1 )
													WriteLog(ERROR, sNombreCiudad, "-36- No se pudo enviar el Token");

												iGameOverLEVexterna = TRUE;

												iCantEquipos = 0;
												iCantEquiposTmp = -1;

												Setea_FDS_FinPasadaToken( &lstFds );

											}

										}
										else
										{
											/*
												ERROR: No pudimos haber recibido la LEVresultados
																si ya termino tanto el Torneo INTERNO como EXTERNO..
											*/
											printf("\n\nWHAT A FUCKKK!! (LEVresultados)\n\n");
										}
										/*intf("\nAntes de LiberaLevResultados\n");
										LiberaLevResultados( &lstLevResultados );
										printf("\nDespues de LiberaLevResultados\n");*/

							/**************************************************************************************/
							/**/break;		/*																 **/
							/**************************************************************************************/


							/**************************************************************************************/
							/**/case CME_EQUIPO:	/**********************************************************/
							/**************************************************************************************/

										/* Para entrega 5: MIGRACION */
											/* Para que chorga se usa esto??? */

							/**************************************************************************************/
							/**/break;		/*																 **/
							/**************************************************************************************/


							/**************************************************************************************/
							/**/case AE_ESTADO:		/********** ACTUALIZACION DE ESTADO DE EQUIPO :: BEGIN ********/
							/**************************************************************************************/

										if ( recv( lstFdsAux->info.FdEquipo, &(iEstadoRecibido), CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1 )
											WriteLog(ERROR, sNombreCiudad, "-34- No se pudo actualizar el estado");

										/*si el estado recibido es jugando, y el equipo estaba en el cde, lo expulso*/
										if ( ( lstFdsAux->info.Estado == ENTRENANDO || lstFdsAux->info.Estado == DESCANSANDO ) && iEstadoRecibido == JUGANDO )
											CIUDAD_CDE_ExpulsaEquipo ( &iFdCDE, lstFdsAux->info.Equipo, sNombreCiudad );

										if ( iEstadoRecibido == MIGRADO )
										{
											/* Seteo del TimeOut */
											lstFdsAux->info.Timeout = GetTimestamp();
										}

										/*actualizo el estado*/
										lstFdsAux->info.Estado = iEstadoRecibido;

										/* Escribe en el LOG */
										sprintf( sLog, "Equipo \"%s\" actualiza su estado a %d", lstFdsAux->info.Equipo, lstFdsAux->info.Estado );
										WriteLog( INFO, sNombreCiudad, sLog );

							/**************************************************************************************/
							/**/break;		/*************** ACTUALIZACION DE ESTADO DE EQUIPO :: END **************/
							/**************************************************************************************/

								} /* END :: switch (CABECERA_CiudadEquipo.TipoMensaje) */


							} /* END :: if ( RecibeCabecera(lstFdsAux->info.FdEquipo,&CABECERA_CiudadEquipo) > 0 ) */

							/* #################################### MURIO UN EQUIPO :: BEGIN #################################### */

							else
							{
								if ( lstFdsAux->info.Estado == MIGRADO ) /* El Equipo cerro su conexion por que ha migrado */
								{
									printf("\n-- CIUDAD: El Equipo \"%s\" ha migrado!\n\n", lstFdsAux->info.Equipo );

									/* Pide al CDE que expulse al equipo que ha muerto */
									CIUDAD_CDE_ExpulsaEquipo ( &iFdCDE, lstFdsAux->info.Equipo, sNombreCiudad );
									sleep(1);

									if ( lstFdsAux->info.Migrado == TRUE ) /* El Equipo no pertenece a esta Ciudad */
									{
										/* Elimina el Equipo de la lista FDSs */
										lstFds = SuprimeNodoFDS( lstFds, lstFdsAux->info.Equipo);

										lstFdsAux = lstFds;
									}
									else
									{
										lstFdsAux->info.FdEquipo = -1;
									}

								}
								else /* El Equipo efectivamente MURIO! */
								{

									/* Decrementa el contador de Equipos para esta Ciudad (si pertenece a la misma) */
									if ( lstFdsAux->info.Migrado == FALSE && lstFdsAux->info.TokenSet == TRUE)
										iCantEquipos--;

									/* Se debe chequear si el Equipo que murio tenia la LEV o no */

									if ( lstFdTieneLEV != NULL && lstFdsAux->info.FdEquipo == lstFdTieneLEV->info.FdEquipo )
									{
										/* Murio el Equipo que tiene la LEV */

										printf("\n\n++++++++++ MURIO EL EQUIPO \"%s\" (tenia la LEV) ++++++++++ \n\n", lstFdsAux->info.Equipo );

										/* Escribe en el LOG */
										sprintf( sLog, "Elimina equipo muerto %s (tenia la LEV)\n", lstFdsAux->info.Equipo );
										WriteLog( INFO, sNombreCiudad, sLog );

										/* Llama a la funcion para que procese la muerte del equipo */
										CIUDAD_MuereEquipo ( &lstFds, &lstLevInterna, &sLev, lstFdsAux, &iFdCDE, sNombreCiudad );

										/* ACA SE DEBERIA ELIMINAR LA INFO DEL EQUIPO EN EL TOKEN */

										/* ENVIO DE LEV AL SIGUIENTE EQUIPO DE LA LISTA */
										lstFds = CIUDAD_EnviaLevEquipo (
																		lstFds, &lstFdTieneLEV, &lstLevInterna,
																		&iFdCDE, &iIdMensaje, sNombreCiudad, &iResult, ES_LEV_INTERNA, &iAlPedo
																		);

										if ( iResult == FAIL )
										{
											/* Ocurrio un error GRAVE (como la muerte del CDE) */
												/*
											ACA DEBERIA ENVIARLE UN MENSAJE A MAIN DICIENDO QUE NO SE PUEDE CONTINUAR, ASI MAIN CIERRA TODITO
												*/
											ctrl_c();
											/*return FAIL;*/
										}

										if ( lstFdTieneLEV == NULL ) /* Ya se termino de enviar la LEV a todos los equipos */
										{
											/* SE TERMINO DE JUGAR EL TORNEO INTERNO */

											lstFds = CIUDAD_FinTorneoInterno( lstFds, &iFdCDE, sNombreCiudad );

											iGameOverLEVinterna = TRUE;

											/* Arma LEV EXTERNA (no tiene los Equipos de nuestra Ciudad) */
											ArmaLevExterna( &lstLevExterna, (TKN_LISTATDP *) strToken.lstTdp, sNombreCiudad );

											printf("\n-- CIUDAD: Iniciando juego con LEV EXTERNA en 5 segundos\n");
											sleep( 5 );

											/* LevSent en FALSE, asi comienza envío de Lev Externa */
											/*RestablecerFds(&lstFds);*/

											if ( lstLevExterna != NULL ) /* Hay Equipos "extranjeros" para jugar la LEV EXTERNA */
											{
												/* DISTRIBUYE LEV EXTERNA a TODOS los Equipos a la vez */
												do
												{
													lstFds = CIUDAD_EnviaLevEquipo (
															lstFds, &lstFdTieneLEV, &lstLevExterna,
															&iFdCDE, &iIdMensaje, sNombreCiudad, &iResult, ES_LEV_EXTERNA, &iCantEquipos
																				);
													/* Como para "desincronizar" un poco a los Equipos */
													sleep( 1 );
												}
												while (lstFdTieneLEV != NULL);

										/*
												Se vuelve a poner LevSent en FALSE, lo vamos a usar para ir chequeado
												que Equipos ya nos devolvieron los resultados de la LEV EXTERNA, y asi
												saber cuando se da por terminado el Torneo Externo en esta Ciudad
										*/
												/*RestablecerFds( &lstFds );*/
											}
											else /* NO hay Equipos para jugar en la LEV EXTERNA */
											{
												/* No se envio a LEV a ningun Equipo, reenvio del TOKEN al Router */

												printf("\n\nSE TERMINO DE JUGAR EL TORNEO EXTERNO (no hay Equipos)\n\n");
												WriteLog(INFO, sNombreCiudad, "Se termino de enviar la LEV Externa, se reenvia el TOKEN");

												/* Convierte TOKEN a cadena */
												iLargoToken = AplanaToken ( strToken, &sToken );

												EnviaCabeceraRouter(iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD);
												if ( send(iFdRouter, sToken, iLargoToken, 0) < 1 )
													WriteLog(ERROR, sNombreCiudad, "-36- No se pudo enviar el Token");

												iGameOverLEVexterna = TRUE;

												Setea_FDS_FinPasadaToken( &lstFds );

											}


										}


									}
									else
									{
										/* Murio un Equipo cualquiera (NO tenia la LEV) */

										printf("\n\n++++++++++ MURIO EL EQUIPO \"%s\" ++++++++++ \n\n", lstFdsAux->info.Equipo );

										/* Escribe en el LOG */
										sprintf( sLog, "Elimina equipo muerto %s\n", lstFdsAux->info.Equipo );
										WriteLog( INFO, sNombreCiudad, sLog );

										/* Llama a la funcion para que procese la muerte del equipo */
										CIUDAD_MuereEquipo ( &lstFds, &lstLevInterna, &sLev, lstFdsAux, &iFdCDE, sNombreCiudad );

										/* ACA SE DEBERIA ELIMINAR LA INFO DEL EQUIPO EN EL TOKEN */

										lstFdsAux = lstFds;
									}

								}

							}

							/* #################################### MURIO UN EQUIPO :: END #################################### */

						} /* END :: if ( FD_ISSET(lstFdsAux->info.FdEquipo,&BolsaDescriptores) ) */

						/* Desplaza el puntero hacia el siguiente equipo para chequear si nos hablo */
						if ( lstFdsAux != NULL )
						{
							lstFdsAux = (FDS *) lstFdsAux->sgte;
						}

					} /* END :: while ( lstFdsAux != NULL ) */


			/**************************************************************************************/
			/* ############ HABLO EQUIPO DE LA LISTA :: END ############## */
			/**************************************************************************************/

					/* ############ SE CONECTO UN EQUIPO QUE QUIERE MIGRAR HACIA ESTA CIUDAD :: BEGIN ############## */

					if ( iFdEscucha != -1 && FD_ISSET(iFdEscucha,&BolsaDescriptores) )
					{
						/* Un Equipo nos hablo por el FD escucha */
						iFdEquipoExtranjero = AceptaConexion(iFdEscucha,NULL,NULL);

						if ( iFdEquipoExtranjero != -1 )
						{
							/*
								Nos identificamos enviando el Nombre de la Ciudad,
								para que el Equipo chequee si somos su Ciudad de Origen
							*/
							if ( send(iFdEquipoExtranjero, sNombreCiudad, strlen(sNombreCiudad)+1, 0) < 1 )
								WriteLog(ERROR, sNombreCiudad, "-189- No se pudo enviar mensaje DISCOVER timeout a Equipo");

							/* Elimina Lev Resultados vieja*/
							if ( sLevResultados != NULL )
								free(sLevResultados);

							if ( recv( iFdEquipoExtranjero, &strEquipo, sizeof(strEquipo), 0 ) < 1 )
								WriteLog(ERROR, sNombreCiudad, "-74654- No se pudo recibir los datos del equipo a crear");

							RecibeCabecera(iFdEquipoExtranjero,&CABECERA_CiudadEquipo);
							sLevResultados = malloc( CABECERA_CiudadEquipo.LargoMensaje );

							if ( recv( iFdEquipoExtranjero, sLevResultados, CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1 )
								WriteLog(ERROR, sNombreCiudad, "-72234- No se pudo recibir la Lev Resultados del equipo a crear");

							RecibeCabecera(iFdEquipoExtranjero,&CABECERA_CiudadEquipo);
							sNombreCiudadOrigen = malloc( CABECERA_CiudadEquipo.LargoMensaje );
							if ( recv( iFdEquipoExtranjero, sNombreCiudadOrigen, CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1 )
								WriteLog(ERROR, sNombreCiudad, "-72234- No se pudo recibir el nombre de Ciudad Origen del equipo a crear");

							RecibeCabecera(iFdEquipoExtranjero,&CABECERA_CiudadEquipo);
							sIPCiudadOrigen = malloc( CABECERA_CiudadEquipo.LargoMensaje );
							if ( recv( iFdEquipoExtranjero, sIPCiudadOrigen, CABECERA_CiudadEquipo.LargoMensaje, 0 ) < 1 )
								WriteLog(ERROR, sNombreCiudad, "-72234- No se pudo recibir la IP de Ciudad Origen del equipo a crear");

							RecibeCabecera(iFdEquipoExtranjero,&CABECERA_CiudadEquipo);
							sPortCiudadOrigen = malloc( CABECERA_CiudadEquipo.LargoMensaje );
							if ( recv( iFdEquipoExtranjero, sPortCiudadOrigen, CABECERA_CiudadEquipo.LargoMensaje,  0 ) < 1 )
								WriteLog(ERROR, sNombreCiudad, "-72234- No se pudo recibir el PORT de Ciudad Origen del equipo a crear");


							close( iFdEquipoExtranjero );
							iFdEquipoExtranjero = -1;

							/* Se recibieron todos los datos del Equipo */

							lstFdsInfo.FdEquipo = 1; /* FLAG FLAG FLAG FLAG FLAG FLAG que se usara para chequear si el Equipo debe crearse o no */

							/* Se chequea si este Equipo NO pertenece a esta Ciudad */
							if ( strcmp( sNombreCiudadOrigen, sNombreCiudad ) != 0 )
							{
								lstFdsInfo.Migrado = TRUE;

								printf("\n-- CIUDAD: El Equipo \"%s\" ha migrado a esta ciudad!\n", strEquipo.Equipo);

								/* Escribe en el LOG */
								sprintf( sLog, "Se crea nuevo equipo migrado \"%s\"",strEquipo.Equipo );
								WriteLog( INFO, sNombreCiudad, sLog );

							}
							else
							{
								lstFdsInfo.Migrado = FALSE;

								/* Se elimina el viejo nodo de la lista de Equipos */
								lstFdsBeAux = lstFds;


								while ( lstFdsBeAux != NULL && strcmp(strEquipo.Equipo,lstFdsBeAux->info.Equipo) != 0 )
								{
									lstFdsBeAux = (FDS *) lstFdsBeAux->sgte;
								}

								if ( lstFdsBeAux != NULL ) /* El Equipo aun NO EXPIRO */
								{
									lstFds = SuprimeNodoFDS( lstFds, lstFdsBeAux->info.Equipo);

									printf("\n-- CIUDAD: El Equipo \"%s\" ha retornado a su ciudad origen!\n\n", strEquipo.Equipo);

									/* Escribe en el LOG */
									sprintf( sLog, "Equipo %s que migro vuelve a su ciudad \"%s\"", strEquipo.Equipo, sNombreCiudad );
									WriteLog( INFO, sNombreCiudad, sLog );


								}
								else
								{
									printf("\n-- CIUDAD: El Equipo \"%s\" ha retornado, pero expiro su tiempo de espera\n\n", strEquipo.Equipo);

									/* Escribe en el LOG */
									sprintf( sLog, "El Equipo \"%s\" ha retornado, pero expiro su tiempo de espera", strEquipo.Equipo );
									WriteLog( INFO, sNombreCiudad, sLog );

									/* No debemos crear el Equipo */
									lstFdsInfo.FdEquipo = -1;

								}

							}

							if ( lstFdsInfo.FdEquipo != -1 ) /* Todo OK con el Equipo => Debe crearse */
							{
								/* Obtiene los FD para el nuevo EQUIPO */
								if ( GetFD_socketpair(&iFdEquipo,sFdEquipo) != OK ) {
									printf("-- CIUDAD: ERROR - No se pudo crear el FD para EQUIPO\n");
									WriteLog(ERROR, sNombreCiudad, "-8- No se pudo crear el FD para EQUIPO");
								}

								while ((iFdEscuchaAux = AbreSocket(++iPort)) < 1);

								sprintf(sKI, "%s" , strEquipo.KI);
								sprintf(sCansancio, "%s" , strEquipo.Cansancio);

								/* Creo nuevo proceso EQUIPO */
								pidEquipo = fork();

								if ( ! pidEquipo ) /* Estoy en el hijo (equipo) */
								{
									/* Parametros para EQUIPO:
											1) FD de EQUIPO
											2) NOMBRE de EQUIPO
											3) Ki
											4) Cansancio
											5) Duracion de partidos
											6) Nombre de esta Ciudad
											7) Nombre de la Ciudad Origen
									*/
									dup2(iFdEscuchaAux, 2);
									/*cambio FH*/
									close ( iFdEquipo );


									execl( "equipo", strEquipo.Equipo, sFdEquipo, sKI, sCansancio, sDuracionPartidos, sNombreCiudad, sNombreCiudadOrigen, sIPCiudadOrigen, sPortCiudadOrigen, (char*)NULL );
									exit(0);
								}

								/* Estoy en el padre (ciudad) */

								close ( atoi(sFdEquipo) );

								lstFdsBeAux = lstFds;


								/* Guarda los datos en el INFO del NODO de lista FDS */
								sprintf( lstFdsInfo.Equipo, "%s", strEquipo.Equipo );
								lstFdsInfo.FdEquipo = iFdEquipo;
								lstFdsInfo.Estado = INACTIVO;
								lstFdsInfo.Port = iPort;
								lstFdsInfo.LevSent = TRUE;
								lstFdsInfo.LevSentExt = TRUE;
								lstFdsInfo.TokenSet = TRUE;
								lstFdsInfo.Puntaje = 0;


								/* Actualiza la lista FDS de Equipos de la CIUDAD */
								lstFds = InsertaNodoFDS( lstFds, lstFdsInfo );

								/* Le enviamos la LEV Resultados para que continue con la ejecucion */
								EnviaCabecera( iFdEquipo, &iIdMensaje, LEV_RESULTADOS, strlen(sLevResultados)+1 );
								send( iFdEquipo, sLevResultados, strlen(sLevResultados)+1, 0 );

							}

						}

					}

					/* ############ SE CONECTO UN EQUIPO QUE QUIERE MIGRAR HACIA ESTA CIUDAD :: END ############## */

				} /* END del if( valret ) */


				/* Obtiene el EPOCH actual */
				tTimestamp = GetTimestamp();


				/* ############ CHEQUEO DE TIMEOUT DE EQUIPOS MIGRADOS :: BEGIN ############## */

				/* FLAG FLAG FLAG FLAG FLAG FLAG FLAG que se usara para chequear si se produjeron timeouts de Equipos */
				iCantEquiposTmp = -1;

				lstFdsAux = lstFds;
				/* Recorre la lista en busqueda de Equipos MIGRADOS que hayan expirado su espera */
				while ( lstFdsAux != NULL )
				{

					if (
								lstFdsAux->info.Migrado == FALSE /* Pertenece a ESTA Ciudad */
								&&
								lstFdsAux->info.Estado == MIGRADO /* Esta actualmente MIGRADO */
								&&
								(tTimestamp - lstFdsAux->info.Timeout) >= Cfg.MigracionTimeout /* Expiro su tiempo de espera */
						 )
					{
						/* Se encontro un Equipo que expiro */

						iCantEquiposTmp = 0;

						printf("\n\n++++++++++ TIMEOUT ALCANZADO PARA EL EQUIPO MIGRADO: \"%s\" ++++++++++\n\n", lstFdsAux->info.Equipo );
						WriteLog(INFO, sNombreCiudad, "-8888- ++++++++++ TIMEOUT ALCANZADO PARA EL EQUIPO MIGRADO ++++++++++");

						/* Se busca al equipo en la TDP y se lo borra */
						lstTokenTdp = (TKN_LISTATDP *) strToken.lstTdp;

						while ( lstTokenTdp != NULL )
						{
							if (
											strcmp(lstTokenTdp->info.Ciudad, sNombreCiudad) == 0
										&&
											strcmp(lstTokenTdp->info.Equipo,lstFdsAux->info.Equipo) == 0
								 )
								break;

							lstTokenTdp = (TKN_LISTATDP *) lstTokenTdp->sgte;
						}

						if ( lstTokenTdp != NULL )
						{
							strToken.lstTdp = (struct TKN_LISTATDP *) SuprimeNodoTokenTDP( (TKN_LISTATDP *) strToken.lstTdp, lstTokenTdp->info );
						}

						/* Elimina el Equipo de la lista FDSs */
						lstFds = SuprimeNodoFDS( lstFds, lstFdsAux->info.Equipo);


						iCantEquipos--;

						lstFdsAux = lstFds;
					}
					else
						lstFdsAux = (FDS *) lstFdsAux->sgte;
				}

				/* Se chequea si aun hay Equipos por esperar */

				lstFdsAux = lstFds;


				if ( iCantEquiposTmp == 0 ) /* Si se produjeron timeouts y se eliminaron Equipos.. */
				{


					strToken.lstTdp = (struct TKN_LISTATDP *) CIUDAD_ChequeaEliminacionEquiposFD ( (TKN_LISTATDP *)strToken.lstTdp, lstFds, sNombreCiudad );

					while ( lstFdsAux != NULL )
					{
						if (
										lstFdsAux->info.Migrado == FALSE /* Pertenece a ESTA Ciudad */
									&&
										lstFdsAux->info.LevSentExt == ENVIO_RESULTADOS /* Ya recibimos los Resultados de este Equipo */
									&&
										lstFdsAux->info.Estado != MIGRADO /* El Equipo NO esta migrado */
									&&
										lstFdsAux->info.TokenSet == TRUE
							)
						{
							iCantEquiposTmp++;
						}

						lstFdsAux = (FDS *) lstFdsAux->sgte;
					}
				}

				/*printf("\niCantEquipos: %d; --- iCantEquiposTmp: %d", iCantEquipos, iCantEquiposTmp);*/

				/*if ( iGameOverLEVinterna == TRUE && iGameOverLEVexterna == FALSE && (iCantEquipos < 1 || iCantEquipos == iCantEquiposTmp) )*/
				if ( iGameOverLEVinterna == TRUE && iGameOverLEVexterna == FALSE && iCantEquipos <= iCantEquiposTmp )
				{
					/* No hay mas Equipos por esperar => Se devuelve el TOKEN al Router */

					printf("\n\nSE TERMINO DE JUGAR EL TORNEO EXTERNO!! (timeout)\n\n");

					/* Convierte TOKEN a cadena */
					iLargoToken = AplanaToken ( strToken, &sToken );

					/* Devolucion de TOKEN a Router */
					EnviaCabeceraRouter(iFdRouter, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD);
					if ( send(iFdRouter, sToken, iLargoToken, 0) < 1 )
						WriteLog(ERROR, sNombreCiudad, "-36- No se pudo enviar el Token");

					iGameOverLEVexterna = TRUE;

					iCantEquipos = 0;
					iCantEquiposTmp = -1;

					Setea_FDS_FinPasadaToken( &lstFds );

				}

				/* ############ CHEQUEO DE TIMEOUT DE EQUIPOS MIGRADOS :: END ############## */


				/* ############ CHEQUEO DE TIMEOUT DE MENSAJES EN ESPERA :: BEGIN ############## */

				lstDISCOVERsentAux = lstDISCOVERsent;
				/* Recorre la lista en busqueda de mensajes DISCOVER que hayan expirado */
				while ( lstDISCOVERsentAux != NULL )
				{

					if ( (tTimestamp - lstDISCOVERsentAux->info.Timeout) >= Cfg.DiscoverTimeout )
					{
						/* Se encontro un mensaje que expiro */

						printf("-- CIUDAD: DISCOVER expirado\n");

						WriteLog(ERROR, sNombreCiudad, "-18989- DISCOVER expirado");

						EnviaCabecera( lstDISCOVERsentAux->info.FdOrigen, &iIdMensaje, BE_NO, 0 );
						/* Envia un mensaje al Equipo Origen del mensaje avisando que no se encontro */
						if ( send( lstDISCOVERsentAux->info.FdOrigen, "0", 3, 0) < 1 )
							WriteLog(ERROR, sNombreCiudad, "-189- No se pudo enviar mensaje DISCOVER timeout a Equipo");

						/* Se elimina el mensaje de la lista */
						lstDISCOVERsent = SuprimeNodoDiscoverSent( lstDISCOVERsent, lstDISCOVERsentAux );

						lstDISCOVERsentAux = lstDISCOVERsent;
					}
					else
						lstDISCOVERsentAux = (DISCOVERSENT *) lstDISCOVERsentAux->sgte;
				}

				/* ############ CHEQUEO DE TIMEOUT DE MENSAJES EN ESPERA :: END ############## */

			}

		}

	}

	return 0;
}
