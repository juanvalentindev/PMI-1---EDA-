
// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

//Constantes
#define ELECTORES_ESPERADOS 2000

//Definicion de struc's Patron
typedef struct {
   int dni; //DNI es el dato x que identifica al struct
   char[50] nombre;
   char[50] apellido;
   char[80] domicilio;
   int codigoPostal;
   int numeroMesa;
   int circuito;
} elector;

//Lista Secuencialmente Ordenada (LSO)
typedef struct{
  elector electores[ELECTORES_ESPERADOS];
  int cantidad;
} lso;


int main(){

    //Definición de Estructuras

    //Lista secuencialmente ordenadas

    lso listaSecuencialOrdenada;



    printf("Hello world!\n");
    return 0;

}
