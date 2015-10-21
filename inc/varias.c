/*
	Ultima modificacion: 21/06 - 02:30
	Modificado por: Christian Pandullo
*/


/*
	Establece el color para la salida por pantalla
*/
void setcolor (int attr, int fg, int bg)
{
	char ControlCmd[13]; /* Comando de control enviado a la terminal */

	sprintf( ControlCmd, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40 );
	printf( "%s", ControlCmd );
}

void muerte_hijo()
{
/*	signal(SIGCHLD,SIG_IGN);

	while(waitpid(0,NULL,WNOHANG)>0);

	signal(SIGCHLD,muerte_hijo);
*/
	return;
}

/* KILL'EM ALL */
void ctrl_c()
{
	/* Se reestablece la pantalla */
	/*setcolor(TXT_RESET, TXT_WHITE, TXT_BLACK);*/

	signal (SIGINT,SIG_IGN);

	kill( 0, SIGKILL );
	signal ( SIGINT,ctrl_c );

	exit(0);

}

/*
	Recibe los comados escritos por linea de comandos.
	Retorna un identificador de comando.
*/
int Comandos ( char *linea )
{
	if ( strcmp(linea,"exit") == 0 ) { /* Salir del programa */
		return EXIT;
	}
	else if ( strcmp(linea,"help") == 0 ) { /* Mostrar ayuda en pantalla */
		return HELP;
	}
	else if ( strcmp(linea,"addcity") == 0 ) { /* Agregar nueva ciudad */
		return ADDCITY;
	}
	else if ( strcmp(linea,"addteam") == 0 ) { /* Agregar nuevo equipo */
		return ADDTEAM;
	}
	else if ( strcmp(linea,"startgame") == 0 ) { /* Comenzar simulacion de juego */
		return STARTGAME;
	}
	else if ( strcmp(linea,"endgame") == 0 ) { /* Finalizar simulacion de juego */
		return ENDGAME;
	}
	else if ( strcmp(linea,"new") == 0 ) { /* Crear Token */
		return NEW;
	}
	else if ( strcmp(linea,"join") == 0 ) { /* Conectarse con otro Router */
		return JOIN;
	}
	else { /* ERROR: Comando no reconocido */
		return FAIL;
	}
}

/*
	Muestra la ayuda del programa en pantalla
*/
void Comandos_Help ( void )
{
	printf("\n--- LISTADO DE COMANDOS ---\n\n\n");
	printf("new : Crea el juego\n");
	printf("join : Se conecta a un juego existente\n");
	printf("addcity : Crea la ciudad\n");
	printf("addteam : Agrega un nuevo equipo\n");
	printf("startgame : Comienza la simulacion de la liga\n");
	printf("endgame : Finaliza la simulacion de la liga\n");
	printf("\nexit : Finaliza la ejecucion del programa\n\n");

	return;
}

/*
	Convierte una cadena a MAYUSCULAS
*/
char * strtoupper ( char *str )
{
	char *res = str;

	while (*str)
	{
		*str = toupper(*str);
		str++;
	}

	return res;
}

/*
	Realiza un socketpair
*/
int GetFD_socketpair ( int *iFdCero, char *iFdUno )
{
	int iFdSocketPair[2];

	/* Realiza la llamada a socketpair() */
	if ( socketpair(AF_UNIX,SOCK_STREAM,0,iFdSocketPair) == -1 ) {
		return FAIL; /* ERROR: No se pudieron generar los FD */
	}

	/* Guarda el FD 0 */
	*iFdCero = iFdSocketPair[0];
	/* Guarda el FD 1 */
	sprintf( iFdUno, "%d", iFdSocketPair[1]);

	return OK;
}

/*
	Elimina el \n de una cadena reemplazandolo por un \0
*/
void Strip_newline ( char *string )
{
	string[ strlen(string)-1 ] = '\0';

	return;
}

/***************************************************************************
****************************************************************************
*					FUNCIONES DE LISTAS DINAMICAS				           *
****************************************************************************
****************************************************************************/

/* Inserta un equipo a la lista (LEV) */
LEV * InsertaNodoLev ( LEV *lista, LEVinfo info )
{
	/*LEVptr pNew;*/ /* Estrategia 1 */
	/*LEVptr pNew, pAux;*/ /* Estrategia 2 */

	/* Estrategia 3 */
	LEV *pNew, *pAux, *pAnt;

	pNew = malloc( sizeof(LEV) );

	/*pNew = (struct LEV *) malloc ( sizeof (LEV) );*/

	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(LEVinfo) );

		/*
			Estrategia 1: Guardar la lista como una "pila",
										en donde el puntero "lista" apunta siempre al nodo mas nuevo.
		*/
		/*
		pNew->sgte = lista;
		lista = pNew;
		*/

		/*
			Estrategia 2: Guardar la lista como una "cola",
										en donde el puntero "lista" apunta siempre al nodo mas viejo,
										el primero de todos los nodos creados...
		*/
		/*
		pNew->sgte = NULL;
		pAux = lista;

		while ( pAux != NULL && pAux->sgte != NULL ) {
			pAux = pAux->sgte;
		}

		if ( pAux == NULL ) {
			lista = pNew;
		}
		else {
			pAux->sgte = pNew;
		}
		*/

		/*
			Estrategia 3: Guardar la lista ordenada por nombre de equipo.
		*/
		pNew->sgte = NULL;
		pAux = lista;
		pAnt = lista;

		while ( pAux != NULL && strcmp(info.Equipo,pAux->info.Equipo) > 0 ) {
			pAnt = pAux;
			pAux = (LEV *)pAux->sgte;
		}

		if ( pAux == pAnt ) {
			lista = pNew;
		}
		else {
			pAnt->sgte = (struct LEV *)pNew;
		}

		pNew->sgte = (struct LEV *)pAux;

	}

	return lista;
}

/****************************************************************************/
/* Borra un equipo de la lista */
LEV * SuprimeNodoLEV ( LEV *lista, char sNombreEquipo[] )
{
	LEV *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && strcmp(pAux->info.Equipo,sNombreEquipo) != 0 ) {
		pAnt = pAux;
		pAux = (LEV *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return lista;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (LEV *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}

/* Inserta un equipo a la lista (FDS) ordenado por PUNTAJE */
FDS * InsertaNodoFDS ( FDS *lista, FDSinfo info )
{
	FDS *pNew, *pAux, *pAnt;

	pNew = malloc( sizeof(FDS) );

	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(FDSinfo) );

		pNew->sgte = NULL;
		pAux = lista;
		pAnt = lista;

		while ( pAux != NULL && info.Puntaje < pAux->info.Puntaje ) {
			/* || strcmp(info.Equipo,pAux->info.Equipo) > 0 */
			pAnt = pAux;
			pAux = (FDS *)pAux->sgte;
		}

		if ( pAux == pAnt ) {
			lista = pNew;
		}
		else {
			pAnt->sgte = (struct FDS *)pNew;
		}

		pNew->sgte = (struct FDS *)pAux;

	}

	return lista;
}
/****************************************************************************/
/* Ordena lista FDS por Puntaje */
FDS * OrdenaPuntajeFDS ( FDS *lista )
{
	FDS *pAux = lista, *pRetorno = NULL;

	while ( pAux != NULL ) {
		pRetorno = InsertaNodoFDS( pRetorno, pAux->info );
		lista = pAux;
		pAux = (FDS *)pAux->sgte;
		free( lista );
	}

	return pRetorno;
}

/****************************************************************************/
/* Borra un equipo de la lista */
FDS * SuprimeNodoFDS ( FDS *lista, char Equipo[] )
{
	FDS *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && strcmp(pAux->info.Equipo, Equipo) != 0 ) {
		pAnt = pAux;
		pAux = (FDS *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (FDS *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}
/****************************************************************************/
/* Inserta un equipo a la lista (LEVRESULTADOS) ordenado por NOMBRE de equipo */
LEVRESULTADOS * InsertaNodoLevResultado ( LEVRESULTADOS *lista, LEVRESULTADOSinfo info )
{
	LEVRESULTADOS *pNew, *pAux, *pAnt;

	pNew = malloc( sizeof(LEVRESULTADOS) );

	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(LEVRESULTADOSinfo) );

		pNew->sgte = NULL;
		pAux = lista;
		pAnt = lista;

		while ( pAux != NULL && strcmp(info.Equipo,pAux->info.Equipo) > 0 ) {
			pAnt = pAux;
			pAux = (LEVRESULTADOS *)pAux->sgte;
		}

		if ( pAux == pAnt ) {
			lista = pNew;
		}
		else {
			pAnt->sgte = (struct LEVRESULTADOS *)pNew;
		}

		pNew->sgte = (struct LEVRESULTADOS *)pAux;

	}

	return lista;
}

/****************************************************************************/
/* Vacia la LEV */
int LiberaLevResultados ( LEVRESULTADOS **lista )
{
	/*LEVRESULTADOS *pAux = *lista;

	while ( pAux != NULL )
	{
		*lista = (LEVRESULTADOS *) pAux->sgte;
		free( pAux );
		pAux = *lista;
	}


	return OK;*/
	*lista = NULL;

	return OK;
}

/****************************************************************************/

/* Inserta un equipo a la lista (CDE) */
CDEEQUIPOS * InsertaNodoCDE ( CDEEQUIPOS *lista, CDEEQUIPOSinfo info )
{
	CDEEQUIPOS *pNew;

	pNew = malloc( sizeof(CDEEQUIPOS) );

	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(CDEEQUIPOSinfo) );

		pNew->sgte = (struct CDEEQUIPOS *)lista;
		lista = pNew;
	}

	return lista;
}

/* Borra un equipo de la lista */
CDEEQUIPOS * SuprimeNodoCDE ( CDEEQUIPOS *lista, CDEEQUIPOS *pDel )
{
	CDEEQUIPOS *pAux, *pAnt;


	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && pAux != pDel ) {
		pAnt = pAux;
		pAux = (CDEEQUIPOS *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (CDEEQUIPOS *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );


	return lista;
}
/****************************************************************************/
/* Inserta un equipo a la COLA (CDE)

Cola falsa: ordenada por algoritmo SPN (TiempoPedido)
*/
CDECOLAEQUIPOS * AgregaNodoCDE ( CDECOLAEQUIPOS *lista, CDECOLAEQUIPOSinfo info )
{
	CDECOLAEQUIPOS *pNew, *pAux, *pAnt;

	pNew = malloc( sizeof(CDECOLAEQUIPOS) );


	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(CDECOLAEQUIPOSinfo) );

		pNew->sgte = NULL;
		pAux = lista;
		pAnt = lista;

		while ( pAux != NULL && info.TiempoPedido >= pAux->info.TiempoPedido ) {

			pAnt = pAux;
			pAux = (CDECOLAEQUIPOS *)pAux->sgte;
		}

		if ( pAux == pAnt ) {
			lista = pNew;
		}
		else {
			pAnt->sgte = (struct CDECOLAEQUIPOS *)pNew;
		}

		pNew->sgte = (struct CDECOLAEQUIPOS *)pAux;

	}

	return lista;
}



CDECOLAEQUIPOS * EliminaNodoCDE ( CDECOLAEQUIPOS *lista, CDECOLAEQUIPOSinfo *info )
{
	CDECOLAEQUIPOS *pAux;

	pAux = lista;

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return NULL;
	}

	/* Guarda los datos del nodo que va a ser eliminado */
	memcpy( info, &(lista->info), sizeof(CDECOLAEQUIPOSinfo) );


	lista = (CDECOLAEQUIPOS *)lista->sgte;

	free(pAux);

	return lista;
}


/* Borra un equipo de la lista (cola)
Solo es usada para cuando se quiere seleccionar uno en especial y borrar (en caso de mensaje de Ciudad)
*/
CDECOLAEQUIPOS * SuprimeNodoColaCDE ( CDECOLAEQUIPOS *lista, CDECOLAEQUIPOS *pDel )
{
	CDECOLAEQUIPOS *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && pAux != pDel ) {
		pAnt = pAux;
		pAux = (CDECOLAEQUIPOS *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (CDECOLAEQUIPOS *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}


/* Equipo|Equipo|Equipo\0  */
/* Encripta Lev en cadena */
void EncriptaLev (LEV *lista, char **sLev)
{
	int iAcum = 0, iCont = 0;
	LEV *listaAux = lista;

	while ( listaAux != NULL) {

		iAcum += strlen(listaAux->info.Equipo) + 2 + strlen(listaAux->info.CiudadOrigen);

		listaAux = (LEV *)listaAux->sgte;
	}

	*sLev = malloc(iAcum);

	listaAux = lista;
	while ( listaAux != NULL)
	{

		memcpy (&((*sLev)[iCont]), listaAux->info.Equipo, strlen(listaAux->info.Equipo));

		iCont += strlen(listaAux->info.Equipo);
		(*sLev)[iCont] = '*';
		iCont++;


		memcpy (&((*sLev)[iCont]), listaAux->info.CiudadOrigen, strlen(listaAux->info.CiudadOrigen));

		iCont += strlen(listaAux->info.CiudadOrigen);
		(*sLev)[iCont] = '|';
		iCont++;



		listaAux = (LEV *)listaAux->sgte;
	}

	(*sLev)[iCont-1] = '\0';

}
/****************************************************************************/

/* ResultadoEquipo|ResultadoEquipo\0  */
/* Encripta Lev en cadena */
void EncriptaLevResultados (LEVRESULTADOS *lista, char **sLev)
{
	int iAcum = 0, iCont = 0;
	char *tmp = malloc( 2 );
	LEVRESULTADOS *listaAux = lista;

	while ( listaAux != NULL) {

		/*iAcum += sizeof(listaAux->info.Resultado) + strlen(listaAux->info.Equipo) + strlen("|");*/
		iAcum += 1 + strlen(listaAux->info.Equipo) + strlen("*") + strlen(listaAux->info.CiudadOrigen) + strlen("|");

		listaAux = (LEVRESULTADOS *)listaAux->sgte;
	}

	*sLev = malloc(iAcum);

	listaAux = lista;
	while ( listaAux != NULL) {

		sprintf( tmp, "%d", listaAux->info.Resultado );

		/*memcpy (&((*sLev)[iCont]), &(listaAux->info.Resultado), sizeof(listaAux->info.Resultado));*/
		memcpy (&((*sLev)[iCont]), tmp, strlen(tmp));

		/*iCont += sizeof(listaAux->info.Resultado);*/
		iCont += strlen(tmp);

		memcpy (&((*sLev)[iCont]), listaAux->info.Equipo, strlen(listaAux->info.Equipo));

		iCont += strlen(listaAux->info.Equipo);


		(*sLev)[iCont] = '*';
		iCont++;


		memcpy (&((*sLev)[iCont]), listaAux->info.CiudadOrigen, strlen(listaAux->info.CiudadOrigen));

		iCont += strlen(listaAux->info.CiudadOrigen);


		(*sLev)[iCont] = '|';
		iCont++;



		listaAux = (LEVRESULTADOS *)listaAux->sgte;
	}

	if ( iCont == 0 )
		iCont = 1;

	(*sLev)[iCont-1] = '\0';

}
/****************************************************************************/

/* Equipo|Equipo|Equipo\0 a LISTA */
/* Desencripta cadena a Lista */
void DesencriptaLev (LEVRESULTADOS **lista, char *sLev)
{
	char *sToken;
	LEVRESULTADOSinfo info;

	LEVRESULTADOS *pAux = *lista;

	while ( pAux != NULL )
	{
		*lista = (LEVRESULTADOS *) pAux->sgte;
		free( pAux );
		pAux = *lista;
	}

	sToken = strtok(sLev,"*\0");
	while(sToken != NULL) {

		info.Resultado = NO_JUGADO;
		sprintf (info.Equipo, "%s", sToken);

		sToken = strtok(NULL,"|\0");
		sprintf (info.CiudadOrigen, "%s", sToken);

		*lista = InsertaNodoLevResultado(*lista, info);

		sToken = strtok(NULL,"*\0");
	}
}
/****************************************************************************/
/* ResultadoEquipo|ResultadoEquipo|ResultadoEquipo\0 a LISTA */
/* Desencripta cadena a Lista */
void DesencriptaLevResultados (LEVRESULTADOS **lista, char *sLev)
{
	char *sToken, sAux[2];
	LEVRESULTADOSinfo info;
	char sLog[1000];
	char *sLevAuxi;

	LEVRESULTADOS *pAux = *lista;

	sLevAuxi = malloc( strlen(sLev) + 1 );

	sprintf( sLog, "LEV: \"%s\"", sLev );
	WriteLog( INFO, "CIUDAD LOCA", sLog );

	while ( pAux != NULL )
	{
		*lista = (LEVRESULTADOS *) pAux->sgte;
		free( pAux );
		pAux = *lista;
	}

	strcpy(sLevAuxi, sLev);

	sToken = strtok(sLevAuxi,"*\0");

	while(sToken != NULL) {

		sAux[0] = sToken[0];
		sAux[1] = '\0';

		info.Resultado = atoi(sAux);
		sprintf (info.Equipo, "%s", &(sToken[1]));

		sToken = strtok(NULL,"|\0");
		sprintf (info.CiudadOrigen, "%s", sToken);

		*lista = InsertaNodoLevResultado(*lista, info);

		sToken = strtok(NULL,"*\0");


	}

	free(sLevAuxi);
}
/****************************************************************************/


/*
	Obtiene un numero random entre min y max
*/
int GetRand ( int min, int max )
{
	int mod = max - min + 1;

	srand ( time(NULL) + rand() );

	return rand() % mod + min;
}

/*
	Arma y envia una cabecera
*/
int EnviaCabecera ( int FdDestino, int *idMensaje, char TipoMensaje, int LargoMensaje )
{
	CABECERA_CIUDADEQUIPO CABECERA_CiudadEquipo;
	int sent;

	/* Arma la cabecera */
	CABECERA_CiudadEquipo.IdMensaje = getpid()*10000 + *idMensaje++;
	CABECERA_CiudadEquipo.TipoMensaje = TipoMensaje;
	CABECERA_CiudadEquipo.LargoMensaje = LargoMensaje;

	/* Envia la cabecera */
	sent = send( FdDestino, &CABECERA_CiudadEquipo, sizeof(CABECERA_CIUDADEQUIPO), 0 );

	return sent; /* Retorna la cantidad de bytes enviados */
}

/*
	Recibe una cabecera
*/
int RecibeCabecera ( int FdFuente, CABECERA_CIUDADEQUIPO *CABECERA_CiudadEquipo )
{
	int received;

	/* Recibe la cabecera */
	received = recv( FdFuente, CABECERA_CiudadEquipo, sizeof(CABECERA_CIUDADEQUIPO), 0 );

	return received; /* Retorna la cantidad de bytes recibidos */
}


/*
	Arma y envia una cabecera
*/
/*
int EnviaCabeceraRouter ( int FdDestino, int *idMensaje, char TipoMensaje, int LargoMensaje,
													CABECERA_ROUTERROUTER CABECERA_RouterRouter , int iRouterRouter )
{
	int sent;

	// Arma la cabecera
	if ( TipoMensaje == BE_DISCOVER || TipoMensaje == BE_FOUND ) // No se auto-incrementa el idMensaje
		CABECERA_RouterRouter.IdMensaje = *idMensaje;
	else
		CABECERA_RouterRouter.IdMensaje = getpid()*1000 + *idMensaje++;

	CABECERA_RouterRouter.TipoMensaje = TipoMensaje;
	CABECERA_RouterRouter.LargoMensaje = LargoMensaje;

	if ( iRouterRouter == ROUTER_ROUTER ) // Esta cabecera es para un Router
	{
		// Se actualizan los campos del mensaje
		CABECERA_RouterRouter.TTL -= 1;
		CABECERA_RouterRouter.Hops += 1;
	}


	// Envia la cabecera
	sent = send( FdDestino, &CABECERA_RouterRouter, sizeof(CABECERA_ROUTERROUTER), 0 );

	return sent; // Retorna la cantidad de bytes enviados
}
*/
/*
	Arma cabecera en string (bytes) con memcpy
	Arma y envia una cabecera

	Busqueda: EnviaCabeceraRouterNUEVA
*/
int EnviaCabeceraRouter ( int FdDestino, int *idMensaje, char TipoMensaje, int LargoMensaje,
							   CABECERA_ROUTERROUTER CABECERA_RouterRouter, char **sCabeceraRouter, int iRouterRouter )
{
	int sent, iLargo = 0;
	unsigned short IdMensaje, LargoMensajeAux;

	/* Arma la cabecera */

	if ( TipoMensaje == BE_DISCOVER || TipoMensaje == BE_FOUND ) /* No se auto-incrementa el idMensaje */
		IdMensaje = (unsigned short) *idMensaje;
	else
		IdMensaje = (unsigned short) ( GetRand(1, getpid()) + (*idMensaje)++ );

	/*
			Conversion - ID del mensaje: INT(4b) => SHORT (2b)
	*/

	/* 1) ID del mensaje: 2b */
	memcpy( &((*sCabeceraRouter)[iLargo]), &IdMensaje, sizeof(IdMensaje) );
	iLargo += sizeof(IdMensaje);

	/* 2) Tipo de mensaje: 1b */
	memcpy( &((*sCabeceraRouter)[iLargo]), &TipoMensaje, sizeof(TipoMensaje) );
	iLargo += sizeof(TipoMensaje);

	if ( iRouterRouter == ROUTER_ROUTER ) /* Esta cabecera es para un Router */
	{
		/* Se actualizan los campos del mensaje */
		CABECERA_RouterRouter.TTL -= 1;
		CABECERA_RouterRouter.Hops += 1;
	}

	/* 3) TTL: 1b */
	memcpy( &((*sCabeceraRouter)[iLargo]), &CABECERA_RouterRouter.TTL, sizeof(CABECERA_RouterRouter.TTL) );
	iLargo += sizeof(CABECERA_RouterRouter.TTL);

	/* 4) Hops: 1b */
	memcpy( &((*sCabeceraRouter)[iLargo]), &CABECERA_RouterRouter.Hops, sizeof(CABECERA_RouterRouter.Hops) );
	iLargo += sizeof(CABECERA_RouterRouter.Hops);

	/*
	Conversion - Largo del Mensaje: INT(4b) => SHORT (2b)
	*/

	/* 5) Largo Mensaje: 2b */
	LargoMensajeAux = (unsigned short) LargoMensaje;

	memcpy( &((*sCabeceraRouter)[iLargo]), &LargoMensajeAux, sizeof(LargoMensajeAux) );
	iLargo += sizeof(LargoMensajeAux);


	/* Envia la cabecera (7 b) */
	sent = send( FdDestino, *sCabeceraRouter, iLargo, MSG_DONTWAIT );


	return sent; /* Retorna la cantidad de bytes enviados */
}


/*
	Recive Cabecera Router en string
	LARGO_CABECERA_ROUTER= 7
*/
int RecibeCabeceraRouter ( int FdFuente, char **sCabeceraRouter, CABECERA_ROUTERROUTER *CABECERA_RouterRouter )
{
	int received, iLargo = 0;
	unsigned short IdMensaje, LargoMensajeAux;


	/* Recibe la cabecera */
	received = recv( FdFuente, *sCabeceraRouter, LARGO_CABECERA_ROUTER, 0 );

	/* La guarda en la estructura */


	/*
			Conversion - ID del mensaje: SHORT (2b) => INT(4b)
	*/

	/* 1) ID del mensaje: 2b */
	/*memcpy( &(CABECERA_RouterRouter->IdMensaje), &((*sCabeceraRouter)[iLargo]), sizeof(CABECERA_RouterRouter->IdMensaje) );
	iLargo += sizeof(CABECERA_RouterRouter->IdMensaje);*/
	memcpy( &IdMensaje, &((*sCabeceraRouter)[iLargo]), sizeof(IdMensaje) );
	iLargo += sizeof(IdMensaje);

	CABECERA_RouterRouter->IdMensaje = (int) IdMensaje;

	/* 2) Tipo de mensaje: 1b */
	memcpy( &(CABECERA_RouterRouter->TipoMensaje), &((*sCabeceraRouter)[iLargo]), sizeof(CABECERA_RouterRouter->TipoMensaje) );
	iLargo += sizeof(CABECERA_RouterRouter->TipoMensaje);

	/* 3) TTL: 1b */
	memcpy( &(CABECERA_RouterRouter->TTL), &((*sCabeceraRouter)[iLargo]), sizeof(CABECERA_RouterRouter->TTL) );
	iLargo += sizeof(CABECERA_RouterRouter->TTL);

	/* 4) Hops: 1b */
	memcpy( &(CABECERA_RouterRouter->Hops), &((*sCabeceraRouter)[iLargo]), sizeof(CABECERA_RouterRouter->Hops) );
	iLargo += sizeof(CABECERA_RouterRouter->Hops);

	/*
	Conversion - Largo del Mensaje: SHORT (2b) => INT(4b)
	*/

	/* 5) Largo Mensaje: 2b */
	/*memcpy( &(CABECERA_RouterRouter->LargoMensaje), &((*sCabeceraRouter)[iLargo]), sizeof(CABECERA_RouterRouter->LargoMensaje) );
	iLargo += sizeof(CABECERA_RouterRouter->LargoMensaje);*/
	memcpy( &LargoMensajeAux, &((*sCabeceraRouter)[iLargo]), sizeof(LargoMensajeAux) );
	iLargo += sizeof(LargoMensajeAux);

	CABECERA_RouterRouter->LargoMensaje = (int) LargoMensajeAux;

	return received; /* Retorna la cantidad de bytes recibidos */
}

/*
	Recive Cabecera Router
*//*
int RecibeCabeceraRouter ( int FdFuente, CABECERA_ROUTERROUTER *CABECERA_RouterRouter )
{
	int received;

	// Recibe la cabecera
	received = recv( FdFuente, CABECERA_RouterRouter, sizeof(CABECERA_ROUTERROUTER), 0 );

	return received; // Retorna la cantidad de bytes recibidos
}
  */
/*
	Obtiene el timestamp
*/
time_t GetTimestamp ( void )
{
	time_t tTimestamp;
	/*
	char sTimestamp[14+2];
	time_t rawtime;
	struct tm * Date;
	*/

	/* Obtiene el EPOCH actual (cantidad de segundos transcurridos desde el 1º de Enero de 1970) */
	tTimestamp = time( NULL );

	/* Obtiene los datos actuales de fecha y hora */
	/*
	time ( &rawtime );
	Date = localtime ( &rawtime );
	*/
	/*sprintf( sTimestamp, "%d%02d%02d%02d%02d%02d",
	Date->tm_year+1900,Date->tm_mon+1,Date->tm_mday,Date->tm_hour,Date->tm_min,Date->tm_sec );*/
	/*
	sprintf( sTimestamp, "%02d%02d%02d%02d%02d",
					 Date->tm_mon+1,Date->tm_mday,Date->tm_hour,Date->tm_min,Date->tm_sec );
	*/

	return tTimestamp;
}



/****************************************************************************/
/* Inserta un ROUTER a la lista (FDROUTER) como "cola" */
FDROUTER * InsertaNodoFDROUTER ( FDROUTER *lista, FDROUTERinfo info )
{
	FDROUTER *pNew, *pAux;

	pNew = malloc( sizeof(FDROUTER) );

	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(FDROUTERinfo) );

		pNew->sgte = NULL;
		pAux = lista;

		while ( pAux != NULL && pAux->sgte != NULL ) {
			pAux = (FDROUTER *) pAux->sgte;
		}

		if ( pAux == NULL ) {
			lista = pNew;
		}
		else {
			pAux->sgte = (struct FDROUTER *) pNew;
		}

	}

	return lista;
}
/****************************************************************************/
/* Borra un equipo de la lista */
FDROUTER * SuprimeNodoFDROUTER ( FDROUTER *lista, FDROUTERinfo info )
{
	FDROUTER *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && info.FdRouter != pAux->info.FdRouter ) {
		pAnt = pAux;
		pAux = (FDROUTER *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (FDROUTER *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/* Inserta una ciudad al TOKEN */
TKN_LISTACIUDADES * InsertaNodoTokenCiudades ( TKN_LISTACIUDADES *lista, TKN_LISTACIUDADESinfo info )
{
	TKN_LISTACIUDADES *pNew, *pAux, *pAnt;

	int iOrden = 0;

	pNew = malloc( sizeof(TKN_LISTACIUDADES) );

	if ( pNew != NULL ) /* Memoria disponible */
	{
		pNew->sgte = NULL;
		pAux = lista;
		pAnt = lista;

		/*while ( pAux != NULL && info.Orden > pAux->info.Orden ) {*/
		while ( pAux != NULL ) {
			pAnt = pAux;
			pAux = (TKN_LISTACIUDADES *)pAux->sgte;
			iOrden++;
		}

		info.Orden = iOrden;

		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(TKN_LISTACIUDADESinfo) );

		if ( pAux == pAnt ) {
			lista = pNew;
		}
		else {
			pAnt->sgte = (struct TKN_LISTACIUDADES *)pNew;
		}

		pNew->sgte = (struct TKN_LISTACIUDADES *)pAux;

	}

	return lista;
}
/****************************************************************************/

TKN_LISTACIUDADES * SuprimeNodoTokenCiudades ( TKN_LISTACIUDADES *lista, char sNombreCiudad[] )
{
	TKN_LISTACIUDADES *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && strcmp( sNombreCiudad, pAux->info.Ciudad ) )
	{
		pAnt = pAux;
		pAux = (TKN_LISTACIUDADES *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (TKN_LISTACIUDADES *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}



/****************************************************************************/
/* Inserta un equipo a la tabla del TOKEN */
TKN_LISTATDP * InsertaNodoTokenTDP ( TKN_LISTATDP *lista, TKN_LISTATDPinfo info )
{
	TKN_LISTATDP *pNew, *pAux, *pAnt;

	pNew = malloc( sizeof(TKN_LISTATDP) );

	if ( pNew != NULL ) /* Memoria disponible */
	{
		/* Guarda los datos del INFO pasado por parametro */
		memcpy( &(pNew->info), &info, sizeof(TKN_LISTATDPinfo) );

		pNew->sgte = NULL;
		pAux = lista;
		pAnt = lista;

		while ( pAux != NULL && info.Puntaje > pAux->info.Puntaje ) {
			/* || strcmp(info.Equipo,pAux->info.Equipo) > 0 */
			pAnt = pAux;
			pAux = (TKN_LISTATDP *)pAux->sgte;
		}

		if ( pAux == pAnt ) {
			lista = pNew;
		}
		else {
			pAnt->sgte = (struct TKN_LISTATDP *)pNew;
		}

		pNew->sgte = (struct TKN_LISTATDP *)pAux;

	}

	return lista;
}
/****************************************************************************/
/* Borra un equipo de la tabla del TOKEN */
TKN_LISTATDP * SuprimeNodoTokenTDP ( TKN_LISTATDP *lista, TKN_LISTATDPinfo info )
{
	TKN_LISTATDP *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && strcmp(info.Equipo, pAux->info.Equipo) ) {
		pAnt = pAux;
		pAux = (TKN_LISTATDP *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (TKN_LISTATDP *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}



/****************************************************************************/
/****************************************************************************/

/* La siguiente funcion nos sirve a la hora de pasar el token completo de Router a Router */
long int AplanaToken (TOKEN Token, char ** sToken)
{
	/* Cantidad de Ciudades que estan dadas de alta en el Token */
	long int iCantCiudades=0;

	/* Cantidad de Equipos que estan dadas de alta en el Token */
	long int iCantEquipos=0;

	/* Largo total de la cadena del Token */
	long int iLargoToken=0;

	/* TAMAÑOS de nodos "info" */
	size_t sizeInfoCiudad = sizeof( TKN_LISTACIUDADESinfo );
	size_t sizeInfoTdp = sizeof( TKN_LISTATDPinfo );

	/* Auxiliar para la lista de CIudades */
	TKN_LISTACIUDADES *ptrAuxCiudades;

	/* Auxiliar para la lista de Equipos */
	TKN_LISTATDP *ptrAuxTabla;

	/* Inicializo los Auxiliares */
	ptrAuxCiudades = (TKN_LISTACIUDADES *) Token.lstCiudades;
	ptrAuxTabla = (TKN_LISTATDP *) Token.lstTdp;

	/* Cuento la cantidad de Ciudades que hay en el Token */
	while (ptrAuxCiudades != NULL)
	{
		iCantCiudades++;
		ptrAuxCiudades = (TKN_LISTACIUDADES *) ptrAuxCiudades->sgte;
	}

	/* Cuento la cantidad de Equipos que hay en el Token */
	while (ptrAuxTabla != NULL)
	{
		iCantEquipos++;
		ptrAuxTabla = (TKN_LISTATDP *) ptrAuxTabla->sgte;
	}

	/* Aloco memoria para la cadena del Token aplanado */
	*sToken = malloc (
											sizeof( Token.IdJuego ) /* ID de juego */
											+
											sizeof( iCantCiudades ) /* Numeros que indicara la cantidad de CIUDADES */
											+
											sizeof( iCantEquipos ) /* Numeros que indicara la cantidad de EQUIPOS */
											+
											sizeInfoCiudad * iCantCiudades /* CIUDADES */
											+
											sizeInfoTdp * iCantEquipos /* EQUIPOS */
										);

	/* PRIMERO el identificador del juego */
	memcpy( &((*sToken)[iLargoToken]), &(Token.IdJuego), sizeof(Token.IdJuego) );
	iLargoToken += sizeof( Token.IdJuego );

	/* LUEGO, 4 bytes seran destinados a un long int con la cantidad de CIUDADES */
	memcpy( &((*sToken)[iLargoToken]), &iCantCiudades, sizeof(iCantCiudades) );
	iLargoToken += sizeof( iCantCiudades );

	/* LUEGO, 4 bytes seran destinados a un long int con la cantidad de EQUIPOS */
	memcpy( &((*sToken)[iLargoToken]), &iCantEquipos, sizeof(iCantEquipos) );
	iLargoToken += sizeof( iCantEquipos );



	/* Compruebo si hay info para almacenar en el sToken */
	if ( !iCantCiudades && !iCantEquipos )
		return iLargoToken;


	/* RE-Inicializo los Auxiliares */
	ptrAuxCiudades = (TKN_LISTACIUDADES *) Token.lstCiudades;
	ptrAuxTabla = (TKN_LISTATDP *) Token.lstTdp;


	/* Cargo la info con respecto a las ciudades del Token al sToken */
	while (ptrAuxCiudades != NULL)
	{
		memcpy( &((*sToken)[iLargoToken]), &(ptrAuxCiudades->info), sizeInfoCiudad );
		iLargoToken += sizeInfoCiudad;
		ptrAuxCiudades = (TKN_LISTACIUDADES *) ptrAuxCiudades->sgte;
	}


	/* Cargo la info con respecto a los equipos del Token a el sToken */
	while (ptrAuxTabla != NULL)
	{
		memcpy( &((*sToken)[iLargoToken]), &(ptrAuxTabla->info), sizeInfoTdp );
		iLargoToken += sizeInfoTdp;
		ptrAuxTabla = (TKN_LISTATDP *) ptrAuxTabla->sgte;
	}

	/* La funcion retorna la cantidad de bytes que mide el sToken */
	return iLargoToken;
}
/****************************************************************************/
void DesaplanaToken ( TOKEN *Token, char *sToken )
{
	long int iCantCiudades;
	long int iCantEquipos;
	long int iIdJuego;
	long int iPosicionPtr = 0;
	long int i;

	/* Punteros */
	TKN_LISTACIUDADES *lstCiudades = NULL;
	TKN_LISTATDP *lstTdp = NULL;

	/* Infos */
	TKN_LISTACIUDADESinfo strInfoCiudades;
	TKN_LISTATDPinfo strInfoEquipos;

	/* TAMAÑOS de nodos "info" */
	size_t sizeInfoCiudad = sizeof( TKN_LISTACIUDADESinfo );
	size_t sizeInfoTdp = sizeof( TKN_LISTATDPinfo );

	/* Obtiene el ID de juego */
	memcpy( &iIdJuego, &(sToken[iPosicionPtr]), sizeof(iIdJuego) );
	iPosicionPtr += sizeof( iIdJuego );

	/* Obtiene la cantidad de CIUDADES */
	memcpy( &iCantCiudades, &(sToken[iPosicionPtr]), sizeof(iCantCiudades) );
	iPosicionPtr += sizeof( iCantCiudades );

	/* Obtiene la cantidad de EQUIPOS */
	memcpy( &iCantEquipos, &(sToken[iPosicionPtr]), sizeof(iCantEquipos) );
	iPosicionPtr += sizeof( iCantEquipos );

	for( i=0; i < iCantCiudades; i++ )
	{
		memcpy( &strInfoCiudades, &(sToken[iPosicionPtr]), sizeInfoCiudad );
		/*Token->lstCiudades = InsertaNodoTokenCiudades( Token->lstCiudades, strInfoCiudades );*/
		lstCiudades = InsertaNodoTokenCiudades( lstCiudades, strInfoCiudades );
		iPosicionPtr += sizeInfoCiudad;
	}

	for( i=0; i < iCantEquipos; i++ )
	{
		memcpy( &strInfoEquipos, &(sToken[iPosicionPtr]), sizeInfoTdp );
		/*Token->lstTdp = InsertaNodoTokenTDP( Token->lstTdp, strInfoEquipos );*/
		lstTdp = InsertaNodoTokenTDP( lstTdp, strInfoEquipos );
		iPosicionPtr += sizeInfoTdp;
	}

	/* Guarda los datos del TOKEN */
	Token->IdJuego = iIdJuego;
	Token->lstCiudades = (struct TKN_LISTACIUDADES *) lstCiudades;
	Token->lstTdp = (struct TKN_LISTATDP *) lstTdp;

	return;
}



/* Se inserta el mensaje en la lista de espera (modo pila) */
DISCOVERSENT * InsertaNodoDiscoverSent( DISCOVERSENT *lista, int iFdOrigen, int iIdMensaje )
{
	DISCOVERSENT *pNew;
	DISCOVERSENTinfo info;

	/* Obtiene el EPOCH actual */
	info.Timeout = GetTimestamp();
	/* Guarda el FD de Origen */
	info.FdOrigen = iFdOrigen;
	/* Guarda el ID del Mensaje */
	info.IdMensaje = iIdMensaje;

	pNew = malloc( sizeof(DISCOVERSENT) );

	memcpy( &(pNew->info), &info, sizeof(DISCOVERSENTinfo) );

	pNew->sgte = (struct DISCOVERSENT *) lista;
	lista = pNew;

	return lista;
}

/* Se elimina el mensaje de la lista de espera */
DISCOVERSENT * SuprimeNodoDiscoverSent( DISCOVERSENT *lista, DISCOVERSENT *pDel )
{
	DISCOVERSENT *pAux, *pAnt;

	pAux = lista;
	pAnt = lista;

	while ( pAux != NULL && pAux != pDel ) {
		pAnt = pAux;
		pAux = (DISCOVERSENT *)pAux->sgte;
	}

	if ( pAux == NULL ) { /* No hay ningun nodo para suprimir */
		return FALSE;
	}
	else if ( pAux == lista ) { /* Hay que borrar el primer nodo */
		lista = (DISCOVERSENT *)lista->sgte;
	}
	else { /* Hay que borrar un nodo del medio o el ultimo */
		pAnt->sgte = pAux->sgte;
	}

	free( pAux );

	return lista;
}




/*recibir nombre cabecera (NO_JUGADO, BE_BUSCAR)*/

/*FH - agregado 20060528*/

/*busca al siguiente equipo en la lista que no haya jugado. Contempla así el tema del encolamiento de equipos tambien*/
LEVRESULTADOS * JuegaSiguientePartido( int iTipoMensaje, LEVRESULTADOS **lstLevResultados, char sNombreEquipo[30], int iFdCiudad, int *iIdMensaje, int iMigrado, char sIPCiudadOrigen[], char sPortCiudadOrigen[], int *iFdCiudadOrigen )
{
	/*recibo lstLevResultados por referencia*/
	LEVRESULTADOS *lstLevResultadosAux = *lstLevResultados;

	/* Cadena Lev Resultados */
	char *sLevResultados = NULL;

	/* Direccion de la Ciudad Origen */
	struct in_addr strAddrCiudadOrigen;
	unsigned long iIPCiudadOrigen;
	int iPortCiudadOrigen = atoi( sPortCiudadOrigen );

	inet_aton( sIPCiudadOrigen, &strAddrCiudadOrigen );
	iIPCiudadOrigen = htonl( strAddrCiudadOrigen.s_addr );

	/*si hay resultados, sigo procesando los equipos, sino, no hago nada, porque ese equipo no tiene la LEV anymore*/
	if ( *lstLevResultados != NULL )
	{
		/* busca el proximo equipo que no haya jugado (puede ser un equipo contra el que todavia no intento jugar, o un equipo contra el que intento jugar, pero no estaba disponible en ese momento (funciona como una cola de esta manera*/
		while  (
					  lstLevResultadosAux != NULL   &&
					  ( lstLevResultadosAux->info.Resultado != NO_JUGADO   ||
					  strcmp( sNombreEquipo,lstLevResultadosAux->info.Equipo ) == 0 )
			   )
		{
			/* Busca el equipo sin jugar siguiente */
			lstLevResultadosAux = (LEVRESULTADOS *) lstLevResultadosAux->sgte;
		}

		/* Si encontro el equipo, continúa con su búsqueda */
		if (lstLevResultadosAux != NULL)
		{

			/****** Envia cabecera a CIUDAD ******/
			if ( EnviaCabecera( iFdCiudad, iIdMensaje, iTipoMensaje, strlen(lstLevResultadosAux->info.Equipo)+1 ) == -1 )
				WriteLog( ERROR, sNombreEquipo, "Error:59: Error al pedir permiso a ciudad para jugar" );

			/* Envía a CIUDAD busqueda de equipo */
			if ( send( iFdCiudad, lstLevResultadosAux->info.Equipo, strlen(lstLevResultadosAux->info.Equipo)+1, 0 ) == -1 )
				WriteLog( ERROR, sNombreEquipo, "Error:10: Error al pedir permiso a ciudad para jugar" );

		}
		/*si no lo encontró, quiere decir que termino de jugar todos los partidos, entonces, envia la LEV de resultados a CIUDAD*/
		else
		{
			/* Se terminó de jugar contra TODOS los Equipos */

			if ( iMigrado == FALSE ) /* El Equipo se encuentra en su Ciudad Origen (termino LEV INTERNA) */
			{
				/* Actualiza el string sLevResultados */
				EncriptaLevResultados (*lstLevResultados, &sLevResultados);

				EnviaCabecera( iFdCiudad, iIdMensaje, LEV_RESULTADOS, strlen(sLevResultados)+1 );

				if ( send( iFdCiudad, sLevResultados, strlen(sLevResultados)+1, 0 ) == -1 )
				{
					WriteLog( ERROR, sNombreEquipo, "Error:26: Error al enviar resultados" );
					/* Se corta la ejecucion */
					exit(0);
				}

				/* Elimina la LEV, se la enviamos a otro equipo */
				LiberaLevResultados( lstLevResultados );
				lstLevResultados = NULL; /* Por si las moscas... */

				WriteLog( INFO, sNombreEquipo, "Termina de jugar contra todos, y devuelve la LEV a ciudad" );
			}
			else
			{
				/* Se conecta con su Ciudad Origen */

				*iFdCiudadOrigen = AbreConexion( iIPCiudadOrigen, iPortCiudadOrigen );

				if ( *iFdCiudadOrigen == -1 )
				{
					/* aca, lo que pase nos chupa bien un huevo, porque como la ciudad tenia el token, se caga todo el juego y eso esta bien */

					WriteLog( ERROR, sNombreEquipo, "\nFATAL : 15468568 : *************** Murio la ciudad que tenia el TOKEN *************\n\n" );
					printf("FATAL: \n*************** Murio la ciudad que tenia el TOKEN *************\n\n");
				}

			}

		}
	}

	return lstLevResultadosAux;

}

/****************************************************************************/
/************************ C I U D A D  ::  B E G I N ************************/
/****************************************************************************/

void Setea_FDS_FinPasadaToken ( FDS **lstFds )
{

	FDS *lstFdsAux = NULL;

	lstFdsAux = *lstFds;

	while ( lstFdsAux != NULL )
	{
		if (
							lstFdsAux->info.Migrado == FALSE
						&&
							lstFdsAux->info.TokenSet == TRUE
						/*&&
					lstFdsAux->info.LevSentExt == ENVIO_RESULTADOS*/
						&&
							lstFdsAux->info.Estado != MIGRADO

			 )
		{
			lstFdsAux->info.LevSentExt = TERMINADO;
		}

		lstFdsAux = (FDS *) lstFdsAux->sgte;
	}



}


/* Acciones a tomar cuando el CDE murio */
void CIUDAD_CDE_EstaMuerto ( int *iFdCDE, char *sNombreProceso )
{
	if ( *iFdCDE > -1 )
	{
		printf("\n\n++ *** Murio el CDE! Los equipos no podran entrenar *** ++\n\n");

		WriteLog(ERROR, sNombreProceso, "Murio el CDE! Los equipos no podran entrenar");

		close( *iFdCDE );

		*iFdCDE = -1;

		/*ctrl_c();*/ /* Esta bien que se corte la ejecucion del programa por esto?? */
	}

	return;
}

/* Expulsa un Equipo del CDE */
int CIUDAD_CDE_ExpulsaEquipo ( int *iFdCDE, char sNombreEquipo[], char sNombreCiudad[] )
{
	/* Valor de retorno del SEND */
	int iResult = -1;

	if ( *iFdCDE > -1 ) /* Esto esta planteado asi por si hacemos que no se corte toda la ejecucion */
	{
		/* Envia el pedido de expulsion de Equipo al CDE */
		iResult = send( *iFdCDE, sNombreEquipo, strlen(sNombreEquipo)+1, 0 );

		if ( iResult < 1 ) /* Fallo el envio al CDE => Esta muerto */
		{
			CIUDAD_CDE_EstaMuerto( iFdCDE, sNombreCiudad );

			return FAIL;
		}
	}
	else /* Esto esta planteado asi por si hacemos que no se corte toda la ejecucion */
	{
		return FAIL;
	}

	return OK;
}

/* Tratamiento de la muerte de un EQUIPO */
void CIUDAD_MuereEquipo ( FDS **lstFds, LEV **lstLevInterna, char **sLev, FDS *lstFdsAux, int *iFdCDE, char sNombreCiudad[] )
{
	/* Pide al CDE que expulse al equipo que ha muerto (sin chequear si esta o no) */
	CIUDAD_CDE_ExpulsaEquipo ( iFdCDE, lstFdsAux->info.Equipo, sNombreCiudad );
	sleep(1);

	close( lstFdsAux->info.FdEquipo );

	/* Elimina la informacion del Equipo en las listas */
	*lstLevInterna = SuprimeNodoLEV ( *lstLevInterna, lstFdsAux->info.Equipo );
	*lstFds = SuprimeNodoFDS( *lstFds, lstFdsAux->info.Equipo );

	/* Regenera LEV interna */
	EncriptaLev( *lstLevInterna, sLev );

	return;
}

/* Pasa la LEV al siguiente equipo */
FDS * CIUDAD_EnviaLevEquipo (
												FDS *lstFds, FDS **lstFdTieneLEV, LEV **lstLevInterna,
												int *iFdCDE, int *iIdMensaje, char *sNombreCiudad, int *iResult, char TipoLev, int *iCantEquipos
										 )
{
	/* Puntero auxiliar para recorrer la lista FDS */
	FDS *lstFdsAux = lstFds;

	/* Cadena de Lev encriptada */
	char *sLev = NULL;
	/* String para guardar en el LOG */
	char sLog[250];

	char iEncontrado = FALSE;

	/* Inicializa el RESULTADO de la funcion */
	*iResult = OK;


	/* Busqueda del primer equipo para enviarle la LEV */
	while ( iEncontrado == FALSE && lstFdsAux != NULL )
	{
		if ( lstFdsAux->info.TokenSet == TRUE )
		{
			if ( TipoLev == ES_LEV_INTERNA && lstFdsAux->info.LevSent == FALSE )
				iEncontrado = TRUE;

			else if ( TipoLev == ES_LEV_EXTERNA && lstFdsAux->info.LevSentExt == FALSE )
			{
				iEncontrado = TRUE;
				(*iCantEquipos)++;
			}

			else
				lstFdsAux = (FDS *) lstFdsAux->sgte;
		}
		else
			lstFdsAux = (FDS *) lstFdsAux->sgte;

	}

	if ( lstFdsAux != NULL ) /* Encontramos un equipo para enviarle la LEV */
	{

		/* Convierte la LEV: lista => cadena */
		EncriptaLev( *lstLevInterna, &sLev );

		/* Pide al CDE que expulse al Equipo que debe recibir la LEV (sin chequear si esta o no) */
		CIUDAD_CDE_ExpulsaEquipo ( iFdCDE, lstFdsAux->info.Equipo, sNombreCiudad );
		sleep(1);

		printf("\n\nENVIO DE LEV AL EQUIPO: %s\n\n", lstFdsAux->info.Equipo);

		/* Escribe en el LOG */
		sprintf( sLog, "Envio de LEV al equipo \"%s\"", lstFdsAux->info.Equipo );
		WriteLog( INFO, sNombreCiudad, sLog );

		/****** Envia cabecera a EQUIPO ******/
		EnviaCabecera( lstFdsAux->info.FdEquipo, iIdMensaje, LEV_ENVIAR, strlen(sLev)+1 );

		/* Envía a EQUIPO la LEV (como cadena) */
		if ( send( lstFdsAux->info.FdEquipo, sLev, strlen(sLev)+1, 0 ) < 1 )
		{
			/* Escribe en el LOG */
			sprintf( sLog, "Error:22 Error al enviar la LEV al equipo \"%s\". Se elimino de la lista!", lstFdsAux->info.Equipo );
			WriteLog( ERROR, sNombreCiudad, sLog );

			close( lstFdsAux->info.FdEquipo );
			lstFdsAux->info.FdEquipo = -1;

			/* Elimina el Equipo de ambas listas */
			*lstLevInterna = SuprimeNodoLEV( *lstLevInterna, lstFdsAux->info.Equipo );
			lstFds = SuprimeNodoFDS( lstFds, lstFdsAux->info.Equipo );

			/* LLAMADA RECURSIVA: Debemos continuar el envio al siguiente EQUIPO */
			return CIUDAD_EnviaLevEquipo( lstFds, lstFdTieneLEV, lstLevInterna, iFdCDE, iIdMensaje, sNombreCiudad, iResult, TipoLev, iCantEquipos );
		}
		else
		{
			/* Se actualiza el puntero del equipo que tiene la lEV */
			*lstFdTieneLEV = lstFdsAux;

			/* Escribe en el LOG */
			sprintf( sLog, "Se envio correctamente la LEV a \"%s\"", lstFdsAux->info.Equipo );
			WriteLog( INFO, sNombreCiudad, sLog );

			/* Actualiza el estado de envio de LEV para el equipo seleccionado */

			sprintf( sLog, "y la LEV essss \"%s\"", sLev );
			WriteLog( INFO, sNombreCiudad, sLog );

			if ( TipoLev == ES_LEV_INTERNA )
				lstFdsAux->info.LevSent = TRUE;

			else if ( TipoLev == ES_LEV_EXTERNA )
			{
				lstFdsAux->info.LevSentExt = TRUE;
				/*lstFdsAux->info.Estado = MIGRADO;*/
				/*lstFdsAux->info.Timeout = GetTimestamp();*/
			}

		}

	}
	else
	{
		/* Termino de enviarse la LEV a todos los equipos, sale del ciclo */
		*lstFdTieneLEV = NULL;
	}

	return lstFds;
}

/* Finalizo el torneo interno */
FDS * CIUDAD_FinTorneoInterno ( FDS *lstFds, int *iFdCDE, char sNombreCiudad[] )
{
	/* Puntero auxiliar para recorrer la lista FDS */
	FDS *lstFdsAux = lstFds;
	/* String para guardar en el LOG */
	char sLog[250];


	printf("\n\nSE TERMINO DE JUGAR EL TORNEO INTERNO!!");

	/* Escribe en el LOG */
	sprintf( sLog, "\n\nSe termino de jugar el torneo interno!!\n\n" );
	WriteLog( INFO, sNombreCiudad, sLog );

	/* Volamos a todos los equipos que esten en el CDE */
	while ( lstFdsAux != NULL )
	{
		if ( lstFdsAux->info.Estado == DESCANSANDO || lstFdsAux->info.Estado == ENTRENANDO )
		{
			/*
			Encontro un equipo en el CDE => Lo expulsa
			(sin chequeo, para no abortar la ejecucion del programa justo al final)
			*/

			/* Pide al CDE que expulse al Equipo */
			CIUDAD_CDE_ExpulsaEquipo ( iFdCDE, lstFdsAux->info.Equipo, sNombreCiudad );
		}

		lstFdsAux = (FDS *) lstFdsAux->sgte;
	}

	sleep(1);

	/* Ordena la lista por PUNTAJES (de mayor a menor) */
	lstFds = OrdenaPuntajeFDS( lstFds );

	lstFdsAux = lstFds;

	printf("\n\n************LISTA DE PUNTAJES***************\n\n");

	while ( lstFdsAux != NULL )
	{
		printf("Equipo %s -> Puntaje %d\n", lstFdsAux->info.Equipo, lstFdsAux->info.Puntaje);

		/* Escribe en el LOG */
		sprintf( sLog, "Equipo %s -> Puntaje %d", lstFdsAux->info.Equipo, lstFdsAux->info.Puntaje );
		WriteLog( INFO, sNombreCiudad, sLog );

		lstFdsAux = (FDS *) lstFdsAux->sgte;
	}

	printf("\n\n");

	return lstFds;
}

/* Procesa los resultados, sumando los puntos correspondientes a cada Equipo */
void CIUDAD_ProcesaLevResultados ( LEVRESULTADOS *lstLevResultados, FDS *lstFds, FDS *lstFdsAux )
{
	FDS *lstFdsEstAux = NULL;

	/* Comienza a recorrer la lista de resultados */
	printf("\nResultados de partidos jugados por \"%s\":\n", lstFdsAux->info.Equipo);

	while( lstLevResultados != NULL )
	{
		/*
			Si se perdio o se empato un partido..
				Se busca el nombre del equipo contrincante para sumarle los puntos
		*/
		lstFdsEstAux = lstFds;

		while ( lstFdsEstAux!=NULL && strcmp(lstLevResultados->info.Equipo,lstFdsEstAux->info.Equipo) !=0 )
		{
			lstFdsEstAux = (FDS *) lstFdsEstAux->sgte;
		}

		if ( lstFdsEstAux != NULL ) /* Se encontro al equipo contrincante */
		{
			if ( lstLevResultados->info.Resultado != NO_JUGADO && lstLevResultados->info.Resultado != EQUIPO_KILLED )
				printf("\nContrincante: %s, Resultado: ", lstLevResultados->info.Equipo);

			/************* ACTUALIZA PUNTAJES DE EQUIPOS *************/

			switch ( lstLevResultados->info.Resultado )
			{
				case VICTORIA:
					lstFdsAux->info.Puntaje += 3;
					printf("GANADO");
					break;
				case EMPATE:
					lstFdsAux->info.Puntaje += 1; /* Suma puntos al equipo que nos envio LEV_RESULTADOS */
					lstFdsEstAux->info.Puntaje += 1; /* Suma puntos al equipo contra el que empato */
					printf("EMPATADO");
					break;
				case DERROTA:
					lstFdsEstAux->info.Puntaje += 3; /* Suma puntos al equipo contra el que perdio */
					printf("PERDIDO");
					break;
				case EQUIPO_KILLED:
					/* El equipo contrincante MURIO */

					/*
						No se deberia hacer nada, ya que en este punto el equipo YA ESTARIA ELIMINADO de todas las listas
					*/
					printf("NO JUGADO");

				break;
			}

		}
		lstLevResultados = (LEVRESULTADOS *)lstLevResultados->sgte;
	}

	printf("\n");

	return;
}



/* Arma la LEV EXTERNA en base a la TDP */
int ArmaLevExterna ( LEV **lstLevExterna, TKN_LISTATDP *lstTdp, char sNombreCiudad[] )
{
	TKN_LISTATDP *lstTdpAux = lstTdp;
	LEVinfo info;

	LEV * lstLevExternaAux = NULL;

	while ( *lstLevExterna != NULL )
	{
		lstLevExternaAux = *lstLevExterna;
		*lstLevExterna = (LEV *) (*lstLevExterna)->sgte;
		free ( lstLevExternaAux );
	}

	*lstLevExterna = NULL;

	while (lstTdpAux != NULL)
	{
		/*printf( "\nCiudad: %s -- Equipo:%s", lstTdpAux->info.Ciudad, lstTdpAux->info.Equipo );*/

		/* Esta funcion tambien retorna valores < 0, que el IF toma como FALSO... por eso el != 0 */
		if ( strcmp(lstTdpAux->info.Ciudad, sNombreCiudad) != 0 )
		{
			strcpy(info.Equipo, lstTdpAux->info.Equipo);
			strcpy(info.CiudadOrigen, lstTdpAux->info.Ciudad);

			*lstLevExterna = InsertaNodoLev(*lstLevExterna, info);
		}

		lstTdpAux = (TKN_LISTATDP *) lstTdpAux->sgte;
	}

	return OK;
}


/* Restablece que no se le mandó la LEV externa a los equipos (una vez que terminaron la Lev Interna) */
int RestablecerFds(FDS **lstFds)
{
	FDS *lstFdsAux;

	lstFdsAux = *lstFds;

	while ( lstFdsAux != NULL )
	{
		/*if ( lstFdsAux->info.TokenSet == FALSE && lstFdsAux->info.Nuevo == FALSE )*/
			lstFdsAux->info.LevSent = FALSE;

		lstFdsAux = (FDS *) lstFdsAux->sgte;
	}

	return OK;
}

/* Busca la Ciudad a la que pertenece el equipo, dentro del Token */
TKN_LISTACIUDADES * BuscaCiudadEnToken( TOKEN strToken, char sEquipo[] )
{
	TKN_LISTATDP *lstTdpAux;
	TKN_LISTACIUDADES *pTknCiudad;

	lstTdpAux = (TKN_LISTATDP *) strToken.lstTdp;
	pTknCiudad = (TKN_LISTACIUDADES *) strToken.lstCiudades;

	while ( lstTdpAux != NULL && strcmp(lstTdpAux->info.Equipo, sEquipo) )
	{
		lstTdpAux = (TKN_LISTATDP *) lstTdpAux->sgte;
	}

	if (lstTdpAux != NULL) /* Se encontro la Ciudad buscada */
	{
		/* Se devuelve la informacion de la Ciudad */
		while ( pTknCiudad != NULL && strcmp(lstTdpAux->info.Ciudad, pTknCiudad->info.Ciudad) )
		{
			pTknCiudad = (TKN_LISTACIUDADES *) pTknCiudad->sgte;
		}

	}
	else
	{
		pTknCiudad = NULL;
	}

	return pTknCiudad;
}


/* Actualiza TDP del TOKEN */
int ActualizaTdp ( TOKEN strToken , LEVRESULTADOS *lstLevResultados, char sEquipo[30] )
{
	TKN_LISTATDP *lstTdpYo, *lstTdpOtro, *lstTdp;

	lstTdp = (TKN_LISTATDP *) strToken.lstTdp;

	/* Comienza a recorrer la lista de resultados */
	printf("\nResultados de partidos jugados por \"%s\":", sEquipo);

	while ( lstLevResultados != NULL )
	{
		lstTdpYo = lstTdpOtro = lstTdp;

		while ( lstTdpYo != NULL && strcmp(lstTdpYo->info.Equipo, sEquipo) )
		{
			lstTdpYo = (TKN_LISTATDP *) lstTdpYo->sgte;
		}

		while ( lstTdpOtro != NULL && strcmp(lstTdpOtro->info.Equipo, lstLevResultados->info.Equipo) )
		{
			lstTdpOtro = (TKN_LISTATDP *) lstTdpOtro->sgte;
		}

		if (lstTdpYo != NULL && lstTdpOtro != NULL)
		{
			printf("\nContrincante: %s, Resultado: ", lstTdpOtro->info.Equipo);

			switch ( lstLevResultados->info.Resultado )
			{
				case VICTORIA:

					lstTdpYo->info.Puntaje += 3;
					lstTdpYo->info.PartidosGanados += 1;
					lstTdpOtro->info.PartidosPerdidos += 1;
					printf("GANADO");

				break;

				case EMPATE:

					lstTdpYo->info.Puntaje += 1; /* Suma puntos al equipo que nos envio LEV_RESULTADOS */
					lstTdpOtro->info.Puntaje += 1; /* Suma puntos al equipo contra el que empato */
					lstTdpYo->info.PartidosEmpatados += 1;
					lstTdpOtro->info.PartidosEmpatados += 1;
					printf("EMPATADO");

				break;

				case DERROTA:

					lstTdpOtro->info.Puntaje += 3; /* Suma puntos al equipo contra el que perdio */
					lstTdpYo->info.PartidosPerdidos += 1;
					lstTdpOtro->info.PartidosGanados += 1;
					printf("PERDIDO");

				break;

				default: /* Solo para informar en pantalla.. */

					printf("NO JUGADO");

				break;

			}
		}
		else
			return FAIL;

		lstLevResultados = (LEVRESULTADOS *) lstLevResultados->sgte;
	}

	return OK;
}


TKN_LISTATDP * CIUDAD_ChequeaEliminacionEquiposFD ( TKN_LISTATDP *lstTokenTdp, FDS *lstFds, char sNombreCiudad[] )
{

	FDS *lstFdsAux = lstFds;
	int iEncontrado;
	TKN_LISTATDP *lstTokenTdpAux = lstTokenTdp;

	/* Chequea si se elimino un equipo. De ser asi, lo elimina del TOKEN */
	while ( lstTokenTdpAux != NULL )
	{

		if ( strcmp(lstTokenTdpAux->info.Ciudad, sNombreCiudad) == 0 )
		{
			iEncontrado = FALSE;

			lstFdsAux = lstFds;

			while ( lstFdsAux != NULL && iEncontrado == FALSE )
			{
				if ( lstFdsAux->info.TokenSet == FALSE || strcmp(lstTokenTdpAux->info.Equipo, lstFdsAux->info.Equipo) == 0 )
					iEncontrado = TRUE;
				else
					lstFdsAux = (FDS*) lstFdsAux->sgte;
			}

			if ( iEncontrado == FALSE )
			{
				/* Elimina el nodo porque el equipo no existe mas en FDS (lo mataron) */
				lstTokenTdp = (TKN_LISTATDP *) SuprimeNodoTokenTDP( (TKN_LISTATDP *) lstTokenTdp, lstTokenTdpAux->info );
				lstTokenTdpAux = lstTokenTdp;
			}
			else
			{
				lstTokenTdpAux = (TKN_LISTATDP *) lstTokenTdpAux->sgte;
			}
		}
		else
			lstTokenTdpAux = (TKN_LISTATDP *) lstTokenTdpAux->sgte;
	}


	return lstTokenTdp;
}

/* Verifica si hay Equipos nuevos creados que aun no se ingresaron al TOKEN */
void AgregaEquiposToken ( FDS *lstFds, TOKEN *strToken, char sNombreCiudad[] )
{
	TKN_LISTATDPinfo strInfoTdp;
	FDS *lstFdsAux = lstFds;


	while ( lstFdsAux != NULL )
	{
		if ( lstFdsAux->info.TokenSet == FALSE )
		{
			/* Encontramos un Equipo que debe ser ingresado al TOKEN */

			sprintf( strInfoTdp.Ciudad, "%s", sNombreCiudad );
			sprintf( strInfoTdp.Equipo, "%s", lstFdsAux->info.Equipo );
			strInfoTdp.Puntaje = 0;
			strInfoTdp.PartidosGanados = 0;
			strInfoTdp.PartidosEmpatados = 0;
			strInfoTdp.PartidosPerdidos = 0;

			lstFdsAux->info.TokenSet = TRUE;

			strToken->lstTdp = (struct TKN_LISTATDP *) InsertaNodoTokenTDP( (TKN_LISTATDP *)strToken->lstTdp, strInfoTdp );
		}

		lstFdsAux = (FDS *) lstFdsAux->sgte;
	}

	/* Chequea si se elimino un equipo. De ser asi, lo elimina del TOKEN */
	strToken->lstTdp = (struct TKN_LISTATDP *) CIUDAD_ChequeaEliminacionEquiposFD ( (TKN_LISTATDP *)strToken->lstTdp, lstFds, sNombreCiudad );

	return;
}

/****************************************************************************/
/************************** C I U D A D  ::  E N D **************************/
/****************************************************************************/


/****************************************************************************/
/************************ R O U T E R  ::  B E G I N ************************/
/****************************************************************************/


/*
	Envia el TOKEN a un nuevo ROUTER conectado
*/
int ROUTER_EnviaTokenNuevoRouter ( FDROUTER **lstFdRouter, int *iIdMensaje, char **sToken, long int iLargoToken, CABECERA_ROUTERROUTER CABECERA_RouterRouter, char **sCabeceraRouter )
{
	FDROUTER *lstFdRouterAux = *lstFdRouter;
	int iEnvioCorrecto = FALSE;


	while ( lstFdRouterAux != NULL && lstFdRouterAux->info.TokenGive == FALSE )
	{
		lstFdRouterAux = (FDROUTER *) lstFdRouterAux->sgte;
	}

	if ( lstFdRouterAux != NULL ) /* Se encontro un ROUTER que todavia no fue agregado al TOKEN */
	{
		/* Trata de enviar el TOKEN al nuevo ROUTER */
		EnviaCabeceraRouter( lstFdRouterAux->info.FdRouter, iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, sCabeceraRouter, ROUTER_ROUTER );

		if ( send(lstFdRouterAux->info.FdRouter, *sToken, iLargoToken, 0) > 0 )
		{
			/* Se envio el TOKEN => Libera memoria y actualiza el estado de TokenSet para ese ROUTER */
			free ( *sToken );
			*sToken = NULL;

			lstFdRouterAux->info.TokenGive = FALSE;

			return TRUE;
		}

		/* Fallo el envio del TOKEN (el nuevo ROUTER no responde) => Suprime el NODO */
		*lstFdRouter = SuprimeNodoFDROUTER ( *lstFdRouter, lstFdRouterAux->info );

		/* LLAMADA RECURSIVA: No se pudo enviar el TOKEN al ROUTER => Se prueba con el siguiente */
		iEnvioCorrecto = ROUTER_EnviaTokenNuevoRouter ( lstFdRouter, iIdMensaje, sToken, iLargoToken, CABECERA_RouterRouter, sCabeceraRouter );
	}

	return iEnvioCorrecto;
}


/* Busca en el TOKEN la posicion del proximo ROUTER */
TKN_LISTACIUDADES * ROUTER_BuscaTokenProximoRouter ( char sNombreCiudad[], TKN_LISTACIUDADES *lstTokenCiudades )
{
	TKN_LISTACIUDADES *lstTokenCiudadesAux = lstTokenCiudades;

	/* Comienza a buscar en la lista de Ciudades, buscandose a si misma */
	while ( lstTokenCiudadesAux != NULL && strcmp(lstTokenCiudadesAux->info.Ciudad, sNombreCiudad) )
	{
		/*printf("CIUDA: %s\n", lstTokenCiudadesAux->info.Ciudad);*/
		lstTokenCiudadesAux = (TKN_LISTACIUDADES *) lstTokenCiudadesAux->sgte;
	}


	if ( lstTokenCiudadesAux == NULL )
	{
		/* ALGO ESTA MAL, ESTO NO PUEDE SUCEDERRRRRRRRRRRRRRRRR!!!!!!!!!! */
		/*printf("\n----VARIAS (linea 2069): !!! ERROR !!!\n\tComo es posible que YO MISMO no figure en el TOKEN?!?!? Me programaron como el orto!!!\n");*/
		ctrl_c();
	}
	else
	{
		/*printf("CIUDA: %s\n", lstTokenCiudadesAux->info.Ciudad);*/
		/* Se desplaza el puntero al proximo nodo */
		lstTokenCiudadesAux = (TKN_LISTACIUDADES *) lstTokenCiudadesAux->sgte;
	}

	if ( lstTokenCiudadesAux == NULL ) /* Nosotros eramos el ultimo nodo */
	{
		/* Entonces apuntamos al primero de la lista */
		lstTokenCiudadesAux = lstTokenCiudades;
	}

	/*printf("CIUDA: %s\n", lstTokenCiudadesAux->info.Ciudad);*/

	return lstTokenCiudadesAux;
}

/* Elimina una Ciudad y sus Equipos del TOKEN */
void ROUTER_EliminaCiudadToken ( TOKEN *strToken, char sNombreCiudad[] )
{
	/* Punteros a TOKEN */
	TKN_LISTACIUDADES *lstTokenCiudades = (TKN_LISTACIUDADES *) (*strToken).lstCiudades;
	TKN_LISTATDP *lstTokenTdp = (TKN_LISTATDP *) (*strToken).lstTdp, *lstTokenTdpAux = NULL;

	lstTokenTdpAux = lstTokenTdp;

	while (lstTokenTdpAux != NULL)
	{
		if ( strcmp(lstTokenTdpAux->info.Ciudad, sNombreCiudad) == 0 )
		{
			/* Se encontro un Equipo perteneciente a la Ciudad => Se elimina */
			lstTokenTdp = (TKN_LISTATDP *) SuprimeNodoTokenTDP( lstTokenTdp, lstTokenTdpAux->info );
			lstTokenTdpAux = lstTokenTdp;
		}
		else
		{
			lstTokenTdpAux = (TKN_LISTATDP *) lstTokenTdpAux->sgte;
		}
	}

	/* Elimina Ciudad de la lista del TOKEN */
	lstTokenCiudades = (TKN_LISTACIUDADES *) SuprimeNodoTokenCiudades ( lstTokenCiudades, sNombreCiudad );

	/* Actualiza la estructura */
	(*strToken).lstCiudades = (struct TKN_LISTACIUDADES *) lstTokenCiudades;
	(*strToken).lstTdp = (struct TKN_LISTATDP *) lstTokenTdp;

	return;

}

/* Envia el TOKEN al proximo ROUTER */
int ROUTER_EnviaTokenProximoRouter ( char sNombreCiudad[], int *iIdMensaje, char **sToken, long int *iLargoToken, CABECERA_ROUTERROUTER CABECERA_RouterRouter, char **sCabeceraRouter, int iGameOver )
{
	/* Estructura del TOKEN */
	TOKEN strToken;
	/* Punteros a TOKEN */
	TKN_LISTACIUDADES *lstTokenCiudades = NULL;
	/*TKN_LISTATDP *lstTokenTdp = NULL;*/

	int iFdRouter = -1;
	int iEnvioCorrecto = FALSE;

	/* Obtiene el TOKEN como esctructura */
	DesaplanaToken ( &strToken, *sToken );

	lstTokenCiudades = ROUTER_BuscaTokenProximoRouter( sNombreCiudad, (TKN_LISTACIUDADES *)strToken.lstCiudades );

	if ( lstTokenCiudades == (TKN_LISTACIUDADES *)strToken.lstCiudades ) /* Se volvio al PRINCIPIO de la lista */
	{
		if ( strcmp(lstTokenCiudades->info.Ciudad, sNombreCiudad) == 0 )
		{
			/* Nosotros mismos somos la Proxima Ciudad.. Por ende.. estamos nosotros solos en el torneo */

			return FALSE;
			/* Este FALSE es el que causa los mensajes "THE END?!?!?!" */
		}

	}

	/* Se envia el TOKEN al proximo ROUTER */
	if (iGameOver == FALSE)
	{
		if (
					(iFdRouter = AbreConexion(lstTokenCiudades->info.IP, lstTokenCiudades->info.Port)) > 0
					&&
					EnviaCabeceraRouter(iFdRouter, iIdMensaje, RRC_TOKEN, *iLargoToken, CABECERA_RouterRouter, sCabeceraRouter, ROUTER_ROUTER) > 0
					&&
					send(iFdRouter, *sToken, *iLargoToken, 0) > 0
				)
		{
			/* Se envio el TOKEN correctamente => Libera memoria */
			free ( *sToken );
			*sToken = NULL;

			return TRUE;
		}
		else /* No se pudo enviar el TOKEN => Se debe dar de baja esta CIUDAD (y sus Equipos) del TOKEN */
		{
			/* Elimina los Equipos que pertenecen a esa Ciudad */
			ROUTER_EliminaCiudadToken( &strToken, lstTokenCiudades->info.Ciudad );

			/* Actualiza sToken */
			*iLargoToken = AplanaToken ( strToken, sToken );

			/* LLAMADA RECURSIVA: No se pudo enviar el TOKEN al ROUTER => Se prueba con el siguiente */
			iEnvioCorrecto = ROUTER_EnviaTokenProximoRouter ( sNombreCiudad, iIdMensaje, sToken, iLargoToken, CABECERA_RouterRouter, sCabeceraRouter, iGameOver );
		}
	}
	else
	{
		if (
				  (iFdRouter = AbreConexion(lstTokenCiudades->info.IP, lstTokenCiudades->info.Port)) > 0
				  &&
				  EnviaCabeceraRouter(iFdRouter, iIdMensaje, RRC_TOKEN_END, *iLargoToken, CABECERA_RouterRouter, sCabeceraRouter, ROUTER_ROUTER) > 0
				  &&
				  send(iFdRouter, *sToken, *iLargoToken, 0) > 0
		   )
		{
			/* Se envio el TOKEN correctamente => Libera memoria */
			free ( *sToken );
			*sToken = NULL;

			return TRUE;
		}
		else /* No se pudo enviar el TOKEN => Se debe dar de baja esta CIUDAD (y sus Equipos) del TOKEN */
		{
			/* Elimina los Equipos que pertenecen a esa Ciudad */
			ROUTER_EliminaCiudadToken( &strToken, lstTokenCiudades->info.Ciudad );

			/* Actualiza sToken */
			*iLargoToken = AplanaToken ( strToken, sToken );

			/* LLAMADA RECURSIVA: No se pudo enviar el TOKEN al ROUTER => Se prueba con el siguiente */
			iEnvioCorrecto = ROUTER_EnviaTokenProximoRouter ( sNombreCiudad, iIdMensaje, sToken, iLargoToken, CABECERA_RouterRouter, sCabeceraRouter, iGameOver );
		}
	}


	return iEnvioCorrecto;
}


/*
	Envia el TOKEN hacia otro Router
*/
void ROUTER_EnviaToken	(
														char sNombreCiudad[],
														int *iFdTokenOrigen,
														FDROUTER **lstFdRouter,
														int *iIdMensaje,
														TOKEN *strToken,
														char **sToken,
														long int iLargoToken,
														CABECERA_ROUTERROUTER CABECERA_RouterRouter,
														char **sCabeceraRouter,
														int iGameOver
												)
{
	int iEnvioCorrecto;

	/* Actualiza sToken */
	iLargoToken = AplanaToken ( *strToken, sToken );

	/* Primero trata de enviarle el TOKEN a algun ROUTER que todavia no este cargado en el mismo */
	iEnvioCorrecto = ROUTER_EnviaTokenNuevoRouter	(
																										lstFdRouter,
																										iIdMensaje,
																										sToken,
																										iLargoToken,
																										CABECERA_RouterRouter,
																										sCabeceraRouter
																								);

	if ( iEnvioCorrecto == FALSE ) /* No hay ROUTERS nuevos conectados para enviarle el TOKEN */
	{
		/* Se envia el TOKEN al ROUTER de Origen (si existe) */
		if ( *iFdTokenOrigen != -1 )
		{
			EnviaCabeceraRouter(*iFdTokenOrigen, iIdMensaje, RRC_TOKEN, iLargoToken, CABECERA_RouterRouter, sCabeceraRouter, ROUTER_ROUTER);
			if ( send(*iFdTokenOrigen, *sToken, iLargoToken, 0) > 0 )
			{
				/* Se envio el TOKEN al Origen correctamente => Libera memoria */
				free ( *sToken );
				*sToken = NULL;

				*iFdTokenOrigen = -1;

				return;
			}
			else
			{
				/* Fallo el envio del TOKEN al Router Origen */
				close( *iFdTokenOrigen );
				*iFdTokenOrigen = -1;
			}

		}

		/* No hay ROUTER de Origen o fallo el envio */

		/* Trata de enviar al proximo que figure en el TOKEN */

		printf("- ROUTER: Reenviando TOKEN al siguiente Router...\n");
		sleep( 1 );

		iEnvioCorrecto = ROUTER_EnviaTokenProximoRouter	(
																												sNombreCiudad,
																												iIdMensaje,
																												sToken,
																												&iLargoToken,
																												CABECERA_RouterRouter,
																												sCabeceraRouter,
																												iGameOver
																										);

		if ( iEnvioCorrecto == FALSE ) /* THE END ?? */
		{
			/*
				Llegamos al final de la lista de Ciudades del TOKEN!
				Deberiamos de ver que se hace cuando se llega a esta situacion, si se vuelve a enviar
				desde el principio o que mierda. Para esto mismo, esta todo preparado en la funcion
				ROUTER_EnviaTokenProximoRouter, modificando en la linea 1615 de varias.c
			*/
			printf("\n\n- ROUTER: THE END ?!?!?\n\n");

			iGameOver = TRUE;
		}

		/* Se envio el TOKEN => Libera memoria */
		free ( *sToken );
		*sToken = NULL;


		return;
	}

}


/*
	Procesa la muerte de la Ciudad
*/
void ROUTER_MuereCiudad	(
														int *iFdCiudad,
														char sNombreCiudad[],
														int *iFdTokenOrigen,
														FDROUTER **lstFdRouter,
														int *iIdMensaje,
														TOKEN *strToken,
														char **sToken,
														long int iLargoToken,
														CABECERA_ROUTERROUTER CABECERA_RouterRouter,
														char **sCabeceraRouter,
														int iGameOver
												)
{
	TKN_LISTATDP *lstTokenTdp = NULL;

	if ( *iFdCiudad == -1 )
		return;

	if ( *iFdCiudad > -1 )
	{
		close( *iFdCiudad );

		printf("\n\n++++++ *** Murio la CIUDAD! Actuando como puente a partir de ahora.. *** ++++++\n\n");
	}

	*iFdCiudad = -1;

	if ( *sToken != NULL ) /* Si el TOKEN se encuentra en este Router se debe enviar */
	{
		/* Obtiene el TOKEN como esctructura */
		DesaplanaToken ( strToken, *sToken );

		/* Se busca a todos los  Equipos de la Ciudad en la TDP y se los borra */
		lstTokenTdp = (TKN_LISTATDP *) (*strToken).lstTdp;

		while ( lstTokenTdp != NULL )
		{
			if ( strcmp(lstTokenTdp->info.Ciudad,sNombreCiudad) == 0 )
			{
				(*strToken).lstTdp = (struct TKN_LISTATDP *) SuprimeNodoTokenTDP( (TKN_LISTATDP *) (*strToken).lstTdp, lstTokenTdp->info );

				lstTokenTdp = (TKN_LISTATDP *) (*strToken).lstTdp;
			}
			else
				lstTokenTdp = (TKN_LISTATDP *) lstTokenTdp->sgte;
		}

		/* Actualiza sToken */
		iLargoToken = AplanaToken ( *strToken, sToken );

		/* Envia el TOKEN hacia otro Router  */
		ROUTER_EnviaToken	(
													sNombreCiudad,
													iFdTokenOrigen,
													lstFdRouter,
													iIdMensaje,
													strToken,
													sToken,
													iLargoToken,
													CABECERA_RouterRouter,
													sCabeceraRouter,
													iGameOver
											);

	}
	else
	{
		/* Aun no tenemos el TOKEN, pero se debe dejar un FLAG para eliminar la Ciudad cuando llegue */
		*iFdCiudad = -2;
	}

}



/****************************************************************************/
/************************** R O U T E R  ::  E N D **************************/
/****************************************************************************/
