/*
	Ultima modificación: 11/05 - 02:40
	Modificación por: Diego
*/



int WriteLog ( int TipoLog, char *ProcName, const char Text[200] )
{
	char LogFile[20] = LOG_FileName;
	char TypeLog[20];
	char output[250];
	time_t rawtime;
	struct tm * Date;
	char sDate[20];
	FILE *Arch;

	if ( (Arch = fopen(LogFile,"aw")) == NULL ) {
		printf( "ERROR: No se puede abrir el archivo de log %s\n", LogFile );
		return FAIL;
	}

	switch ( TipoLog ) /* Obtiene el tipo de mensaje a guardarse */
	{
		case INFO:
			sprintf( TypeLog, "Info" );
		break;
		case WARNING:
			sprintf( TypeLog, "Warning" );
		break;
		case ERROR:
			sprintf( TypeLog, "Error" );
		break;
	}

	/* Obtiene los datos actuales de fecha y hora */
	time ( &rawtime );
	Date = localtime ( &rawtime );
	/* Guarda la fecha y la hora con formato */
	sprintf( sDate, "%d/%02d/%02d-%02d:%02d:%02d",
					 Date->tm_year+1900,Date->tm_mon+1,Date->tm_mday,Date->tm_hour,Date->tm_min,Date->tm_sec );

	/* Arma la cadena para ser escrita al archivo de LOG */
	sprintf( output, "%s %s[%d]: %s: %s\n", sDate, ProcName, getpid(), TypeLog, Text );

	/* Escribe los datos en el archivo */
	fprintf( Arch, "%s", output );

	fclose (Arch);

	return OK;
}
