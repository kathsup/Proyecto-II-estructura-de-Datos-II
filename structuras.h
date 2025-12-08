#ifndef STRUCTURAS_H
#define STRUCTURAS_H

#include <time.h>

struct Particion{
    char nombre[50];
    int tamanio;
    int inicio;
    char tipo;          // 'P', 'E', 'L'
    char estado;        // 'U', 'L'
    char fit;
};

struct MBR {
    int tamanio_disco;
    int num_particiones;
    Particion particiones[20];
    char fit_disco;
};

struct ParticionMontada {
    char id[10];
    char path_disco[200];
    char nombre_particion[50];
    char letra;
    int numero;
};


#endif // STRUCTURAS_H
