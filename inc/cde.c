/*
	Ultima modificacion: 15/05 - 05:30
	Modificado por: Diego
*/

#include "./headers/global_includes.h"
#include "./headers/defines.h"
#include "./inc/config.c"
#include "./inc/log.c"
#include "./socketservidor.c"
#include "./socketcliente.c"
#include "./inc/varias.c"


int main ( int iArgs, char *aArgs[] )
{
	/* FD de CIUDAD (sFdEquipo en ciudad.c) */
	int iFdCiudad;

	/* FD de EQUIPO que quiere usar el CDE */
	int iFdEquipo;

	/* FD Escucha para los partidos con otros equipos */
	int iFdEscucha = FD_CDE;

	/* Bolsa de descriptores */
	fd_set BolsaDescriptores;

	/* El valor del FD mas alto */
	int iMaximo, iNroEquipos=0;

	/* El valor que devuelve la funcion select */
	int iValRet;

	/* Equipo a buscar recibido de Ciudad */
	char sEquipo[30];

	/* String para guardar en el LOG */
	char sLog[250];

	/* Archivo Cfg */
	CFG Cfg;

	/* Estructura del equipo: Nombre, Estado y Tiempo que quiere entrenar */
	EQUIPO_CDE strEquipoCDE;

	/* Timestamp */
	time_t tTimestamp;

	/* TimeOut select */
	struct timeval strTimeOut;

	/* Lista de Equipos usando el CDE */
	CDEEQUIPOS *lstCDEequipos = NULL, *lstCDEequiposAux;
	CDEEQUIPOSinfo lstCDEequiposInfo;

	/* Cola de Equipos esperando por el CDE */
	CDECOLAEQUIPOS *lstCDEcola = NULL, *lstCDEcolaAux;
	CDECOLAEQUIPOSinfo lstCDEcolaInfo;

	/* Para ignorar el SIGPIPE (si el send da -1, linux hace giladas) */
	signal(SIGPIPE, SIG_IGN);

	/* Llamado a la función que carga el archivo de configuración */
	if ( CargaConfig(&Cfg) == FAIL ) {
		printf( "\n\nERROR FATAL: No se puede cargar el archivo de configuracion\n\n");
		return FAIL;
	}

	WriteLog( INFO, "CDE", "CDE iniciado" );

	/* Obtiene el FD de CIUDAD pasado por parametro */
	sscanf( aArgs[1], "%d" , &iFdCiudad );


	while( 1 )
	{
		FD_ZERO ( &BolsaDescriptores );
		FD_SET ( iFdCiudad,&BolsaDescriptores );
		FD_SET ( iFdEscucha, &BolsaDescriptores );

		iMaximo = iFdCiudad;

		if ( iMaximo < iFdEscucha )
			iMaximo = iFdEscucha;

		strTimeOut.tv_usec = 0;
		strTimeOut.tv_sec = 1;

		/* SELECT */
		iValRet = select( iMaximo+1, &BolsaDescriptores, NULL,NULL, &strTimeOut );

		/* Mensaje recibido desde algun FD de la bolsa */
		if ( iValRet > 0 )
		{

			/**************************************************************************************/
			/**/if ( FD_ISSET(iFdCiudad,&BolsaDescriptores) )/**/
						/**************************************************************************************/
					{
						/* CIUDAD pide que un EQUIPO sea expulsado del CDE */

						if ( recv( iFdCiudad, sEquipo, 30, 0 ) < 1 ) /* Recibe el nombre del equipo a ser expulsado */
						{
							WriteLog( ERROR, "CDE", "MURIO LA CIUDAD -- Autodestruyendo este CDE.." );
							exit(0);
						}

						/* Comienza la busqueda del equipo en lista de equipos entrenando actualmente */
						lstCDEequiposAux = lstCDEequipos;

						while ( lstCDEequiposAux != NULL && strcmp(lstCDEequiposAux->info.Equipo, sEquipo) != 0)
						{
							lstCDEequiposAux = (CDEEQUIPOS *)lstCDEequiposAux->sgte;
						}

						if (lstCDEequiposAux != NULL) /* Si el equipo se encuentra entrenando en este momento */
						{
							iNroEquipos--;

							/* Le envia mensaje al equipo avisandole que finalizo su uso del CDE */
							send( lstCDEequiposAux->info.FdEquipo, "1", sizeof(int), 0 );

							/* Cierra conexion con el equipo */
							close (lstCDEequiposAux->info.FdEquipo);

							printf("-- CDE: EXPULSA a \"%s\"\n", lstCDEequiposAux->info.Equipo);

							/* Escribe en el LOG */
							sprintf( sLog, "EXPULSA a \"%s\"", lstCDEequiposAux->info.Equipo );
							WriteLog( INFO, "CDE", sLog );

							lstCDEequipos = SuprimeNodoCDE( lstCDEequipos, lstCDEequiposAux );

						}
						else /* El equipo NO se encuentra entrenando en este momento => Chequea la cola de espera */
						{
							lstCDEcolaAux = lstCDEcola;

							while (lstCDEcolaAux != NULL && strcmp(lstCDEcolaAux->info.Equipo, sEquipo) != 0)
							{
								lstCDEcolaAux = (CDECOLAEQUIPOS *)lstCDEcolaAux->sgte;
							}

							if (lstCDEcolaAux != NULL) /* Si el equipo se encuentra encolado en este momento */
							{

								/* Le envia mensaje al equipo avisandole que finalizo su uso del CDE */
								send( lstCDEcolaAux->info.FdEquipo, "1", sizeof(int), 0 );

								/* Cierra conexion con el equipo */
								close (lstCDEcolaAux->info.FdEquipo);

								printf("-- CDE: EXPULSA a \"%s\"\n", lstCDEcolaAux->info.Equipo);

								/* Escribe en el LOG */
								sprintf( sLog, "EXPULSA a \"%s\"", lstCDEcolaAux->info.Equipo );
								WriteLog( INFO, "CDE", sLog );

								lstCDEcola = SuprimeNodoColaCDE( lstCDEcola, lstCDEcolaAux);

							}
						}

					}
					/**************************************************************************************/


					/**************************************************************************************/
					/**/if ( FD_ISSET(iFdEscucha,&BolsaDescriptores) )									/**/
								/**************************************************************************************/
							{
								/* Acepta una conexion */
								iFdEquipo = AceptaConexion(iFdEscucha,NULL,NULL);

								/* Recibe la estructura del Equipo: Estado, nombre y tiempo a entrenar */
								recv( iFdEquipo, &strEquipoCDE, sizeof(strEquipoCDE), 0 );

				/*
								Busca y verifica que el equipo que pide el CDE no este ya en el CDE
				*/
								lstCDEequiposAux = lstCDEequipos;
								/* Busca en la LISTA */
								while ( lstCDEequiposAux != NULL && strcmp(lstCDEequiposAux->info.Equipo, strEquipoCDE.Equipo) != 0 )
								{
									lstCDEequiposAux = (CDEEQUIPOS *)lstCDEequiposAux->sgte;
								}

								lstCDEcolaAux = lstCDEcola;
								/* Busca en la COLA */
								while ( lstCDEcolaAux != NULL && strcmp(lstCDEcolaAux->info.Equipo, strEquipoCDE.Equipo) != 0 )
								{
									lstCDEcolaAux = (CDECOLAEQUIPOS *)lstCDEcolaAux->sgte;
								}

								if ( lstCDEequiposAux == NULL && lstCDEcolaAux == NULL )
								{
									/* El equipo que nos hablo NO esta actualmente en el CDE => Debemos atenderlo */

									if ( iNroEquipos < Cfg.EquiposCdeMAX) /* HAY lugar disponible en el CDE para el equipo */
									{
										/* Incrementa el contador de equipos actualmente entrenando */
										iNroEquipos++;

										printf("-- CDE: INGRESA a entrenar \"%s\" por %d segundos\n", strEquipoCDE.Equipo, strEquipoCDE.TiempoPedido);

										/* Escribe en el LOG */
										sprintf( sLog, "INGRESA a entrenar \"%s\" por %d segundos", strEquipoCDE.Equipo, strEquipoCDE.TiempoPedido );
										WriteLog( INFO, "CDE", sLog );



										/* Obtiene el EPOCH actual */
										tTimestamp = GetTimestamp();

										/* Arma INFO */
										lstCDEequiposInfo.TiempoPedido = strEquipoCDE.TiempoPedido;
										lstCDEequiposInfo.FdEquipo = iFdEquipo;
										strcpy( lstCDEequiposInfo.Equipo, strEquipoCDE.Equipo );
										lstCDEequiposInfo.TiempoInicio = tTimestamp;

										/* Inserta nodo */
										lstCDEequipos = InsertaNodoCDE( lstCDEequipos, lstCDEequiposInfo );

										/*20060525***************/
										lstCDEequiposAux = lstCDEequipos;

										while ( lstCDEequiposAux != NULL )
										{
											sprintf( sLog, "Listado de Equipos entrenando: \"%s\" \n", lstCDEequiposAux->info.Equipo);
											WriteLog( INFO, "CDE", sLog );

											lstCDEequiposAux = (CDEEQUIPOS *)lstCDEequiposAux->sgte;
										}


										/************************/

									}
									else /* NO HAY lugar disponible en el CDE para el equipo */
									{
						/* ************* Se ENCOLA *************

										Usando algoritmo SPN.
						*/

										/* Arma INFO */
										lstCDEcolaInfo.TiempoPedido = strEquipoCDE.TiempoPedido;
										lstCDEcolaInfo.FdEquipo = iFdEquipo;
										strcpy( lstCDEcolaInfo.Equipo, strEquipoCDE.Equipo );

										lstCDEcola = AgregaNodoCDE( lstCDEcola, lstCDEcolaInfo );

										/* PRUEBA */
										printf("-- CDE: ENCOLA \"%s\", equipos en cola actual:\n", strEquipoCDE.Equipo );

										/* Escribe en el LOG */
										sprintf( sLog, "ENCOLA \"%s\"", strEquipoCDE.Equipo );
										WriteLog( INFO, "CDE", sLog );

										lstCDEcolaAux = lstCDEcola;
										while (lstCDEcolaAux != NULL)
										{
											printf("-- CDE: Equipo \"%s\" (%d s)\n", lstCDEcolaAux->info.Equipo, lstCDEcolaAux->info.TiempoPedido);

											lstCDEcolaAux = (CDECOLAEQUIPOS *)lstCDEcolaAux->sgte;
										}

										/* FIN PRUEBA */

									}

								}
							}
							/**************************************************************************************/


		}/* fin select sin errores */


		/* ###################### VER SI EQUIPO DEJO DE ENTRENAR ###################### */
		else if (iValRet == 0)
		{
			/* Obtiene el EPOCH actual */
			tTimestamp = GetTimestamp();

			lstCDEequiposAux = lstCDEequipos;
			/* Recorre la lista en busqueda de los equipos que hayan finalizado de usar el CDE */
			while ( lstCDEequiposAux != NULL )
			{

				if ( (tTimestamp - lstCDEequiposAux->info.TiempoInicio) >= lstCDEequiposAux->info.TiempoPedido )
				{
					/* Decrementa el contador de equipos actualmente entrenando */
					iNroEquipos--;

					/* Ya termino de entrenar */
					printf("-- CDE: TERMINA de entrenar \"%s\"\n",lstCDEequiposAux->info.Equipo);

					/* Escribe en el LOG */
					sprintf( sLog, "TERMINA de entrenar \"%s\"",lstCDEequiposAux->info.Equipo );
					WriteLog( INFO, "CDE", sLog );

					/* Le envia mensaje al equipo avisandole que finalizo su uso del CDE */
					send( lstCDEequiposAux->info.FdEquipo, "0", sizeof(int), 0 );

					/* Cierra conexion con el equipo */
					close (lstCDEequiposAux->info.FdEquipo);

					/* Suprime nodo de la lista de equipos que usan el CDE */
					lstCDEequipos = SuprimeNodoCDE( lstCDEequipos, lstCDEequiposAux );

					/* Busca en la cola aquellos equipos que esten esperando por usar el CDE */
					if (lstCDEcola != NULL)
					{

						if ( iNroEquipos < Cfg.EquiposCdeMAX) /* HAY lugar disponible en el CDE para el equipo */
						{
							/* Incrementa el contador de equipos actualmente entrenando */
							iNroEquipos++;

							/* Le da el lugar al primero en la cola de espera */
							lstCDEcola = EliminaNodoCDE (lstCDEcola, &lstCDEcolaInfo );

							/* Arma INFO */
							lstCDEequiposInfo.TiempoPedido = lstCDEcolaInfo.TiempoPedido;
							lstCDEequiposInfo.FdEquipo = lstCDEcolaInfo.FdEquipo;
							sprintf( lstCDEequiposInfo.Equipo, lstCDEcolaInfo.Equipo );
							lstCDEequiposInfo.TiempoInicio = tTimestamp;

							/* Inserta nodo */
							lstCDEequipos = InsertaNodoCDE( lstCDEequipos, lstCDEequiposInfo );

							printf("-- CDE: INGRESA a entrenar \"%s\" (salio de la cola)\n",lstCDEcolaInfo.Equipo);

							/* Escribe en el LOG */
							sprintf( sLog, "INGRESA a entrenar \"%s\" (salio de la cola)",lstCDEcolaInfo.Equipo );
							WriteLog( INFO, "CDE", sLog );
						}
					}

				}

				/*paso al siguiente nodo*/
				lstCDEequiposAux = (CDEEQUIPOS *)lstCDEequiposAux->sgte;

			}

		}


		else if (iValRet == -1) {
			printf("\nERROR: Error de interrupciones");
		}

	} /* Fin del WHILE */

	return 0;
}
