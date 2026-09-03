
// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

//Constantes
#define ELECTORES_ESPERADOS 2000

static float costoTotalLSO = 0;

//Definicion de struc's Patron
typedef struct {
   long dni; //DNI es el dato x que identifica al struct
   char nombre [50];
   char apellido [50] ;
   char domicilio[80];
   int codigoPostal;
   int numeroMesa;
   int circuito;
}elector;

//Lista Secuencialmente Ordenada (LSO)
typedef struct{
  elector electores[ELECTORES_ESPERADOS];
  int cantidad;
} lso;


//Localizar LSO
/*Directrices
limite inferior inclusivo, l ́ımi-
te superior inclusivo, testigo a izquierda y segmento mas grande a la izquierda.*/

void LocalizarLSO(lso *lista, int *pos, long dni, int *exito, float *costoTotalLSO){
    int li = 0;
    int ls = (lista->cantidad - 1);
    int t = (li + ls) / 2;

    while (li <= ls) {
        (*costoTotalLSO)++; //Costo por consultar la celda t
        if (lista->electores[t].dni == dni) {
            break; //Si esta, corta
        }

        if (lista->electores[t].dni < dni){
            li = t + 1;
        } else {
            ls = t - 1;
        }
        t = (li + ls) / 2;
    }
    if (li <= ls) {
        *exito = 1;
        *pos = t;
    } else {
        *exito = 0;
        *pos = li;
    }
}
//Baja LSO
void BajaLSO(lso *lista, long dniBuscar, int *exito, float *costoTotalLSO){
    int pos;
    LocalizarLSO(lista, &pos, dniBuscar, exito);

    if (*exito == 1) {
        for (int j = pos; j < lista->cantidad - 1; j++) {
            lista->electores[j] = lista->electores[j + 1];
            (*costoTotalLSO)++;
        }
        lista->cantidad--;
    }
}
//Alta LSO
void AltaLSO(lso *lista, elector nuevoElector, int *exito, float *costoTotalLSO){
    int pos;
    int exitoPrima;

    LocalizarLSO(lista, &pos, nuevoElector.dni, &exitoPrima);

    if (exitoPrima == 1) {
        *exito = 2; //Fracasa por elemento repetido

    } else if (lista->cantidad == ELECTORES_ESPERADOS) {
        *exito = 3; //Fracasa por falta de espacio

    } else {

        for (int j = lista->cantidad; j > pos; j--) {
            lista->electores[j] = lista->electores[j - 1];
            (*costoTotalLSO)++; // Sumamos 1 al costo por cada movimiento
        }

        // Insertamos la nupla completa en la posición liberada
        lista->electores[pos] = nuevoElector;
        lista->cantidad++;
        *exito = 1;
    }
}

int main(){
    //Definición de Estructuras
    //Lista secuencialmente ordenadas
    lso listaSecuencialOrdenada;

    return 0;

}
