/*
	Funciones de Socket CLIENTE:

	Ultima modificación: 07/05/06 - 12:00
	Modificado por: Pablo
*/


int AbreConexionInet (char *Host_Servidor,char *Puerto,unsigned long *IpFirst,
		      unsigned *PuertoFirst) {

  struct sockaddr_in Direccion;
  struct hostent *Host;
  int Descriptor;
  struct in_addr HostAuxiliar;



  Host = gethostbyname (Host_Servidor);
  if (Host == NULL)
    return -1;
  if(inet_aton(Host_Servidor,&HostAuxiliar)==0)
    return -1;

  *IpFirst = ntohl(HostAuxiliar.s_addr);
  *PuertoFirst = atoi(Puerto);

  Direccion.sin_family = AF_INET;
  Direccion.sin_addr.s_addr = ((struct in_addr *)(Host->h_addr))->s_addr;
  Direccion.sin_port = htons(atoi(Puerto));
  memset(&(Direccion.sin_zero),'\0',8);

  Descriptor = socket (AF_INET, SOCK_STREAM, 0);
  if (Descriptor == -1)
    return -1;

  if (connect (Descriptor,(struct sockaddr *)&Direccion,
	       sizeof (Direccion)) == -1) {
    return -1;
  }

  return Descriptor;
}

int AbreConexion (unsigned long Ip,unsigned Puerto) {

  struct sockaddr_in Direccion;
  struct hostent *Host;
  int Descriptor;
  struct in_addr HostAuxiliar;
  char *IpPuntos=NULL;

  HostAuxiliar.s_addr = htonl(Ip);
  IpPuntos = inet_ntoa(HostAuxiliar);

  Host = gethostbyname (IpPuntos);
  if (Host == NULL)
    return -1;

  Direccion.sin_family = AF_INET;
  Direccion.sin_addr.s_addr = ((struct in_addr *)(Host->h_addr))->s_addr;
  Direccion.sin_port = htons(Puerto);
  memset(&(Direccion.sin_zero),'\0',8);

  Descriptor = socket (AF_INET, SOCK_STREAM, 0);
  if (Descriptor == -1)
    return -1;

  if (connect (Descriptor,(struct sockaddr *)&Direccion,
	       sizeof (Direccion)) == -1) {
    return -1;
  }

  return Descriptor;
}
