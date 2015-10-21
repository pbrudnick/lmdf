/*
	Ultima modificacion: 21/06 - 03:45
	Modificado por: El "Beto" Marcico
*/


/* ###### T O K E N ###### */
#ifndef STRUCT_TOKEN
#define STRUCT_TOKEN

/* Lista de Ciudades */
typedef struct {
	int Orden;
	char Ciudad[30];
	unsigned long IP;
	int Port;
} TKN_LISTACIUDADESinfo;
typedef struct {
	TKN_LISTACIUDADESinfo info;
	struct TKN_LISTACIUDADES *sgte;
} TKN_LISTACIUDADES;

/* Lista Tabla de Posiciones */
typedef struct {
	char Ciudad[30];
	char Equipo[30];
	int Puntaje;
	int PartidosGanados;
	int PartidosEmpatados;
	int PartidosPerdidos;
} TKN_LISTATDPinfo;
typedef struct {
	TKN_LISTATDPinfo info;
	struct TKN_LISTATDP *sgte;
} TKN_LISTATDP;

/* Estructura PRINCIPAL */
typedef struct {
	long int IdJuego;
	struct TKN_LISTACIUDADES *lstCiudades;
	struct TKN_LISTATDP *lstTdp;
} TOKEN;

#endif


/* ###### CABECERA CIUDAD-ROUTER / ROUTER-ROUTER ###### */
#ifndef STRUCT_CABECERA_ROUTERROUTER
#define STRUCT_CABECERA_ROUTERROUTER

typedef struct {
	int IdMensaje;
	char TipoMensaje;
	char TTL;
	char Hops;
	int LargoMensaje;
} CABECERA_ROUTERROUTER;

#endif


/* ###### CABECERA CIUDAD-EQUIPO ###### */
#ifndef STRUCT_CABECERA_CIUDADEQUIPO
#define STRUCT_CABECERA_CIUDADEQUIPO

typedef struct {
	unsigned long IdMensaje;
	char TipoMensaje;
	unsigned long LargoMensaje;
} CABECERA_CIUDADEQUIPO;

#endif


/* ###### ESTADOS ###### */
#ifndef ENUM_ESTADOS
#define ENUM_ESTADOS

enum Estados
{
	INACTIVO, JUGANDO, ENTRENANDO, DESCANSANDO, MUERTO, MIGRADO
};

#endif


/* ###### RESULTADOS ###### */
#ifndef ENUM_RESULTADOS
#define ENUM_RESULTADOS
/*
	La cantidad de elementos DEBE ser menor a 10,
	sino hay bardo con EncriptaLevResultados y DesncriptaLevResultados
*/
enum Resultados
{
	DERROTA, VICTORIA, EMPATE, NO_JUGADO, EQUIPO_KILLED
};

#endif


/* ###### ROUTER ###### */
#ifndef STRUCT_FDROUTER
#define STRUCT_FDROUTER

typedef struct {
	int FdRouter;
	int TokenGive;	/* Si nos pide el envio del TOKEN para que cargue su Ciudad */
} FDROUTERinfo;

typedef struct {
	FDROUTERinfo info;
	struct FDROUTER *sgte;
} FDROUTER;

#endif


/* ###### LEV ###### */
#ifndef STRUCT_LEV
#define STRUCT_LEV

typedef struct {
	char Equipo[30];
	char CiudadOrigen[30];
} LEVinfo;

typedef struct {
	LEVinfo info;
	struct LEV *sgte;
} LEV;

#endif


/* ###### FD ###### */
#ifndef STRUCT_FDS
#define STRUCT_FDS

typedef struct {
	char Equipo[30];
	int FdEquipo;
	int Estado;
	int Port;
	time_t Timeout; /* Timeout de migracion */

	int LevSent;
	int LevSentExt;
	int TokenSet;
	int Migrado; /* Flag para saber si el equipo es extranjero o no */

	int Puntaje;

	/* char Migrado; */ /* IMPORTANTE: Para tener en cuenta..
												Si lo definimos aca Ciudad tira SEGMENTATION FAULT.. Pueden creerlo?? */

} FDSinfo;
typedef struct {
	FDSinfo info;
	struct FDS *sgte;
} FDS;

#endif


/* ###### LEV RESULTADOS ###### */
#ifndef STRUCT_LEV_RESULTADOS
#define STRUCT_LEV_RESULTADOS

typedef struct {
	char Resultado;
	char Equipo[30];
	char CiudadOrigen[30];
} LEVRESULTADOSinfo;
typedef struct {
	LEVRESULTADOSinfo info;
	struct LEVRESULTADOS *sgte;
} LEVRESULTADOS;

#endif

/* ###### CDE EQUIPOS ###### */
#ifndef STRUCT_CDE_EQUIPOS
#define STRUCT_CDE_EQUIPOS

typedef struct {
	int FdEquipo;
	char Equipo[30];
	int TiempoPedido;
	time_t TiempoInicio;
} CDEEQUIPOSinfo;
typedef struct {
	CDEEQUIPOSinfo info;
	struct CDEEQUIPOS *sgte;
} CDEEQUIPOS;

#endif

#ifndef STRUCT_CDE_COLAEQUIPOS
#define STRUCT_CDE_COLAEQUIPOS

typedef struct {
	int FdEquipo;
	char Equipo[30];
	int TiempoPedido;
} CDECOLAEQUIPOSinfo;
typedef struct {
	CDECOLAEQUIPOSinfo info;
	struct CDECOLAEQUIPOS *sgte;
} CDECOLAEQUIPOS;

#endif


/* ###### TABLA PUNTAJES ###### */
#ifndef STRUCT_TABLA_RESULTADOS
#define STRUCT_TABLA_RESULTADOS
/*
typedef struct {
	char Puntaje;
	char Equipo[30];
} TABLAPUNTAJESinfo;
typedef struct {
	TABLAPUNTAJESinfo info;
	struct TABLAPUNTAJES *sgte;
} TABLAPUNTAJES;
*/
#endif


/* ###### TIPOS DE MENSAJES ###### */
#ifndef STRUCT_TYPES
#define STRUCT_TYPES

/* @@@@ TIPOS: BUSCA EQUIPO (0, 1 y 2) @@@@ */
/*
	Tipo 0:
	Equipo -> Ciudad
	El equipo pasa contra quien quiere jugar y en que condición (Local o Visita)
*/
typedef struct {
	char Local;
	char Equipo[30];
} TIPO_0;
/*
	Tipo 1:
	Ciudad -> Equipo
	La ciudad le envía al equipo la dirección de la Ciudad donde reside el Equipo
	Si Local = 1 es local.. si es = 0, es visitante/extranjero
*/
typedef struct {
	char Local;
	unsigned long IP;
	unsigned int Port;
} TIPO_1;
/*
	Tipo 2:
	Ciudad -> Equipo
	Nada, el equipo no se encontró
*/


/* @@@@ TIPOS: PIDE CDE (3, 4 y 5) @@@@ */
/*
	Tipo 3:
	Equipo -> Ciudad
	El equipo le pide el tiempo que desea usar el CDE a la ciudad
*/

/*
	Tipo 4:
	Ciudad -> Equipo
	La ciudad le envía la dirección del CDE al equipo
*/
typedef struct {
	int Estado;
	unsigned long IP;
	unsigned int Port;
} TIPO_4;

typedef struct {
	int Estado;
	char Equipo[30];
	int TiempoPedido;
} EQUIPO_CDE;
/*
	Tipo 5:
	Ciudad -> Equipo
	Nada, el CDE no puede ser utilizado
*/


/* @@@@ TIPOS: PIDE JUGAR CONTRA EQUIPO (6, 7 y 8) @@@@ */
/*
	Tipo 6:
	Equipo -> Ciudad
	El equipo le dice a la Ciudad contra que equipo quiere jugar
*/

/*
	Tipo 7:
	Ciudad -> Equipo
	La ciudad le envía la dirección del Equipo
*/
typedef struct {
	unsigned long IP;
	unsigned int Port;
} TIPO_7;
/*
	Tipo 8:
	Ciudad -> Equipo
	La ciudad NO (no puede jugar)
*/


/* @@@@ TIPOS: ENVIO DE LEV (9 y 10) @@@@ */
/*
	Tipo 9:
	Ciudad -> Equipo
	La ciudad le envía la LEV al equipo.
			Se trata como cadena concatenada
*/

/*
	Tipo 10:
	Equipo -> Ciudad
	El equipo le envía sus resultados a la Ciudad.
			Se trata como cadena concatenada con byte delante de cada nombre
*/


/* @@@@ TIPOS: CREACION O MIGRACION DE EQUIPO (11 y 12) @@@@ */
/*
	Tipo 11:
	Equipo -> Ciudad
	Main -> Ciudad
*/
typedef struct {
	char KI[3+1];
	char Cansancio[3+1];
	char Nuevo;
	char DuracionPartidos[5+1];
	char Equipo[30];
} TIPO_11;
/*
	Tipo 12:
	Ciudad -> Equipo
	Ciudad -> Main
	Confirmación de que se crea o migra
*/


/* @@@@ TIPOS: ACTUALIZA ESTADOS (13 y 14) @@@@ */
/*
	Tipo 13:
	Equipo -> Ciudad
	Le pasa el estado que quiere actualizar:
			0 = inactivo, 1 = jugando, 2 = entrenando
*/

/*
	Tipo 14:
	Ciudad -> Equipo
	Confirma que se actualiza el estado de la lista de estado de equipos en Ciudad
*/


/* @@@@ TIPOS: CREACION CIUDAD (15 y 16) @@@@ */
/*
	Tipo 15:
	Main -> Ciudad
	Envia el nombre de la Ciudad para crear
	NO SE USA, PERO POR LAS DUDAS QUEDA
*/

/*
	Tipo 16:
	Ciudad -> Main
	Ciudad creada OK
*/

typedef struct {
	char Ciudad[30];
	char Equipo[30];
} DISCOVER;

/* Respuesta del DISCOVER */
typedef struct {
	unsigned long IPCiudad;
	unsigned int PortCiudad;
} FOUND;
/*
###### SOLICITUD IP AL EQUIPO ######
Tipo 17:
Equipo -> Ciudad
El equipo le indica a la ciudad que termino todos sus partidos.
le devuelve un tipo "resultados generados" que todavia no esta hecho

###### FIN SOLICITUD IP AL EQUIPO ######

###### MUESTRA RESULTADOS EN PANTALLA CIUDAD ######
Tipo 18:
Ciudad -> Main
La ciudad le pasa a main los resultados para mostrar en pantalla

se envia un string formateado (no definido aun)

###### FIN MUESTRA RESULTADOS EN PANTALLA CIUDAD ######
*/

#endif


/* ###### LISTA DE MENSAJES DISCOVER ENVIADOS CON TIMEOUT ###### */
#ifndef STRUCT_LST_DISCOVERSENT
#define STRUCT_LST_DISCOVERSENT

typedef struct {
	time_t Timeout;
	int FdOrigen;
	int IdMensaje;
} DISCOVERSENTinfo;
typedef struct {
	DISCOVERSENTinfo info;
	struct DISCOVERSENT *sgte;
} DISCOVERSENT;

#endif


/* ###### ENUM DE MENSAJES ###### */
#ifndef ENUM_MENSAJES
#define ENUM_MENSAJES

/*
	Los mensajes se dividen por las secciones ya definidas..
	Secciones:

	BE = Buscar Equipo
	CDE = Campo de Entrenamiento
	JE = Jugar con Equipo
	LEV = LEV
	CME = Creacion / Modificacon de Equipo
	AE = Actualización de Estado
	CC = Creacion Ciudad
*/
enum Mensajes {
	BE_BUSCAR,
	BE_DIRECCION_CIUDAD,
	BE_NO,
	CDE_PEDIR,
	CDE_DIRECCION,
	CDE_NO,
	JE_JUGAR_CON,
	JE_DIRECCION_EQUIPO,
	JE_NO,
	LEV_ENVIAR,
	LEV_RESULTADOS,
	CME_EQUIPO,
	CME_CONFIRMACION,
	AE_ESTADO,
	AE_CONFIRMACION,
	CC_CREAR,
	CC_CONFIRMACION,
	BE_GET_IP,
	JE_TERMINO_PARTIDOS,
	JE_RESULTADOS_PANTALLA,
	TKN_NEW,
	TKN_JOIN,
	TKN_END,
	BE_CDE_MUERTO
};

#endif


/* ###### ENUM DE MENSAJES ROUTER-CIUDAD / ROUTER-ROUTER ###### */
#ifndef ENUM_MENSAJES_ROUTERROUTER
#define ENUM_MENSAJES_ROUTERROUTER

/*
	Los mensajes se dividen por las secciones ya definidas..
	Secciones:

	BE = Buscar Equipo
	CR = Ciudad-Router
	RRC = Router-Router / Router - Ciudad
*/
enum MensajesRouter {
	BE_DISCOVER,
	BE_FOUND,
	BE_NOT,
	CR_CONECTA,
	RRC_TOKEN_GIVE,
	RRC_TOKEN_END,
	RRC_TOKEN,
	RRC_END
};

#endif


/* ###### ARCHIVO DE CONFIGURACION ###### */
#ifndef STRUCT_CFG
#define STRUCT_CFG

typedef struct {
	int CantMaxEquipos;
	int DuracionPartidosMIN;
	int DuracionPartidosMAX;
	int TiempoInactividad;
	int CansancioMAX;
	int EquiposCdeMAX;
	int TiempoCdeMIN;
	int TiempoCdeMAX;
	unsigned long RouterRemoteIP;
	int RouterRemotePort;
	unsigned long RouterLocalIP;
	char CiudadLocalIP[20]; /* Este no se debe contar como un parametro mas, es igual que "RouterLocalIP" pero sin convertir */
	int RouterLocalPort;
	int DiscoverTimeout;
	int MigracionTimeout;
	char TTL;

	int CantParams;
} CFG;
/*
Cantidad de parametros que contiene esta estructura
..entonces..
Cantidad de parametros que se deben leer del CFG obligatoriamente
*/
#define CFG_CantParams 15
#define CFG_FileName "./../config.cfg"

#endif


/* ###### ARCHIVO DE LOG ###### */
#ifndef LOG_Parameters
#define LOG_Parameters

enum LOG_tipos {
	INFO,
	WARNING,
	ERROR
};

#define LOG_FileName "./../logfile.log"

#endif


/* ###### DEFINICIONES GENERALES ###### */
#ifndef DEFINES
#define DEFINES

/* Retornos y estados estandar */
#define OK 0
#define FAIL -1
#define TRUE 1
#define FALSE 0
#define ENVIO_RESULTADOS 2
#define TERMINADO 3

/* Resultados del ingreso de comandos */
#define EXIT 0
#define HELP 1
#define NEW 2
#define JOIN 3
#define ADDCITY 4
#define ADDTEAM 5
#define STARTGAME 6
#define ENDGAME 7

/*
	Colores y demases
*/
#define TXT_RESET           0
#define TXT_BRIGHT          1
#define TXT_DIM             2
#define TXT_UNDERLINE       3
#define TXT_BLINK           4
#define TXT_REVERSE         7
#define TXT_HIDDEN          8
#define TXT_BLACK           0
#define TXT_RED             1
#define TXT_GREEN           2
#define TXT_YELLOW          3
#define TXT_BLUE            4
#define TXT_MAGENTA         5
#define TXT_CYAN            6
#define TXT_WHITE           7

/* Logitud maxima del string de un comando */
#define COMLEN 80
/* Tamaños de buffers para leer y escribir en los sockets */
#define BUFFLEN 80

/* FDs de ESCUCHA */
#define FD_MAIN 0
#define FD_ROUTER 2
#define FD_CIUDAD 2
#define FD_CDE 2

#define ROUTER_ROUTER 1
#define ROUTER_CIUDAD 0

#define ES_LEV_INTERNA 0
#define ES_LEV_EXTERNA 1

#define LARGO_CABECERA_ROUTER 7

#endif
