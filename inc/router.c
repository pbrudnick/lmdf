/*
	Ultima modificacion: 22/06 - 05:30
	Modificado por: Tony Meola (??)
*/

#include "./headers/global_includes.h"
#include "./headers/defines.h"
#include "./inc/config.c"
#include "./inc/log.c"
#include "./socketservidor.c"
#include "./socketcliente.c"
#include "./inc/varias.c"

int main ( int iArgs,char *aArgs[] )
{
	/* FD de MAIN */
	int iFdMain = FD_MAIN;

	/* Incremental para IdMensaje de Cabecera */
	int iIdMensaje = 0;

	/* Declara estructura TOKEN */
	/*TOKEN strToken;*/

	/* ##### Cabeceras y Mensajes de comunicación ##### */
	CABECERA_ROUTERROUTER CABECERA_RouterRouter;	/* Cabecera ROUTER-ROUTER / ROUTER-CIUDAD */
	CABECERA_CIUDADEQUIPO CABECERA_MainRouter;	/* Cabecera MAIN-ROUTER / ROUTER-MAIN */

	/* Cabecera Router_Router para mandar => 7 bytes */
	char *sCabeceraRouter = NULL;

	/* FD Escucha para el ROUTER */
	int iFdEscucha = -1;

	/* Puerto de ROUTER */
	int iPort;

	/* FD de Ciudad */
	int iFdCiudad;

	/* FD de otro Router */
	int iFdRouter = -1;

	/* Bolsa de descriptores */
	fd_set BolsaDescriptores;

	/* El valor del FD mas alto */
	int iMaximo;

	/* El valor que devuelve la funcion select */
	int iValRet;

	/* Nombre de la CIUDAD conectada a este ROUTER */
	char sNombreCiudad[30];

	/* Estructura del token */
	TOKEN strToken;

	/* Punteros a TOKEN */
	/*
	TKN_LISTACIUDADES *lstTokenCiudades = NULL;
	TKN_LISTATDP *lstTokenTdp = NULL;
	*/

	/*
		::: Cadena del Token Aplanado :::
		Sirve tambien como FLAG para saber si el TOKEN esta actualmente en este ROUTER o no

		sToken == NULL => El TOKEN NO ESTA en este Router
		sToken != NULL => El TOKEN ESTA en este Router
	*/
	char *sToken = NULL;
	char *sTokenTmp = NULL;

	/* Largo del Token */
	long int iLargoToken;

	/* El fd de origen del Token (Por donde me enviaron el token por primera vez) */
	int iFdTokenOrigen = -1;

	/* No se recibio el Token */
	int iTokenRecibido = FALSE;

	/* Jugo ya nuestra ciudad su LEV interna y externa?? */
	int iJugoLEVCiudad = FALSE;

	/* Estado del Juego */
	int iGameOver = FALSE;
	int iGameOverAux = FALSE;

	/* Para chequear si se envio correctamente el TOKEN a aquellos Routers que todavia no figuren alli */
	int iEnvioCorrecto = FALSE;

	int iEsNew = FALSE;

	/* Archivo de Configuracion */
	CFG Cfg;

	/* Lista de los FDs del Router */
	FDROUTER *lstFdRouter = NULL, *lstFdRouterAux = NULL, *lstFdRouterAuxTmp;
	FDROUTERinfo lstFdRouterInfo;

	DISCOVER strDiscover;
	FOUND strFound;

	/* Lista de IdMensajes propagados */
	DISCOVERSENT *lstDISCOVERsent = NULL, *lstDISCOVERsentAux;

	/*para ignorar el SIGPIPE (si el send da -1, linux hace giladas)*/
	signal(SIGPIPE, SIG_IGN);


	/* ########################## INICIALIZACION BEGIN ########################## */

	/* Obtiene el PUERTO de ROUTER pasado por parametro (Main lo recibe del CFG) */
	sscanf( aArgs[1], "%d" , &iPort );

	sscanf( aArgs[2], "%d" , &iFdCiudad );


	/* Llamado a la función que carga el archivo de configuración */
	if ( CargaConfig(&Cfg) == FAIL )
	{
		printf( "\n\nERROR FATAL: No se puede cargar el archivo de configuracion\n\n");
		WriteLog( ERROR, "Router", "Error:1: No se pudo cargar el archivo de configuracion" );
		return FAIL;
	}

	/* Obtiene un FD de escucha */
	if ( (iFdEscucha = AbreSocket(iPort)) < 1 )
	{
		printf( "- ROUTER: No se puede abrir el puerto de escucha %d\n", iPort );
		WriteLog( ERROR, "Router", "Error:1: No se puede abrir el puerto de escucha" );
		return FAIL;
	}

	/* Reserva lugar para Cabecera de 7 bytes */
	sCabeceraRouter = malloc(LARGO_CABECERA_ROUTER);

	WriteLog( INFO, "Router", "Router iniciado" );

	/* ########################## INICIALIZACION END ########################## */

	/* Entra en ciclo principal */
	while ( 1 )
	{

		FD_ZERO( &BolsaDescriptores );

		FD_SET( iFdEscucha, &BolsaDescriptores );
		iMaximo = iFdEscucha;

		if ( iFdCiudad > -1 )
		{
			FD_SET( iFdCiudad, &BolsaDescriptores );
			if ( iMaximo < iFdCiudad )
				iMaximo = iFdCiudad;
		}

		if ( iFdMain != -1 )
		{
			FD_SET( iFdMain, &BolsaDescriptores );
			if ( iMaximo < iFdMain )
				iMaximo = iFdMain;
		}

		/* Carga FDs de ROUTERS */
		lstFdRouterAux = lstFdRouter;

		while ( lstFdRouterAux != NULL )
		{
			if ( lstFdRouterAux->info.FdRouter > iMaximo )
				iMaximo = lstFdRouterAux->info.FdRouter;

			FD_SET ( lstFdRouterAux->info.FdRouter, &BolsaDescriptores );

			lstFdRouterAux = (FDROUTER *) lstFdRouterAux->sgte;
		}


		/* SELECT */
		iValRet = select(iMaximo + 1,&BolsaDescriptores,NULL,NULL,NULL);

		/* Mensaje recibido desde algun FD de la bolsa */
		if ( iValRet > -1 )
		{

			/**************************************************************************************/
			/* #################### HABLO CIUDAD :: BEGIN #################### */
			/**************************************************************************************/

			if ( iFdCiudad > -1 && FD_ISSET(iFdCiudad,&BolsaDescriptores) )
			{
				if ( RecibeCabeceraRouter(iFdCiudad, &sCabeceraRouter, &CABECERA_RouterRouter) > 0 )
				{
					/* Acciones que debe realizar segun TipoMensaje */
					switch ( CABECERA_RouterRouter.TipoMensaje )
					{

					/**************************************************************************************/
					/**/case BE_DISCOVER:	/*************** MENSAJE DISCOVER :: BEGIN ******************/
					/**************************************************************************************/

							if ( recv( iFdCiudad, &strDiscover, CABECERA_RouterRouter.LargoMensaje, 0 ) == -1 )
							{
								WriteLog( ERROR, "-Router", "-1227- Error al recibir Discover de Ciudad" );

								ROUTER_MuereCiudad	(
																				&iFdCiudad,
																				sNombreCiudad,
																				&iFdTokenOrigen,
																				&lstFdRouter,
																				&iIdMensaje,
																				&strToken,
																				&sToken,
																				iLargoToken,
																				CABECERA_RouterRouter,
																				&sCabeceraRouter,
																				iGameOver
																		);

								break;
							}

							/* Que no propague si TTL == 1 */
							if ( CABECERA_RouterRouter.TTL > 1 )
							{
								/*printf("- ROUTER: Propagando DISCOVER.. (recibido de Ciudad)\n");*/

								/* Recorre la lista de los Routers conectados y propaga */
								lstFdRouterAux = lstFdRouter;
								lstFdRouterAuxTmp = lstFdRouter;

								while (lstFdRouterAux != NULL)
								{

									EnviaCabeceraRouter( lstFdRouterAux->info.FdRouter, &(CABECERA_RouterRouter.IdMensaje), BE_DISCOVER, sizeof(DISCOVER), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER );

									if ( send(lstFdRouterAux->info.FdRouter, &strDiscover, sizeof(DISCOVER), 0) < 1 )
									{
										WriteLog(ERROR, "Router", "-456- No se pudo propagar DISCOVER a Router");

										lstFdRouter = SuprimeNodoFDROUTER( lstFdRouter, lstFdRouterAux->info );

										if ( lstFdRouter != NULL )
											lstFdRouterAux = (FDROUTER *) lstFdRouterAuxTmp->sgte;
										else
											lstFdRouterAux = NULL;
									}
									else
									{
										lstFdRouterAuxTmp = lstFdRouterAux;
										lstFdRouterAux = (FDROUTER *) lstFdRouterAux->sgte;
									}

								}

							}


					/**************************************************************************************/
					/**/break;		/*					MENSAJE DISCOVER :: END					 **/
					/**************************************************************************************/


					/**************************************************************************************/
					/**/case BE_FOUND:	/*********** MENSAJE FOUND :: BEGIN ***************/
					/**************************************************************************************/

								/* Encontro Equipo en la Ciudad */

								if ( recv( iFdCiudad, &strFound, CABECERA_RouterRouter.LargoMensaje, 0 ) == -1 )
								{
									WriteLog(ERROR, "Router", "-3467- No se pudo recibir mensaje FOUND de Ciudad");

									ROUTER_MuereCiudad	(
																					&iFdCiudad,
																					sNombreCiudad,
																					&iFdTokenOrigen,
																					&lstFdRouter,
																					&iIdMensaje,
																					&strToken,
																					&sToken,
																					iLargoToken,
																					CABECERA_RouterRouter,
																					&sCabeceraRouter,
																					iGameOver
																			);

									break;
								}


								lstDISCOVERsentAux = lstDISCOVERsent;
								/* Recorre la lista en busqueda de mensajes DISCOVER buscando por IDMENSAJE */
								while ( lstDISCOVERsentAux != NULL && lstDISCOVERsentAux->info.IdMensaje != CABECERA_RouterRouter.IdMensaje )
								{
									lstDISCOVERsentAux = (DISCOVERSENT *) lstDISCOVERsentAux->sgte;
								}

								if ( lstDISCOVERsentAux != NULL ) /* SE ENVIA EL FOUND AL ROUTER ORIGEN */
								{
									/* Que no propague si TTL == 1 */
									/*if ( CABECERA_RouterRouter.TTL > 1 )
									{*/
										/* Se envian los datos de Ubicacion de la Ciudad del Equipo buscado */
										EnviaCabeceraRouter( lstDISCOVERsentAux->info.FdOrigen, &(CABECERA_RouterRouter.IdMensaje), BE_FOUND, sizeof(strFound), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER );

										if ( send( lstDISCOVERsentAux->info.FdOrigen, &strFound, CABECERA_RouterRouter.LargoMensaje, 0 ) == -1 )
											WriteLog(ERROR, "Router", "-2565- No se pudo enviar FOUND a Router");
										/*}*/

									/* Se elimina el mensaje de la lista */
									lstDISCOVERsent = SuprimeNodoDiscoverSent( lstDISCOVERsent, lstDISCOVERsentAux );
								}
								else
								{
									/* JAMAS DEBERIA ENTRAR ACA */
								}


					/**************************************************************************************/
					/**/break;		/*					MENSAJE FOUND :: END							*/
					/**************************************************************************************/


					/**************************************************************************************/
					/**/case CR_CONECTA:	/*********** CONECTA CON CIUDAD:: BEGIN ***************/
					/************************************************************************************/

							if ( recv( iFdCiudad, &sNombreCiudad, sizeof(sNombreCiudad), 0 ) == -1 )
							{
								WriteLog( ERROR, "Router", "Error:227: Error al recibir nombre de Ciudad" );

								ROUTER_MuereCiudad	(
																				&iFdCiudad,
																				sNombreCiudad,
																				&iFdTokenOrigen,
																				&lstFdRouter,
																				&iIdMensaje,
																				&strToken,
																				&sToken,
																				iLargoToken,
																				CABECERA_RouterRouter,
																				&sCabeceraRouter,
																				iGameOver
																		);

								break;
							}
							else
							{
								printf("- ROUTER: Se conecto la ciudad \"%s\"\n", sNombreCiudad);
							}

					/**************************************************************************************/
					/**/break;		/*					CONECTA CON CIUDAD :: END				 **/
					/**************************************************************************************/

					/**************************************************************************************/
					/**/case RRC_TOKEN:	/*********** RECIBE TOKEN DE CIUDAD :: BEGIN ***************/
					/************************************************************************************/


								/* Guarda el largo del TOKEN */
								iLargoToken = CABECERA_RouterRouter.LargoMensaje;

								sTokenTmp = (char *) malloc ( iLargoToken );

								/* Recibe el TOKEN de CIUDAD */
								if ( recv( iFdCiudad, sTokenTmp, iLargoToken, 0 ) < 1 )
								{
									printf("- ROUTER: No se pudo recibir el TOKEN de Ciudad\n");

									ROUTER_MuereCiudad	(
																					&iFdCiudad,
																					sNombreCiudad,
																					&iFdTokenOrigen,
																					&lstFdRouter,
																					&iIdMensaje,
																					&strToken,
																					&sToken,
																					iLargoToken,
																					CABECERA_RouterRouter,
																					&sCabeceraRouter,
																					iGameOver
																			);

									free( sTokenTmp );
									sTokenTmp = NULL;

									break;
								}

								/* Guarda el TOKEN recibido en sToken */
								sToken = (char *) malloc ( iLargoToken );
								memcpy( sToken, sTokenTmp, iLargoToken );
								free( sTokenTmp );
								sTokenTmp = NULL;

								/* Obtiene el TOKEN como esctructura */
								DesaplanaToken ( &strToken, sToken );

								printf("- ROUTER: Recibe TOKEN de Ciudad\n");

								/* Primero trata de enviarle el TOKEN a algun ROUTER que todavia no este cargado en el mismo */
								iEnvioCorrecto = ROUTER_EnviaTokenNuevoRouter ( &lstFdRouter, &iIdMensaje, &sToken, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter );

								if ( iEnvioCorrecto == FALSE ) /* No hay ROUTERS nuevos conectados para enviarle el TOKEN */
								{
									/* Se envia el TOKEN al ROUTER de Origen (si existe) */
									if ( iFdTokenOrigen != -1 )
									{
										EnviaCabeceraRouter(iFdTokenOrigen, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER);
										if ( send(iFdTokenOrigen, sToken, iLargoToken, 0) > 0 )
										{
											/* Se envio el TOKEN al Origen correctamente => Libera memoria */
											free ( sToken );
											sToken = NULL;

											iFdTokenOrigen = -1;
										}
										else /* Fallo el envio del TOKEN al Router Origen */
										{
											ROUTER_EnviaToken	(
																						sNombreCiudad,
																						&iFdTokenOrigen,
																						&lstFdRouter,
																						&iIdMensaje,
																						&strToken,
																						&sToken,
																						iLargoToken,
																						CABECERA_RouterRouter,
																						&sCabeceraRouter,
																						iGameOver
																				);
										}
									}

									/* Aun no se envio el TOKEN a Ciudad para que juegue la LEV (interna y externa) */
									else if ( iJugoLEVCiudad == FALSE )
									{
										/* Envia el TOKEN a la Ciudad para que empiece a jugar su LEV interna */
										EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

										if ( send( iFdCiudad, sToken, iLargoToken, 0 ) < 1 )
										{
											ROUTER_MuereCiudad	(
																							&iFdCiudad,
																							sNombreCiudad,
																							&iFdTokenOrigen,
																							&lstFdRouter,
																							&iIdMensaje,
																							&strToken,
																							&sToken,
																							iLargoToken,
																							CABECERA_RouterRouter,
																							&sCabeceraRouter,
																							iGameOver
																					);

											break;
										}

										iJugoLEVCiudad = TRUE;
									}

									/* Ya hicimos todo lo que teniamos que hacer.. => Simplemente nos dedicamos a pasar el TOKEN al siguiente ROUTER */

									else
									{
										ROUTER_EnviaToken	(
																					sNombreCiudad,
																					&iFdTokenOrigen,
																					&lstFdRouter,
																					&iIdMensaje,
																					&strToken,
																					&sToken,
																					iLargoToken,
																					CABECERA_RouterRouter,
																					&sCabeceraRouter,
																					iGameOver
																			);
									}

								}


					/**************************************************************************************/
					/**/break;		/*					RECIBE TOKEN DE CIUDAD :: END				 **/
					/**************************************************************************************/

					}
				}
				else
				{
					/* ################# MURIO CIUDAD :: BEGIN ################# */

					/* Se llama a la funcion que procesa la muerte de la Ciudad */
					ROUTER_MuereCiudad	(
																	&iFdCiudad,
																	sNombreCiudad,
																	&iFdTokenOrigen,
																	&lstFdRouter,
																	&iIdMensaje,
																	&strToken,
																	&sToken,
																	iLargoToken,
																	CABECERA_RouterRouter,
																	&sCabeceraRouter,
																	iGameOver
															);

					/* ################# MURIO CIUDAD :: END ################# */
				}

			}

			/**************************************************************************************/
			/* #################### HABLO CIUDAD :: END #################### */
			/**************************************************************************************/



			/**************************************************************************************/
			/* #################### HABLO MAIN :: BEGIN #################### */
			/**************************************************************************************/

			if ( FD_ISSET(iFdMain,&BolsaDescriptores) )
			{
				if ( RecibeCabecera(iFdMain,&CABECERA_MainRouter) > 0 )
				{
					switch ( CABECERA_MainRouter.TipoMensaje )
					{
					/**************************************************************************************/
					/**/case TKN_NEW:	/*********** CREA EL TOKEN, LO ENVIA A CIUDAD  :: BEGIN *************/
					/*************************************************************************************/

								/* Envio del TOKEN inicializado a la CIUDAD */
								strToken.IdJuego = GetRand( 7, 913 ); /* ID del Juego */
								strToken.lstCiudades = NULL;
								strToken.lstTdp = NULL;

								iLargoToken = AplanaToken( strToken, &sToken );

								iTokenRecibido = TRUE;

								printf("- ROUTER: Se ha creado un juego!\n");

								iEsNew = TRUE;


					/**************************************************************************************/
					/**/break;		/*					CREA EL TOKEN, LO ENVIA A CIUDAD :: END				 **/
					/**************************************************************************************/


					/**************************************************************************************/
					/**/case TKN_JOIN:	/*********** CONECTA ROUTER CON OTRO :: BEGIN ********************/
					/*************************************************************************************/

								if ( (iFdRouter = AbreConexion (Cfg.RouterRemoteIP, Cfg.RouterRemotePort)) > 0 ) /* POR CFG */
								{
									/* Le envia un mensaje al ROUTER remoto diciendole que debe pasarle el TOKEN (para insertar nuestra Ciudad) */
									EnviaCabeceraRouter( iFdRouter, &iIdMensaje, RRC_TOKEN_GIVE, 0, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER );

									/* Inserta el FD del Router a la lista */
									lstFdRouterInfo.FdRouter = iFdRouter;
									lstFdRouterInfo.TokenGive = FALSE; /* Jamas nos va a pedir el TOKEN ya que el mismo nos lo va a pasar */
									lstFdRouter = InsertaNodoFDROUTER( lstFdRouter, lstFdRouterInfo );

									/* El TOKEN vendra SI o SI por este ROUTER que nos acabamos de conectar.. */
									iFdTokenOrigen = iFdRouter; /* ..se guarda el FD Origen */

									WriteLog( INFO, "Router", "Se ha conectado con el ROUTER remoto" );
									printf("- ROUTER: Se ha conectado con el ROUTER remoto\n");

								}
								else
								{
									WriteLog( ERROR, "Router", "No se pudo conectar con el ROUTER remoto" );
									printf("- ROUTER: No se pudo conectar con el ROUTER remoto\n");
								}

					/**************************************************************************************/
					/**/break;		/*					CONECTA ROUTER CON OTRO :: END				 **/
					/**************************************************************************************/


					/**************************************************************************************/
					/**/case TKN_END:	/*********** FINALIZA EL JUEGO  :: BEGIN *************/
					/*************************************************************************************/

								/* Se setea el FLAG de Fin de Juego */
								iGameOver = TRUE;
								iGameOverAux = TRUE;


					/**************************************************************************************/
					/**/break;		/*					FINALIZA EL JUEGO :: END				 **/
					/**************************************************************************************/

						case ADDCITY:

							if ( iEsNew == TRUE )
							{
								/* Envia TOKEN a CIUDAD */
								EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

								if ( send( iFdCiudad, sToken, iLargoToken, 0 ) < 1 )
								{
									/* Hacemos NEW y no pudimos enviar el TOKEN a Ciudad => Estamos fritosssss */
									close( iFdCiudad );
									iFdCiudad = -1;

									printf("\n\n++++++ *** Murio la CIUDAD! Actuando como puente a partir de ahora.. *** ++++++\n\n");

								}
							}
						break;
					}
				}
			}

			/**************************************************************************************/
			/* #################### HABLO MAIN :: END #################### */
			/**************************************************************************************/




			/**************************************************************************************/
			/* #################### HABLO ALGUN ROUTER :: BEGIN #################### */
			/**************************************************************************************/

			lstFdRouterAux = lstFdRouter;
			while ( lstFdRouterAux != NULL )
			{
				if ( FD_ISSET(lstFdRouterAux->info.FdRouter, &BolsaDescriptores) )
				{

					if ( RecibeCabeceraRouter(lstFdRouterAux->info.FdRouter, &sCabeceraRouter, &CABECERA_RouterRouter) > 0 )
					{
						switch ( CABECERA_RouterRouter.TipoMensaje )
						{
						/**************************************************************************************/
						/**/case BE_DISCOVER:	/*************** MENSAJE DISCOVER :: BEGIN ******************/
						/**************************************************************************************/

									/* Recibe DISCOVER de otro Router y lo propaga */
									if ( recv(lstFdRouterAux->info.FdRouter, &strDiscover, CABECERA_RouterRouter.LargoMensaje, 0) == -1 )
									{
										WriteLog( ERROR, "Router", "Error:1228: Error al recibir Discover de Router" );
										break;
									}

									/* Inserta el mensaje en la lista de espera */
									lstDISCOVERsent = InsertaNodoDiscoverSent( lstDISCOVERsent, lstFdRouterAux->info.FdRouter, CABECERA_RouterRouter.IdMensaje );

									/* La CIUDAD del Equipo a buscar somos nosotros */
									if ( strcmp(strDiscover.Ciudad, sNombreCiudad) == 0 )
									{
										printf("- ROUTER: Enviando DISCOVER a Ciudad..\n");

										/* Pregunta a su Ciudad si tiene al equipo */
										EnviaCabeceraRouter( iFdCiudad, &(CABECERA_RouterRouter.IdMensaje), BE_DISCOVER, sizeof(DISCOVER), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

										if ( send( iFdCiudad, &strDiscover, sizeof(DISCOVER), 0 ) < 1 )
										{
											/* Se llama a la funcion que procesa la muerte de la Ciudad */
											ROUTER_MuereCiudad	(
																							&iFdCiudad,
																							sNombreCiudad,
																							&iFdTokenOrigen,
																							&lstFdRouter,
																							&iIdMensaje,
																							&strToken,
																							&sToken,
																							iLargoToken,
																							CABECERA_RouterRouter,
																							&sCabeceraRouter,
																							iGameOver
																					);

										}

									}
									/* Continua con la propagacion del DISCOVER */
									else
									{
										/*printf("- ROUTER: Propagando DISCOVER.. (recibido de Router)\n");*/

										/* Recorre la lista de los Routers conectados y propaga */
										lstFdRouterAuxTmp = lstFdRouter;

										while (lstFdRouterAuxTmp != NULL)
										{
											/* Excepto del que recibo */
											if (lstFdRouterAux->info.FdRouter != lstFdRouterAuxTmp->info.FdRouter)
											{
												/* Que no propague si TTL == 1 */
												if ( CABECERA_RouterRouter.TTL > 1 )
												{
													EnviaCabeceraRouter( lstFdRouterAuxTmp->info.FdRouter, &(CABECERA_RouterRouter.IdMensaje), BE_DISCOVER, sizeof(DISCOVER), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER );
													if ( send( lstFdRouterAuxTmp->info.FdRouter, &strDiscover, sizeof(DISCOVER), 0 ) == -1 )
														WriteLog(ERROR, "Router", "-2222- No se pudo propagar mensaje DISCOVER a un Router");
												}
											}

											lstFdRouterAuxTmp = (FDROUTER *) lstFdRouterAuxTmp->sgte;
										}
									}



						/**************************************************************************************/
						/**/break;		/*					MENSAJE DISCOVER :: END					 **/
						/**************************************************************************************/


						/**************************************************************************************/
						/**/case BE_FOUND:	/*********** MENSAJE FOUND :: BEGIN ***************/
						/**************************************************************************************/

									/* Recibe FOUND de otro Router */
									if ( recv(lstFdRouterAux->info.FdRouter, &strFound, CABECERA_RouterRouter.LargoMensaje, 0) == -1 )
									{
										WriteLog( ERROR, "Router", "Error:1228: Error al recibir DISCOVER de Router" );
										break;
									}

									/*printf("- ROUTER: Enviando FOUND a Ciudad..\n");*/

									lstDISCOVERsentAux = lstDISCOVERsent;
									/* Recorre la lista en busqueda de mensajes DISCOVER buscando por IDMENSAJE */
									while ( lstDISCOVERsentAux != NULL && lstDISCOVERsentAux->info.IdMensaje != CABECERA_RouterRouter.IdMensaje )
									{
										lstDISCOVERsentAux = (DISCOVERSENT *) lstDISCOVERsentAux->sgte;
									}

									if ( lstDISCOVERsentAux != NULL ) /* SE ENVIA EL FOUND AL ROUTER ORIGEN */
									{
										/* Que no propague si TTL == 1 */
										/*if ( CABECERA_RouterRouter.TTL > 1 )
										{*/
											/* Se envian los datos de Ubicacion de la Ciudad del Equipo buscado */
											EnviaCabeceraRouter( lstDISCOVERsentAux->info.FdOrigen, &(CABECERA_RouterRouter.IdMensaje), BE_FOUND, sizeof(strFound), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER );

											if ( send( lstDISCOVERsentAux->info.FdOrigen, &strFound, CABECERA_RouterRouter.LargoMensaje, 0 ) == -1 )
												WriteLog(ERROR, "Router", "-2565- No se pudo enviar FOUND a Router");
											/*}*/

										/* Se elimina el mensaje de la lista */
										lstDISCOVERsent = SuprimeNodoDiscoverSent( lstDISCOVERsent, lstDISCOVERsentAux );
									}
									else /* SE ENVIA EL FOUND A LA CIUDAD */
									{

										/* Se envian los datos de Ubicacion de la Ciudad del Equipo buscado */
										EnviaCabeceraRouter( iFdCiudad, &(CABECERA_RouterRouter.IdMensaje), BE_FOUND, sizeof(strFound), CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

										if ( send( iFdCiudad, &strFound, CABECERA_RouterRouter.LargoMensaje, 0 ) < 1 )
										{
											/* Se llama a la funcion que procesa la muerte de la Ciudad */
											ROUTER_MuereCiudad	(
																							&iFdCiudad,
																							sNombreCiudad,
																							&iFdTokenOrigen,
																							&lstFdRouter,
																							&iIdMensaje,
																							&strToken,
																							&sToken,
																							iLargoToken,
																							CABECERA_RouterRouter,
																							&sCabeceraRouter,
																							iGameOver
																					);
										}

									}


						/**************************************************************************************/
						/**/break;		/*					MENSAJE FOUND :: END				 **/
						/**************************************************************************************/


						/**************************************************************************************/
						/**/case RRC_TOKEN_GIVE:	/*********** UN ROUTER NOS PIDE EL TOKEN :: BEGIN *********/
						/**************************************************************************************/

									/* Se actualiza el Flag de envio de TOKEN para este Router */
									lstFdRouterAux->info.TokenGive = TRUE;

						/**************************************************************************************/
						/**/break;		/*					UN ROUTER NOS PIDE EL TOKEN :: END				 **/
						/**************************************************************************************/


						/**************************************************************************************/
						/**/case RRC_TOKEN_END:	/*********** UN ROUTER NOS ENVIA END JUEGO :: BEGIN ***********/
						/**************************************************************************************/

								/* Guarda el largo del TOKEN */
								iLargoToken = CABECERA_RouterRouter.LargoMensaje;

								sToken = (char *) malloc ( iLargoToken );

								/* Recibe el TOKEN del ROUTER */
								if (recv( lstFdRouterAux->info.FdRouter, sToken, iLargoToken, 0 ) < 1)
									printf("- ROUTER: No se pudo recibir el Token\n");

								printf("- ROUTER: Recibe FIN DE JUEGO de Router\n");

								if ( iGameOverAux == FALSE )
								{
									/* Envia TOKEN a CIUDAD para que Imprima los Resultados (no hace falta comprobar su muerte) */

									EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_END, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );
									send( iFdCiudad, sToken, iLargoToken, 0 );

									/* Propaga mensaje de fin de juego a todos los ROUTERS */
									iEnvioCorrecto = ROUTER_EnviaTokenProximoRouter ( sNombreCiudad, &iIdMensaje, &sToken, &iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, iGameOver );
								}

								/* Se setea el FLAG de Fin de Juego */
								iGameOver = TRUE;
								iGameOverAux = TRUE;


						/**************************************************************************************/
						/**/break;		/*				UN ROUTER NOS ENVIA END JUEGO  :: END				**/
						/**************************************************************************************/


						/**************************************************************************************/
						/**/case RRC_TOKEN:	/*********** RECIBE TOKEN DE OTRO ROUTER :: BEGIN ***************/
						/************************************************************************************/

								/* Guarda el largo del TOKEN */
								iLargoToken = CABECERA_RouterRouter.LargoMensaje;

								/* Si nunca se recibio el TOKEN entonces se guarda el FD del Router de Origen */
								/*
								if( iTokenRecibido == FALSE )
									iFdTokenOrigen = lstFdRouterAux->info.FdRouter;
								*/

								sToken = (char *) malloc ( iLargoToken );

								/* Recibe el TOKEN del ROUTER */
								if ( recv( lstFdRouterAux->info.FdRouter, sToken, iLargoToken, 0 ) < 1 )
								{
									printf("++++++++ *** - ROUTER: No se pudo recibir el TOKEN. El Torneo se ha interrumpido. *** ++++++++\n");
									WriteLog(ERROR, "Router", " No se pudo recibir el TOKEN. El Torneo se ha interrumpido");

									break;
								}

								printf("- ROUTER: Recibe TOKEN de Router\n");

								/* Obtiene el TOKEN como esctructura */
								DesaplanaToken ( &strToken, sToken );

								/* Si la CIUDAD ha muerto y debemos eliminar sus Equipos del TOKEN.. */
								if ( iFdCiudad == -2 )
								{
									/* Se llama a la funcion que procesa la muerte de la Ciudad */
									ROUTER_MuereCiudad	(
																					&iFdCiudad,
																					sNombreCiudad,
																					&iFdTokenOrigen,
																					&lstFdRouter,
																					&iIdMensaje,
																					&strToken,
																					&sToken,
																					iLargoToken,
																					CABECERA_RouterRouter,
																					&sCabeceraRouter,
																					iGameOver
																			);

									break;
								}

								/* La Ciudad ha muerto y ya fue procesada */
								if ( iFdCiudad == -1 )
								{
									iTokenRecibido = TRUE;
									iJugoLEVCiudad = TRUE;
								}


								/* Si es la primera vez que este ROUTER recibe el TOKEN, se lo envia a su Ciudad para que cargue los Equipos */
								if ( iTokenRecibido == FALSE )
								{

									/* Envia el TOKEN a la Ciudad para que inserte sus Equipos */
									EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

									if ( send( iFdCiudad, sToken, iLargoToken, 0 ) < 1 )
									{
										/* Se llama a la funcion que procesa la muerte de la Ciudad */
										ROUTER_MuereCiudad	(
																						&iFdCiudad,
																						sNombreCiudad,
																						&iFdTokenOrigen,
																						&lstFdRouter,
																						&iIdMensaje,
																						&strToken,
																						&sToken,
																						iLargoToken,
																						CABECERA_RouterRouter,
																						&sCabeceraRouter,
																						iGameOver
																				);
									}

									iTokenRecibido = TRUE;


								}

								/* Si todavia este ROUTER no devolvio el TOKEN al origen */
								else if ( iFdTokenOrigen != -1 )
								{

									/* Primero trata de enviarle el TOKEN a algun ROUTER que todavia no este cargado en el mismo */
									iEnvioCorrecto = ROUTER_EnviaTokenNuevoRouter ( &lstFdRouter, &iIdMensaje, &sToken, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter );

									if ( iEnvioCorrecto == FALSE ) /* No hay ROUTERS nuevos conectados para enviarle el TOKEN => Lo devuelve al origen */
									{
										/* Se envia el TOKEN al ROUTER de Origen */
										if (
											EnviaCabeceraRouter(iFdTokenOrigen, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_ROUTER) > 0 && send(iFdTokenOrigen, sToken, iLargoToken, 0) > 0
											 )
										{
											/* Se envio el TOKEN al Origen correctamente => Libera memoria */
											free ( sToken );
											sToken = NULL;

											iFdTokenOrigen = -1;
											break;
										}
										else /* No se pudo enviar el TOKEN al Origen */
										{
											close( iFdTokenOrigen );
											iFdTokenOrigen = -1;

											ROUTER_EnviaToken	(
																						sNombreCiudad,
																						&iFdTokenOrigen,
																						&lstFdRouter,
																						&iIdMensaje,
																						&strToken,
																						&sToken,
																						iLargoToken,
																						CABECERA_RouterRouter,
																						&sCabeceraRouter,
																						iGameOver
																				);

										}

									}
								}

								/* Se le envia el TOKEN a la CIUDAD para que juegue su Lev (interna y externa) */
								else if ( iJugoLEVCiudad == FALSE )
								{
									/* Primero trata de enviarle el TOKEN a algun ROUTER que todavia no este cargado en el mismo */
									iEnvioCorrecto = ROUTER_EnviaTokenNuevoRouter ( &lstFdRouter, &iIdMensaje, &sToken, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter );

									if ( iEnvioCorrecto == FALSE ) /* No hay ROUTERS nuevos conectados para enviarle el TOKEN */
									{

										/* Envia el TOKEN a la Ciudad para que empiece a jugar su LEV (interna y externa) */
										if ( EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD ) < 1 || send( iFdCiudad, sToken, iLargoToken, 0 ) < 1 )
										{
											/* Se llama a la funcion que procesa la muerte de la Ciudad */
											ROUTER_MuereCiudad	(
																							&iFdCiudad,
																							sNombreCiudad,
																							&iFdTokenOrigen,
																							&lstFdRouter,
																							&iIdMensaje,
																							&strToken,
																							&sToken,
																							iLargoToken,
																							CABECERA_RouterRouter,
																							&sCabeceraRouter,
																							iGameOver
																					);
										}

										iJugoLEVCiudad = TRUE;
									}
								}

								/* Ya hicimos todo lo que teniamos que hacer.. => Simplemente nos dedicamos a pasar el TOKEN al siguiente ROUTER */
								/*
									Dentro de este else habria que agregar algun chequeo de nuevos equipos cargados en el TOKEN y que todavia
									no hayan jugado contra los de nuestra Ciudad
								*/
								else
								{
									/*
										Esta espera es para reducir la frecuencia con la que se pasa el TOKEN
										cuando se esta en esta etapa de recibir y enviar automaticamente el TOKEN..
										Asi se logra reducir la carga del sistema en general, cuando por ejemplo,
										solo hay 2 Routers en el juego y han llegado a esta etapa.. por lo cual
										se pasan el TOKEN el uno al otro constantemente sin parar
									*/
									sleep( 1 );


									/* Primero trata de enviarle el TOKEN a algun ROUTER que todavia no este cargado en el mismo */
									iEnvioCorrecto = ROUTER_EnviaTokenNuevoRouter ( &lstFdRouter, &iIdMensaje, &sToken, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter );

									/* No hay ROUTERS nuevos conectados para enviarle el TOKEN => Lo pasa al siguiente ROUTER */
									if ( iEnvioCorrecto == FALSE )
									{

										if ( iGameOver == TRUE ) /* Se ha recibido la orden de Finalizar el Juego */
										{
											/* Envia TOKEN a CIUDAD para que Imprima los Resultados */
											/*aca no hace falta comprobar la muerte de la ciudad*/
											EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_END, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );
											send( iFdCiudad, sToken, iLargoToken, 0 );

											/* Propaga mensaje de fin de juego a todos los ROUTERS */
											iEnvioCorrecto = ROUTER_EnviaTokenProximoRouter ( sNombreCiudad, &iIdMensaje, &sToken, &iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, iGameOver );

										}
										else if ( iFdCiudad > -1 )
										{
											/* Envia el TOKEN a la Ciudad para que empiece a jugar su LEV */
											EnviaCabeceraRouter( iFdCiudad, &iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, &sCabeceraRouter, ROUTER_CIUDAD );

											if ( send( iFdCiudad, sToken, iLargoToken, 0 ) < 1 )
											{
												/* Se llama a la funcion que procesa la muerte de la Ciudad */
												ROUTER_MuereCiudad	(
																								&iFdCiudad,
																								sNombreCiudad,
																								&iFdTokenOrigen,
																								&lstFdRouter,
																								&iIdMensaje,
																								&strToken,
																								&sToken,
																								iLargoToken,
																								CABECERA_RouterRouter,
																								&sCabeceraRouter,
																								iGameOver
																						);

												break;
											}

										}
										else
										{
											ROUTER_EnviaToken	(
																						sNombreCiudad,
																						&iFdTokenOrigen,
																						&lstFdRouter,
																						&iIdMensaje,
																						&strToken,
																						&sToken,
																						iLargoToken,
																						CABECERA_RouterRouter,
																						&sCabeceraRouter,
																						iGameOver
																				);
										}

									}

								}

						/**************************************************************************************/
						/**/break;		/*					RECIBE TOKEN DE OTRO ROUTER :: END				 **/
						/**************************************************************************************/

						}
					}
					else
					{

						/* ################# MURIO UN ROUTER :: BEGIN ################# */

						lstFdRouter = SuprimeNodoFDROUTER(lstFdRouter, lstFdRouterAux->info);

						lstFdRouterAux = NULL;

						/*
						printf("\n\n++++++ *** Murio un ROUTER! *** ++++++\n\n");
						WriteLog(ERROR, "Router", "-219231- Fallecio un Router");
						*/

						/* ################# MURIO UN ROUTER :: END ################# */
					}

				}

				if ( lstFdRouterAux == NULL )
				{
					/* Murio algun Router => Se inicializa el puntero */
					lstFdRouterAux = lstFdRouter;
				}
				else
				{
					/* Desplaza el puntero hacia el siguiente FD para chequear si nos hablo */
					lstFdRouterAux = (FDROUTER *) lstFdRouterAux->sgte;
				}
			}

			/**************************************************************************************/
			/* #################### HABLO ALGUN ROUTER :: END #################### */
			/**************************************************************************************/



			/**************************************************************************************/
			/* #################### HABLO ROUTER NUEVO :: BEGIN #################### */
			/**************************************************************************************/

			if ( FD_ISSET(iFdEscucha,&BolsaDescriptores) )
			{
				/* Se conecto un ROUTER */
				if ( (iFdRouter = AceptaConexion(iFdEscucha,NULL,NULL)) != 1 )
				{
					/* Inserta el FD del ROUTER a la lista */
					lstFdRouterInfo.FdRouter = iFdRouter;
					lstFdRouterInfo.TokenGive = FALSE; /* Se asume que NO nos va a pedir el TOKEN */
					lstFdRouter = InsertaNodoFDROUTER( lstFdRouter, lstFdRouterInfo );

					WriteLog( INFO, "Router", "Un router se ha conectado" );
					printf("- ROUTER: Un router se ha conectado\n");
				}
				else
				{
					WriteLog( INFO, "Router", "No se pudo conectar nuevo Router" );
					printf("- ROUTER: No se pudo conectar nuevo Router\n");
				}

			}

			/**************************************************************************************/
			/* #################### HABLO ROUTER NUEVO :: END #################### */
			/**************************************************************************************/

		}


	}

	return 0;
}
