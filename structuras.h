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
    Particion particiones[4];  // Máximo 4
    char fit_disco;
};


#endif // STRUCTURAS_H
