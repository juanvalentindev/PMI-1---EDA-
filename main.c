
// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

//Constantes
#define ELECTORES_ESPERADOS 2000

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

void LocalizarLSO(lso *lista, int *pos, long dni, int *exito){
  int li = 0;
  int ls = (lista->cantidad-1);
  int t;

  while (li<=ls){
    t = (li + ls)/2; //Testigo a la izquierda

    if (lista->electores[t].dni < dni){
        li = t + 1;
    }else{
        ls = t - 1;
    }
  }

  *pos = t;
  if(lista->electores[li].dni == dni){
    *exito = 1;
  }else{
    *exito = 0;
  }
}

//Alta LSO
void AltaLSO(lso *lista, long dni, int *exito){
    int pos;
    int exitoPrima;
    LocalizarLSO(lista,&pos,dni,&exitoPrima);


    if(exitoPrima == 1){
        exitoPrima == 0;

        if (lista->electores[pos].dni == dni){
            *exito = 0;
        }

    }else{
        if(lista->cantidad != ELECTORES_ESPERADOS){
            //Agregar logica de corrimiento + contador de costos
            *exito = 1;
        }else{
            *exito = 0;
        }


    }
}

//Baja LSO
void BajaLSO(){


}


int main(){
    //Definición de Estructuras
    //Lista secuencialmente ordenadas
    lso listaSecuencialOrdenada;

    return 0;

}
