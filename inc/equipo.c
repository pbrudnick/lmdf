/*
	Ultima modificacion: 22/06 - 02:00
	Modificado por: Fernando Niembro
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

	/* FD de la Ciudad a migrar */
	int iFdCiudadMigra = -1;

	/* Datos de nuestra Ciudad Origen. Sera enviado por parametro a toda Ciudad a la cual migremos */
	char *sNombreCiudadOrigen = NULL;
	char *sIPCiudadOrigen = NULL;
	char *sPortCiudadOrigen = NULL;

	/* Nombre de la Ciudad actual en la que estamos. Sera enviado por parametro por la Ciudad a la que pertenezcamos */
	char *sNombreCiudad = NULL;

	/* Datos de Ciudad a la que se va a migrar. Sera enviado en un mensaje desde la Ciudad a la que vamos a migrar */
	char sNombreCiudadMigra[30];

	/* FD de CDE (sFdCDE en ciudad.c) */
	int iFdCDE=-1;

	/* Nombre del equipo creado */
	char *sNombreEquipo;

	/* FD Escucha para los partidos con otros equipos */
	int iFdEscucha = 2;

	/* FD del equipo visitante */
	int iFdEquipoVisitante=-1;

	/*el descriptor del equipo contra el q se solicito un partido*/
	int iFdEquipoVisitado = -1;

	/* Cadena Lev Resultados para enviarle a la Ciudad a migrar */
	char *sLevResultados = NULL;

	/* Bolsa de descriptores */
	fd_set BolsaDescriptores;

	/* El valor del FD mas alto */
	int iMaximo;

	/* El valor que devuelve la funcion select */
	int iValRet;

	/* Cabecera */
	CABECERA_CIUDADEQUIPO CABECERA_CiudadEquipo;

	/* Incremental para IdMensaje de Cabecera */
	int iIdMensaje = 0;

	/* Cadena LEV para desencriptar y pasar a lista */
	char *sLev = NULL;

	/* Argumentos de Ciudad */
	int iKI, iCansancio, iEstado, iDuracionPartidos;

	/* Estado auxiliar para asignar ENTRENANDO o DESCANSANDO si startgame==TRUE */
	int iEstadoAux = INACTIVO;

	/* Resultado del partido jugado (sirve para equipo local y visitante) */
	int iResultadoPartido = NO_JUGADO;

	/* Simplemente.. una variable al pedo (pa recibir de CDE) */
	char sAlPedo[1+1];

	/* String para guardar en el LOG */
	char sLog[250];

	/* Lista de resultados */
	LEVRESULTADOS *lstLevResultados = NULL, *lstLevResultadosAux = NULL;

	/* TimeOut select */
	struct timeval strTimeOut, *ptrTimeOut = NULL;

	/* Archivo de Configuracion */
	CFG Cfg;

	/* DEFINICION DE TIPOS */
	FOUND strDireccionCiudad;
	TIPO_4 strDireccionCDE; /* Tipo 4 */
	TIPO_7 strDireccionEquipo;
	TIPO_11 strEquipo;

	EQUIPO_CDE strEquipoCDE;

	/* Si esta migrado o no */
	int iMigrado = FALSE;

	/*para ignorar el SIGPIPE (si el send da -1, linux hace giladas)*/
	signal(SIGPIPE, SIG_IGN);

	/* ########################## INICIALIZACION BEGIN ########################## */

	/* Obtiene el nombre del equipo pasado por parametro */
	sNombreEquipo = malloc( strlen(aArgs[0]) + 1 );
	strcpy( sNombreEquipo, aArgs[0] );
	/* Obtiene el FD de CIUDAD pasado por parametro */
	sscanf( aArgs[1], "%d" , &iFdCiudad );

	sscanf( aArgs[2], "%d" , &iKI );
	sscanf( aArgs[3], "%d" , &iCansancio );
	sscanf( aArgs[4], "%d" , &iDuracionPartidos );

	/* Obtiene el Nombre de la Ciudad en la que estamos actualmente */
	sNombreCiudad = malloc( strlen(aArgs[5]) + 1 );
	strcpy( sNombreCiudad, aArgs[5] );

	/* Obtiene los datos de la Ciudad Origen */
	sNombreCiudadOrigen = malloc( strlen(aArgs[6]) + 1 );
	strcpy( sNombreCiudadOrigen, aArgs[6] );
	sIPCiudadOrigen = malloc( strlen(aArgs[7]) + 1 );
	strcpy(sIPCiudadOrigen, aArgs[7]);
	sPortCiudadOrigen = malloc( strlen(aArgs[8]) + 1 );
	strcpy(sPortCiudadOrigen, aArgs[8]);

	/* Si este Equipo esta MIGRANDO.. */
	if ( strcmp(sNombreCiudadOrigen,sNombreCiudad) != 0 )
		iMigrado = TRUE;

	WriteLog( INFO, sNombreEquipo, "Equipo iniciado" );

	/* Llamado a la función que carga el archivo de configuración */
	if ( CargaConfig(&Cfg) == FAIL )
	{
		printf( "\n\nERROR FATAL: No se puede cargar el archivo de configuracion\n\n");
		WriteLog( ERROR, sNombreEquipo, "Error:1: No se pudo cargar el archivo de configuración" );
		return FAIL;
	}

	/*inicializa el estado en inactivo (Estado inicial)*/
	iEstado = INACTIVO;

	/* ########################## INICIALIZACION END ########################## */


	while( 1 )
	{
		FD_ZERO ( &BolsaDescriptores );
		FD_SET ( iFdCiudad,&BolsaDescriptores );
		FD_SET ( iFdEscucha, &BolsaDescriptores );

		/* Se nos conectó un equipo (no tenemos LEV) */
		if ( iFdEquipoVisitante != -1 ) {
			FD_SET( iFdEquipoVisitante, &BolsaDescriptores );
		}

		if ( iFdCDE != -1 ) {
			FD_SET( iFdCDE, &BolsaDescriptores );
		}

		if ( iFdCiudadMigra != -1 ) {
			FD_SET( iFdCiudadMigra, &BolsaDescriptores );
		}

		iMaximo = iFdCiudad;

		if ( iMaximo < iFdEscucha ) {
			iMaximo = iFdEscucha;
		}

		if ( iMaximo < iFdCiudadMigra ) {
			iMaximo = iFdCiudadMigra;
		}

		if( iMaximo < iFdEquipoVisitante ) {
			iMaximo = iFdEquipoVisitante;
		}

		if( iMaximo < iFdCDE ) {
			iMaximo = iFdCDE;
		}


		strTimeOut.tv_usec = 0;

		/* Ultimo parámetro (TIMEOUT) del SELECT */
		if ( iEstado == INACTIVO )
		{
			strTimeOut.tv_sec = Cfg.TiempoInactividad; /* Tiempo inactivo por CFG */
			ptrTimeOut = &strTimeOut;
		}
		else if ( iEstado == DESCANSANDO || iEstado == ENTRENANDO )
		{
			ptrTimeOut = NULL;
		}
		else if ( iEstado == JUGANDO && iFdEquipoVisitado != -1 ) /* Somos visitantes */
		{
			/* Setea el timeout para el SELECT segun la duracion de los partidos
			** (valor generado random segun archivo config) */
			strTimeOut.tv_sec = iDuracionPartidos;
			ptrTimeOut = &strTimeOut;
		}
		else if ( iEstado == JUGANDO && iFdEquipoVisitante != -1 ) /* Somos locales */
		{
			/*
			strTimeOut.tv_sec = Cfg.DuracionPartidosMAX;
			ptrTimeOut = &strTimeOut;
			*/

			ptrTimeOut = NULL;
		}
		else
		{
			/*
			Para salvar de la desincronizacion entre CIUDAD, EQUIPO y CDE
			*/

			if ( ptrTimeOut == NULL )
			{
				/* Si el puntero es NULL => Se puede setear un nuevo TimeOut */

				strTimeOut.tv_sec = 1; /* Le damos 1 segundo de TimeOut */
				/*
					Cada 1 segundo se va a tratar de Jugar con el proximo Equipo.
					Esto intenta de resolver la desincronizacion entre CIUDAD, EQUIPO y CDE,
					de forma tal que no quedarnos esperando en el Select de manera infinita.
					Notar que solo entra aca si el Equipo esta ENTRENANDO y DESCANSANDO (entre otros)
				*/
				ptrTimeOut = &strTimeOut;
			}
			else
			{
				/*
					Si el puntero NO esta en NULL => Ya hay un TimeOut en espera..
				*/

				/* Lo dejamos como esta para que continue descontando desde donde estaba */
			}
		}


		/* SELECT */
		iValRet = select( iMaximo+1, &BolsaDescriptores, NULL,NULL, ptrTimeOut );


		/* Mensaje recibido desde algun FD de la bolsa */
		if ( iValRet > 0 )
		{

			/* ###################### SE CONECTA UN EQUIPO VISITANTE :: BEGIN ###################### */

			if (FD_ISSET(iFdEscucha,&BolsaDescriptores))
			{
				/* Un equipo nos hablo por el FD escucha */
				iFdEquipoVisitante = AceptaConexion(iFdEscucha,NULL,NULL);

				if ( iFdEquipoVisitante > 0 )
				{
					WriteLog( INFO, sNombreEquipo, "Equipo (local/visitado) acepta la conexión del equipo que tiene la LEV y juega el partido" );

					/* Actualizar Estado JUGANDO */
					iEstado = JUGANDO;

					/****** Envia cabecera a CIUDAD ******/
					if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:35: Error al actualizar estado" );

					/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
					if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:36: Error al actualizar estado" );
				}
				else
					WriteLog( ERROR, sNombreEquipo, "Error:34: Error al conectar con equipo visitante" );

			}

			/* ###################### SE CONECTA UN EQUIPO VISITANTE :: END ###################### */



			/* #################### HABLO CDE :: BEGIN #################### */

			if ( iFdCDE != -1 && FD_ISSET(iFdCDE,&BolsaDescriptores) )
			{
				/* Termino de entrenar => Actualiza su estado */

				if ( recv( iFdCDE, &sAlPedo, sizeof(int), 0 ) > 0 ) /* Por ahora no lo usamos.. */
				{
					if ( atoi(sAlPedo) == 1 )
					{
						/* Fuimos expulsados */
					}
					else
					{
						/* Expiro nuestro tiempo de estadia en el CDE */
					}

				}

				/* Cierra la conexion con CDE */
				close(iFdCDE);
				iFdCDE = -1;

				/* Solo ponemos cansancio a 0 y estado a inactivo si estamos entrenando o descansando */
				if ( iEstado == ENTRENANDO || iEstado == DESCANSANDO )
				{
					/* Restablece el cansancio */
					iCansancio = 0;

					/* Actualiza el Estado a INACTIVO */
					iEstado = INACTIVO;

					WriteLog( INFO, sNombreEquipo, "Termina de entrenar y actualiza su estado" );

					/****** Envia cabecera a CIUDAD ******/
					EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) );

					/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
					if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:4: Error al enviar actualizacion de estado a Ciudad" );

					/* Busca siguiente equipo para jugar (si lstLevResultados != NULL) */
					lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
				}

				/* Sino, no le damos pelota al mensaje del CDE */

			}

			/* #################### HABLO CDE :: END #################### */



			/* #################### HABLO CIUDAD A MIGRAR :: BEGIN #################### */

			if ( iFdCiudadMigra != -1 && FD_ISSET(iFdCiudadMigra,&BolsaDescriptores) )
			{
				/* Recibe el Nombre de la Ciudad */
				if ( recv( iFdCiudadMigra, &sNombreCiudadMigra, sizeof(sNombreCiudadMigra), 0 ) > 0 )
				{
					if ( strcmp(sNombreCiudadMigra, sNombreCiudadOrigen) == 0 )
					{
						/* La Ciudad a la cual queremos migrar es nuestra Ciudad Origen */

						/* Por ahora no se usa, pero quiza haya que usarlo.. lo dejo por el momento */
					}

					/* Se envia a la Ciudad que migramos todos los datos actuales del Equipo */

					sprintf( strEquipo.KI, "%d", iKI );
					sprintf( strEquipo.Cansancio, "%d", iCansancio );
					sprintf( strEquipo.DuracionPartidos, "%d", iDuracionPartidos );
					strEquipo.Nuevo = TRUE; /* o FALSE ??? Igual esta mierda no la usamos */
					sprintf( strEquipo.Equipo, "%s", sNombreEquipo );

					/* Se actualiza el string de LevResultados */
					EncriptaLevResultados ( lstLevResultados, &sLevResultados );


					if (
								send( iFdCiudadMigra, &strEquipo, sizeof(strEquipo), 0 ) > 0
								&&
								EnviaCabecera( iFdCiudadMigra, &iIdMensaje, 0, strlen(sLevResultados)+1 ) > 0
								&&
								send( iFdCiudadMigra, sLevResultados, strlen(sLevResultados)+1, 0 ) > 0
								&&
								EnviaCabecera( iFdCiudadMigra, &iIdMensaje, 0, strlen(sNombreCiudadOrigen)+1 ) > 0
								&&
								send( iFdCiudadMigra, sNombreCiudadOrigen, strlen(sNombreCiudadOrigen)+1, 0 ) > 0
								&&
								EnviaCabecera( iFdCiudadMigra, &iIdMensaje, 0, strlen(sIPCiudadOrigen)+1 ) > 0
								&&
								send( iFdCiudadMigra, sIPCiudadOrigen, strlen(sIPCiudadOrigen)+1, 0 )  > 0
								&&
								EnviaCabecera( iFdCiudadMigra, &iIdMensaje, 0, strlen(sPortCiudadOrigen)+1 ) > 0
								&&
								send( iFdCiudadMigra, sPortCiudadOrigen, strlen(sPortCiudadOrigen)+1, 0 ) > 0
						 )
					{

						/* Actualiza el Estado a MIGRADO */
						iEstado = MIGRADO;

						WriteLog( INFO, sNombreEquipo, "El Equipo va a MIGRAR!" );

						/****** Envia cabecera a CIUDAD ******/
						EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) );
						/* Envía a CIUDAD el Estado actualizado (se migro) [no importa si falla] */
						send( iFdCiudad, &iEstado, sizeof(iEstado), 0 );

						/* Espera a que la Ciudad actualice el Estado */
						sleep( 1 );

						/* Cierra la conexion con CIUDAD */
						close(iFdCiudad);
						iFdCiudad = -1;

						/* NOS FUIMOSSSSSSSSSSSSSSSSSS */

						exit(0);
					}
				}
				/* Hubo algun error al conectarse a la Ciudad a migrar */

				close(iFdCiudadMigra);
				iFdCiudadMigra = -1;

				lstLevResultadosAux->info.Resultado = EQUIPO_KILLED;

				/* Escribe en el LOG */
				sprintf( sLog, "Doy como muerto (EQUIPO_KILLED) a \"%s\"", lstLevResultadosAux->info.Equipo );
				WriteLog( INFO, sNombreEquipo, sLog );


				/* Busca siguiente equipo para jugar */
				lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

			}

			/* #################### HABLO CIUDAD A MIGRAR :: END #################### */


			/* #################### HABLO CIUDAD ORIGEN :: BEGIN #################### */

			if ( iFdCDE != -1 && FD_ISSET(iFdCDE,&BolsaDescriptores) )
			{
				/* Termino de entrenar => Actualiza su estado */

				if ( recv( iFdCDE, &sAlPedo, sizeof(int), 0 ) > 0 ) /* Por ahora no lo usamos.. */
				{
					if ( atoi(sAlPedo) == 1 )
					{
						/* Fuimos expulsados */
					}
					else
					{
						/* Expiro nuestro tiempo de estadia en el CDE */
					}

				}

				/* Cierra la conexion con CDE */
				close(iFdCDE);
				iFdCDE = -1;

				/* Solo ponemos cansancio a 0 y estado a inactivo si estamos entrenando o descansando */
				if ( iEstado == ENTRENANDO || iEstado == DESCANSANDO )
				{
					/* Restablece el cansancio */
					iCansancio = 0;

					/* Actualiza el Estado a INACTIVO */
					iEstado = INACTIVO;

					WriteLog( INFO, sNombreEquipo, "Termina de entrenar y actualiza su estado" );

					/****** Envia cabecera a CIUDAD ******/
					EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) );

					/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
					if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:4: Error al enviar actualizacion de estado a Ciudad" );

					/* Busca siguiente equipo para jugar (si lstLevResultados != NULL) */
					lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
				}

				/* Sino, no le damos pelota al mensaje del CDE */

			}

			/* #################### HABLO CIUDAD ORIGEN :: END #################### */



			/* #################### HABLO NUESTRA CIUDAD :: BEGIN #################### */

			if ( FD_ISSET(iFdCiudad,&BolsaDescriptores) )
			{
				/****** Recibe cabecera desde CIUDAD ******/
				if ( RecibeCabecera(iFdCiudad,&CABECERA_CiudadEquipo) > 0 )
				{
					/* Acciones que debe realizar segun TipoMensaje */
					switch ( CABECERA_CiudadEquipo.TipoMensaje )
					{

			/**************************************************************************************/
			/**/	case BE_CDE_MUERTO:	/**** MURIO EL CDE :: BEGIN *****/
			/**************************************************************************************/

						close(iFdCDE);
						iFdCDE = -1;

						iCansancio = 0;

						/*si estaba en el cde,juega el siguiente partido*/
						if ( iEstado == ENTRENANDO || iEstado == DESCANSANDO )
						{

							iEstado = INACTIVO;

							lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

						}

			/**************************************************************************************/
			/**/	break;		/*	MURIO EL CDE :: END			***/
			/**************************************************************************************/

			/**************************************************************************************/
			/**/	case BE_DIRECCION_CIUDAD:	/**** EQUIPO CONTRINCANTE ENCONTRADO :: BEGIN *****/
			/**************************************************************************************/

							/* Recibe desde CIUDAD la ubicacion del equipo contrincante */
							if ( recv( iFdCiudad, &strDireccionCiudad, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
								WriteLog( ERROR, sNombreEquipo, "Error:8: Error al recibir la direccion de la ciudad" );
							else
							{
								if ( strDireccionCiudad.IPCiudad != 0 ) /* El equipo contrincante es EXTRANJERO */
								{
									/* ################## MIGRACION #####################*/

									/*
									printf("\n\n---- EQUIPO %s MIGRA hacia IP:%ld Port:%d\n", sNombreEquipo, strDireccionCiudad.IPCiudad, strDireccionCiudad.PortCiudad);
									*/

									/* Se conecta a la Ciudad */
									iFdCiudadMigra = AbreConexion (strDireccionCiudad.IPCiudad, strDireccionCiudad.PortCiudad);

									if ( iFdCiudadMigra == -1 )
									{
										/* Si hay error en la conexion, da al equipo como muerto en la lista de resultados */

										WriteLog( ERROR, sNombreEquipo, "Error:18: Error al conectarse con Ciudad a migrar" );
										/* Actualiza la lista de resultados con EQUIPO_KILLED */
										lstLevResultadosAux->info.Resultado = EQUIPO_KILLED;

										/* Pide permiso a ciudad para jugar el siguiente partido */
										lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
									}
								}
								else /* El equipo contrincante es LOCAL */
								{
									/* Pide permiso a la Ciudad para jugar (para que se fije el estado) */
									lstLevResultadosAux = JuegaSiguientePartido( JE_JUGAR_CON, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
								}
							}

			/**************************************************************************************/
			/**/	break;		/*				EQUIPO CONTRINCANTE ENCONTRADO :: END			***/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case BE_NO:		/****************** EQUIPO NO ENCONTRADO :: BEGIN *************/
			/**************************************************************************************/

							/* No se encontró al Equipo, paso a buscar al siguiente de mi LEV */
							/*
							PUNTO A ESTIPULAR: Que pasa si no se puede jugar?

							El equipo ya esta jugando, entrenando o migrado en otra ciudad,
							=> entonces hay que encolarlo.
							Si el equipo se cayó, hay que pasar al siguiente.
							*/

							/* Comienza a recorrer la LEV desde el comienzo, comprobando que el partido no se haya jugado ni haya sido encolado (en realidad es lo mismo) */
							if ( recv( iFdCiudad, &sAlPedo, sizeof(int), 0 ) == -1 )
								WriteLog( ERROR, sNombreEquipo, "Error:2: Error al recibir actualizacion de Ciudad" );

							/*sAlPedo: 1, esta encolado
							sAlPedo: 0, murio el equipo*/

							/*si el equipo esta muerto, lo informo*/
							if ( atoi(sAlPedo) != 1 )
							{
								lstLevResultadosAux->info.Resultado = EQUIPO_KILLED;

								/* Escribe en el LOG */
								sprintf( sLog, "Doy como muerto (EQUIPO_KILLED) a \"%s\"", lstLevResultadosAux->info.Equipo );
								WriteLog( INFO, sNombreEquipo, sLog );

							}
							/* Pide ubicacion de la Ciudad para jugar el siguiente partido */
							lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

			/**************************************************************************************/
			/**/	break;		/*					EQUIPO NO ENCONTRADO :: END					 **/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case CDE_DIRECCION:		/******************************************************/
			/**************************************************************************************/


							/*
								IMPORTANTE: Solo se va a usar el CDE si estamos inactivos actualmente.
														Esto arregla la desincronizacion entre CIUDAD, EQUIPO y CDE.
							*/
							if ( iEstado == INACTIVO )
							{
								/* Actualiza el estado asincronicamente (ENTRENANDO o DESCANSANDO) */
								iEstado = iEstadoAux;

								if ( recv( iFdCiudad, &strDireccionCDE, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
									WriteLog( ERROR, sNombreEquipo, "Error:12: Error al recibir la direccion del CDE" );

								/* Se conecta al CDE */
								iFdCDE = AbreConexion (strDireccionCDE.IP, strDireccionCDE.Port);

								if ( iFdCDE == -1 )
								{
									/* Si no puede conectarse al CDE, informa a ciudad que este murio */
									EnviaCabecera( iFdCiudad, &iIdMensaje, CDE_NO, 0 );

									/* Restablece el cansancio */
									iCansancio = 0;

									/* Actualiza el Estado a INACTIVO */
									iEstado = INACTIVO;

									WriteLog( INFO, sNombreEquipo, "Termina de entrenar y actualiza su estado" );

									/****** Envia cabecera a CIUDAD ******/
									if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
										WriteLog( ERROR, sNombreEquipo, "Error:3: Error al enviar actualizacion de estado a Ciudad" );

									/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
									if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
										WriteLog( ERROR, sNombreEquipo, "Error:4: Error al enviar actualizacion de estado a Ciudad" );

									/* Busca siguiente equipo para jugar */
									lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
								}
								else
								{
									/* Envia la informacion del equipo al CDE */
									strEquipoCDE.TiempoPedido = GetRand( Cfg.TiempoCdeMIN, Cfg.TiempoCdeMAX );
									strcpy(strEquipoCDE.Equipo, sNombreEquipo);
									strEquipoCDE.Estado = iEstado;

									if ( send( iFdCDE, &strEquipoCDE, sizeof(strEquipoCDE), 0 ) < 1 )
									{
										/****** Envia cabecera a CIUDAD ******/
										EnviaCabecera( iFdCiudad, &iIdMensaje, CDE_NO, 0 );

										/* Restablece el cansancio */
										iCansancio = 0;

										/* Actualiza el Estado a INACTIVO */
										iEstado = INACTIVO;

										WriteLog( INFO, sNombreEquipo, "No hay CDE para entrenar" );

										/****** Envia cabecera a CIUDAD ******/
										if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
											WriteLog( ERROR, sNombreEquipo, "Error:3: Error al enviar actualizacion de estado a Ciudad" );

										/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
										if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
											WriteLog( ERROR, sNombreEquipo, "Error:4: Error al enviar actualizacion de estado a Ciudad" );

										/* Cierra la conexion con CDE */
										close(iFdCDE);
										iFdCDE = -1;

										/* Busca siguiente equipo para jugar */
										lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
									}
									else /* Todo fue bien con el CDE */
									{
										/****** Envia cabecera a CIUDAD ******/
										if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
											WriteLog( ERROR, sNombreEquipo, "Error:15: Error al enviar actualizacion del estado a Ciudad" );

										/* Envía a CIUDAD el Estado actualizado */
										if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
											WriteLog( ERROR, sNombreEquipo, "Error:16: Error al enviar actualizacion del estado a Ciudad" );
									}

								}
							}
							else /* Esto arregla el problema de desincronizacion entre CIUDAD, EQUIPO y CDE */
							{
								/* Actualiza el Estado a INACTIVO */
								iEstado = INACTIVO;

								WriteLog( INFO, sNombreEquipo, "Termina de entrenar y actualiza su estado" );

								/****** Envia cabecera a CIUDAD ******/
								if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
									WriteLog( ERROR, sNombreEquipo, "Error:3: Error al enviar actualizacion de estado a Ciudad" );

								/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
								if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
									WriteLog( ERROR, sNombreEquipo, "Error:4: Error al enviar actualizacion de estado a Ciudad" );

								/* Busca siguiente equipo para jugar */
								lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
							}

			/**************************************************************************************/
			/**/	break;		/*																 **/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case CDE_NO:	/**************************************************************/
			/**************************************************************************************/

							/* Restablece el cansancio */
							iCansancio = 0;

							/* Busca siguiente equipo para jugar */
							lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

			/**************************************************************************************/
			/**/	break;																		/**/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case JE_DIRECCION_EQUIPO:	/**************************************************/
			/**************************************************************************************/

							if ( recv( iFdCiudad, &strDireccionEquipo, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
								WriteLog( ERROR, sNombreEquipo, "Error:17: Error al enviar dirección del equipo" );

							/* Se conecta al Equipo */
							iFdEquipoVisitado = AbreConexion (strDireccionEquipo.IP, strDireccionEquipo.Port);

							if ( iFdEquipoVisitado > 0 )
							{
								printf("Equipo \"%s\" enfrenta \"%s\"\n", sNombreEquipo, lstLevResultadosAux->info.Equipo);

								/* Escribe en el LOG */
								sprintf( sLog, "Equipo \"%s\" enfrenta \"%s\"", sNombreEquipo, lstLevResultadosAux->info.Equipo );
								WriteLog( INFO, sNombreEquipo, sLog );

								/* Actualizar Estado JUGANDO */
								iEstado = JUGANDO;
								/****** Envia cabecera a CIUDAD ******/
								EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) );
								/* Envía a CIUDAD el Estado actualizado (empezó a jugar) */
								send( iFdCiudad, &iEstado, sizeof(iEstado), 0 );
							}
							else
							{
								/* Si hay error en la conexion, da al equipo como muerto en la lista de resultados */

								WriteLog( ERROR, sNombreEquipo, "Error:18: Error al conectarse con equipo" );
								/* Actualiza la lista de resultados con EQUIPO_KILLED */
								lstLevResultadosAux->info.Resultado = EQUIPO_KILLED;

								/* Pide permiso a ciudad para jugar el siguiente partido */
								lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

							}

			/**************************************************************************************/
			/**/	break;		/*																 **/
			/**************************************************************************************/

			/**************************************************************************************/
			/**/	case JE_NO:		/********** NO SE PUEDE JUGAR CON EQUIPO :: BEGIN *************/
			/**************************************************************************************/

							/* No se encontró al Equipo, paso a buscar al siguiente de mi LEV */
							/*
							PUNTO A ESTIPULAR: Que pasa si no se puede jugar?

							El equipo ya esta jugando, entrenando o migrado en otra ciudad,
							=> entonces hay que encolarlo.
							Si el equipo se cayó, hay que pasar al siguiente.
							*/
							/* Comienza a recorrer la LEV desde el comienzo, comprobando que el partido no se haya jugado ni haya sido encolado (en realidad es lo mismo) */
							if ( recv( iFdCiudad, &sAlPedo, sizeof(int), 0 ) == -1 )
								WriteLog( ERROR, sNombreEquipo, "-562- Recibir motivo por el cual no se encontro el equipo" );

							/*sAlPedo: 1, esta encolado
							sAlPedo: 0, murio el equipo*/

							/*si el equipo esta muerto, lo informo*/
							if ( atoi(sAlPedo) != 1 )
							{
								lstLevResultadosAux->info.Resultado = EQUIPO_KILLED;

								/* Escribe en el LOG */
								sprintf( sLog, "Doy como muerto (EQUIPO_KILLED) a  \"%s\"", lstLevResultadosAux->info.Equipo );
								WriteLog( INFO, sNombreEquipo, sLog );

							}

							/*
								Esta espera es para reducir la frecuencia con la que el Equipo le pregunta
								a la Ciudad para jugar con un contrincante cuando este aun no esta disponible.
								Asi se logra reducir la carga del sistema en general,
								evitando ciclos innecesarios de mensajes entre Ciudad y Equipo
							*/
							sleep( 1 );

							/* Pide permiso a ciudad para jugar el siguiente partido */
							lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

			/**************************************************************************************/
			/**/	break;		/*				NO SE PUEDE JUGAR CON EQUIPO :: END				 **/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case LEV_ENVIAR:	/***********	RECEPCION DE LEV :: BEGIN	***************/
			/**************************************************************************************/

							if ( sLev != NULL ) /* Elimina LEV vieja */
							{

								/*sLev = NULL;*/
								free(sLev);

								/*printf("\n\n---- EQUIPO (%s): LLEGO LEV EXTERNA!!\n\n", sNombreEquipo);*/

								/* Como para "desincronizar" un poco a los Equipos */
								sleep( GetRand(5,10) );
							}

							sLev = malloc( CABECERA_CiudadEquipo.LargoMensaje );
							/******* Recibe LEV desde CIUDAD *********/
							if ( recv( iFdCiudad, sLev, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
								WriteLog( ERROR, sNombreEquipo, "Error:27: Error al recibir LEV" );

							WriteLog( INFO, sNombreEquipo, "Recibe la LEV" );

							/* Convierte la LEV: cadena => lista */
							DesencriptaLev( &lstLevResultados, sLev );

							/* Pide la ubicacion de la Ciudad en donde se encuentra el Equipo a enfrentar */
							lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

			/**************************************************************************************/
			/**/	break;		/*				RECEPCION DE LEV :: END							 **/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case LEV_RESULTADOS:	/***********	RECEPCION DE LEV RESULTADOS :: BEGIN	***************/
			/**************************************************************************************/

							/*
								Esto solamente se ejecuta cuando el Equipo recibe la LEV Resultados
								de la Ciudad a la cual ha migrado, para que continuemos jugando
							*/

							sLevResultados = malloc( CABECERA_CiudadEquipo.LargoMensaje );

							/******* Recibe LEV Resultados desde CIUDAD *********/
							if ( recv( iFdCiudad, sLevResultados, CABECERA_CiudadEquipo.LargoMensaje, 0 ) == -1 )
								WriteLog( ERROR, sNombreEquipo, "Error:27: Error al recibir LEV" );

							WriteLog( INFO, sNombreEquipo, "Recibe la LEV Resultados de la Ciudad a la que migramos" );

							/* Convierte la LEV: cadena => lista */
							DesencriptaLevResultados( &lstLevResultados, sLevResultados );

							/* Pide la ubicacion de la Ciudad en donde se encuentra el Equipo a enfrentar */
							lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );

			/**************************************************************************************/
			/**/	break;		/*				RECEPCION DE LEV RESULTADOS :: END							 **/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case CME_CONFIRMACION:	/******************************************************/
			/**************************************************************************************/

							/* Migracion de Equipo */

			/**************************************************************************************/
			/**/	break;		/*																 **/
			/**************************************************************************************/


			/**************************************************************************************/
			/**/	case AE_CONFIRMACION:	/******************************************************/
			/**************************************************************************************/

			/**************************************************************************************/
			/**/	break;		/*																 **/
			/**************************************************************************************/

					}

				}
				else
				{
					WriteLog( ERROR, sNombreEquipo, "MURIO LA CIUDAD -- Autodestruyendo este Equipo.." );
					exit(0);
				}

			}

			/* #################### HABLO NUESTRA CIUDAD :: END #################### */



			/* ###################### EQUIPO LOCAL-VISITADO :: BEGIN ###################### */

			if (iFdEquipoVisitante != -1 &&  FD_ISSET(iFdEquipoVisitante,&BolsaDescriptores))
			{
				/* Recibe los resultados del partido */
				if ( recv(iFdEquipoVisitante, &iResultadoPartido , sizeof(iResultadoPartido), 0) < 1 )
				{
					close ( iFdEquipoVisitante );

					WriteLog( ERROR, sNombreEquipo, "Error:30: Error al recibir resultados (equipo local)" );

					/* Actualiza su estado */
					iEstado = INACTIVO;

					/****** Envia cabecera a CIUDAD ******/
					if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
 						WriteLog( ERROR, sNombreEquipo, "Error:31: Error con conexion a ciudad" );

					/* Envía a CIUDAD el Estado actualizado (termino de jugar) */
					if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:32: Error con conexion a ciudad" );


				}

				/* Actualiza el KI del equipo */
				if ( iResultadoPartido == DERROTA ) {
					iKI++;
				}
				else if ( iResultadoPartido == VICTORIA ) {
					iKI--;
				}

				WriteLog( INFO, sNombreEquipo, "Equipo (local/visitado) conoce el resultado de su partido" );

				/* Genera cansancio del equipo */
				iCansancio += GetRand( 1, 5 );

				/* Cierra la conexion con el equipo VISITANTE */
				close(iFdEquipoVisitante);
				iFdEquipoVisitante = -1;


				/* Actualiza su estado */
				iEstado = INACTIVO;

				/****** Envia cabecera a CIUDAD ******/
				if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
					WriteLog( ERROR, sNombreEquipo, "Error:31: Error con conexion a ciudad" );

				/* Envía a CIUDAD el Estado actualizado (termino de jugar) */
				if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
					WriteLog( ERROR, sNombreEquipo, "Error:32: Error con conexion a ciudad" );


				/* Termina de jugar y chequea su cansancio */
				if ( iCansancio >= Cfg.CansancioMAX )
				{
					WriteLog( INFO, sNombreEquipo, "Equipo (local/visitado) pide CDE" );

					/* Pide CDE por CANSANCIO MAXIMO alcanzado */
					iEstadoAux = DESCANSANDO;

					if ( EnviaCabecera( iFdCiudad, &iIdMensaje, CDE_PEDIR, 0 ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:33: Error al pedir CDE" );

				}

			}

			/* ###################### EQUIPO LOCAL-VISITADO :: END ###################### */


		} /* fin select sin errores */


		/* ###################### EQUIPO QUE TIENE LA LEV (visitante) :: BEGIN ###################### */

		else if ( iValRet == 0 ) /* Se produjo un TimeOut en el Select */
		{
			if ( iEstado == INACTIVO )
			{
				iEstadoAux = ENTRENANDO;

				/*************** CDE POR INACTIVIDAD ******************/
				if ( EnviaCabecera( iFdCiudad, &iIdMensaje, CDE_PEDIR, 0 ) == -1 )
					WriteLog( ERROR, sNombreEquipo, "Error:37: Error al pedir CDE" );


				WriteLog( INFO, sNombreEquipo, "Equipo va a entrenar por inactividad" );

			}
			else if ( iEstado == JUGANDO )
			{
				/*************** TERMINA PARTIDO ******************/

				/* SE "JUEGA" EL PARTIDO => Genera resultado random del partido */

				/*FH - hace un truco, jeje*/
				if ( ( ( !strcmp( "Boca", sNombreEquipo ) || !strcmp( "Boca Juniors", sNombreEquipo ) || !strcmp( "Boca Jrs", sNombreEquipo  ) ) && !strcmp("San Lorenzo", lstLevResultadosAux->info.Equipo) ) ||  ( ( !strcmp( "Boca", lstLevResultadosAux->info.Equipo ) || !strcmp( "Boca Juniors", lstLevResultadosAux->info.Equipo ) || !strcmp( "Boca Jrs", lstLevResultadosAux->info.Equipo  ) ) && !strcmp("San Lorenzo", sNombreEquipo) ))
				{
					if ( !(!strcmp( "Boca", sNombreEquipo ) || !strcmp( "Boca Juniors", sNombreEquipo ) || !strcmp( "Boca Jrs", sNombreEquipo  )))
						iResultadoPartido = VICTORIA;
					else
						iResultadoPartido = DERROTA;
				}
				else
				{
					iResultadoPartido = GetRand( 0, 2 );
				}

				/* Actualiza el KI del equipo */
				if ( iResultadoPartido == VICTORIA )
				{
					iKI++;
				}
				else if ( iResultadoPartido == DERROTA )
				{
					iKI--;
				}

				WriteLog( INFO, sNombreEquipo, "Equipo termina de jugar el partido" );

				/* Genera cansancio del equipo */
				iCansancio += GetRand( 1, 5 );

				/* Envia al equipo VISITADO el resultado del partido */
				if ( send(iFdEquipoVisitado, &iResultadoPartido , sizeof(iResultadoPartido), 0) == -1 )
					WriteLog( ERROR, sNombreEquipo, "Error:38: Error al enviar al equipo local el resultado del partido" );

				/* Cierra la conexion con el equipo VISITADO */
				close(iFdEquipoVisitado);

				iFdEquipoVisitado = -1;

				lstLevResultadosAux->info.Resultado = iResultadoPartido;

				/* Actualiza su estado */
				iEstado = INACTIVO;

				/****** Envia cabecera a CIUDAD ******/
				if ( EnviaCabecera( iFdCiudad, &iIdMensaje, AE_ESTADO, sizeof(iEstado) ) == -1 )
					WriteLog( ERROR, sNombreEquipo, "Error:39: Error al actualizar estado" );

				/* Envía a CIUDAD el Estado actualizado (termino de jugar) */
				if ( send( iFdCiudad, &iEstado, sizeof(iEstado), 0 ) == -1 )
					WriteLog( ERROR, sNombreEquipo, "Error:40: Error al actualizar estado" );


				/* Termina de jugar y chequea su cansancio */
				if ( iCansancio >= Cfg.CansancioMAX )
				{
					/* Pide CDE por CANSANCIO MAXIMO alcanzado */
					iEstadoAux = DESCANSANDO;

					WriteLog( INFO, sNombreEquipo, "Equipo va a descansar por cansancio maximo alcanzado" );

					if ( EnviaCabecera( iFdCiudad, &iIdMensaje, CDE_PEDIR, 0 ) == -1 )
						WriteLog( ERROR, sNombreEquipo, "Error:41: Error al pedir CDE" );
				}
				else
				{
					/* Pide permiso a ciudad para jugar el siguiente partido */
					lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );
				}
			}
			else
			{
				/*
					Para salvar de la desincronizacion entre CIUDAD, EQUIPO y CDE
				*/

				/* Busca siguiente equipo para jugar */
				/*lstLevResultadosAux = JuegaSiguientePartido( BE_BUSCAR, &lstLevResultados, sNombreEquipo, iFdCiudad, &iIdMensaje, iMigrado, sIPCiudadOrigen, sPortCiudadOrigen, &iFdCiudadMigra );*/
			}

			/* ###################### EQUIPO QUE TIENE LA LEV (visitante) :: END ###################### */



		}
		else if (iValRet == -1)
		{
			printf("\nERROR: Error de interrupciones");
			WriteLog( ERROR, sNombreEquipo, "Error:46: Error de interrupciones" );
		}

	}

	return 0;
}
