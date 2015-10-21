/*
	Ultima modificacion: ??/?? - ??:??
	Modificado por: Lucas
*/

int AbreSocket (int Puerto) {
  int Descriptor;
  struct sockaddr_in Direccion;
  int yes;

  if ((Descriptor = socket(AF_INET,SOCK_STREAM,0)) == -1)
    return -1;

  if(setsockopt(Descriptor,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1) {
	close(Descriptor);
    return -1;
	}

  Direccion.sin_family = AF_INET;
  Direccion.sin_port = htons (Puerto);
  Direccion.sin_addr.s_addr = INADDR_ANY;
  memset(&(Direccion.sin_zero),'\0',8);

  if (bind(Descriptor,(struct sockaddr *)&Direccion,sizeof(Direccion)) == -1) {
    close(Descriptor);
    return -1;
  }

  if (listen(Descriptor,10) == -1) {
    close(Descriptor);
    return -1;
  }

  return Descriptor;
}

int AceptaConexion (int Descriptor,unsigned long *IpCliente,unsigned *PuertoCliente) {
  struct sockaddr_in Cliente;
  socklen_t LongitudCliente;
  int Hijo;
  struct in_addr HostLong;

  LongitudCliente = sizeof(struct sockaddr_in);

  if ((Hijo = accept(Descriptor,(struct sockaddr *)&Cliente,&LongitudCliente)) == -1) {
    return -1;
  }

  if((IpCliente != NULL)&&(PuertoCliente != NULL)) {
    HostLong = Cliente.sin_addr;
    *IpCliente = HostLong.s_addr;
    *PuertoCliente = htons(Cliente.sin_port);
  }

  return Hijo;
}
