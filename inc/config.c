/*
	Ultima modificacion: 07/05 - 07:10
	Modificado por: Diego
*/

int LineaValida ( char * );
int CargaConfigDatos( char *, char *, CFG * );

int CargaConfig ( CFG *Config )
{
	char ConfigFile[20] = CFG_FileName;
	char *ptr;
	char linea[200 + 2];
	char campo[30 + 2];
	int CantParams = 0;
	FILE *Arch;


	/* Cantidad de parametros que debe contener el archivo CFG */
	Config->CantParams = CFG_CantParams;

	if ( (Arch = fopen(ConfigFile,"rt")) == NULL ) {
		printf( "ERROR: No se puede abrir el archivo de configuracion %s\n", ConfigFile );
		return FAIL;
	}

	ptr = malloc ( 200 + 2 );

	while ( fgets(linea,200+2,Arch) != NULL )
	{
		if( LineaValida(linea) )
		{
			ptr = strtok( linea, "=" );
			if ( ptr == NULL ) {
				printf( "ERROR: El archivo de configuracion no tiene un formato valido\n" );
				fclose( Arch );
				return FAIL;
			}
			strcpy( campo, ptr );
			ptr = strtok( NULL, ";" );
			if ( ptr == NULL ) {
				printf( "ERROR: El archivo de configuracion no tiene un formato valido\n" );
				fclose( Arch );
				return FAIL;
			}
			if ( CargaConfigDatos( campo, ptr, Config) == OK ) {
				CantParams++;
			}
		}
	}

	if ( CantParams == Config->CantParams ) { /* Se cargaron todos los campos de la estructura */
		return OK;
	}
	/* No se cargaron todos los campos */
	printf( "ERROR: Falta(n) %d parametro(s) de configuracion en el archivo\n", Config->CantParams-CantParams );
	return FAIL;
}

int LineaValida ( char *linea )
{
	char CommentChar = '#';
	char line[200+2];
	/*char *ptr;*/

	/* Se planteo asi para futuras implementaciones de mejorias, por ahora no es necesario */
	strcpy( line, linea ); /* Guarda una copia de la cadena original */

	if ( line[0] == CommentChar ) { return FALSE; }
	if ( ! isalpha(line[0]) ) { return FALSE; }

	return TRUE;
}

int CargaConfigDatos( char *campo, char *valor, CFG *Config )
{
	struct in_addr strAddr;

	if ( strcmp(campo,"CantMaxEquipos") == 0 ) {
		Config->CantMaxEquipos = atoi(valor);
	}
	else if ( strcmp(campo,"DuracionPartidosMIN") == 0 ) {
		Config->DuracionPartidosMIN = atoi(valor);
	}
	else if ( strcmp(campo,"DuracionPartidosMAX") == 0 ) {
		Config->DuracionPartidosMAX = atoi(valor);
	}
	else if ( strcmp(campo,"TiempoInactividad") == 0 ) {
		Config->TiempoInactividad = atoi(valor);
	}
	else if ( strcmp(campo,"CansancioMAX") == 0 ) {
		Config->CansancioMAX = atoi(valor);
	}
	else if ( strcmp(campo,"EquiposCdeMAX") == 0 ) {
		Config->EquiposCdeMAX = atoi(valor);
	}
	else if ( strcmp(campo,"TiempoCdeMIN") == 0 ) {
		Config->TiempoCdeMIN = atoi(valor);
	}
	else if ( strcmp(campo,"TiempoCdeMAX") == 0 ) {
		Config->TiempoCdeMAX = atoi(valor);
	}
	else if ( strcmp(campo,"RouterLocalIP") == 0 ) {
		inet_aton( valor, &strAddr );
		Config->RouterLocalIP = htonl( strAddr.s_addr );
		sprintf( Config->CiudadLocalIP, "%s", valor ); /* Se guarda la IP sin convertir */
	}
	else if ( strcmp(campo,"RouterLocalPort") == 0 ) {
		Config->RouterLocalPort = atoi(valor);
	}
	else if ( strcmp(campo,"RouterRemoteIP") == 0 ) {
		inet_aton( valor, &strAddr );
		Config->RouterRemoteIP = htonl( strAddr.s_addr );
	}
	else if ( strcmp(campo,"RouterRemotePort") == 0 ) {
		Config->RouterRemotePort = atoi(valor);
	}
	else if ( strcmp(campo,"DiscoverTimeout") == 0 ) {
		Config->DiscoverTimeout = atoi(valor);
	}
	else if ( strcmp(campo,"MigracionTimeout") == 0 ) {
		Config->MigracionTimeout = atoi(valor);
	}
	else if ( strcmp(campo,"TTL") == 0 ) {
		Config->TTL = (char) atoi(valor);
	}
	else {
		printf( "WARNING: Campo no reconocido en archivo de configuracion: %s\n", campo );
		return FAIL;
	}

	return OK;
}
