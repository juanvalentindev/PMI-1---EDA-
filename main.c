
// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

//Constantes
#define ELECTORES_ESPERADOS 2000
#define MAS_INFINITO 999999999

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


/*
 =====================================
Lista Secuencialmente Ordenada (LSO)
    ================================
*/

typedef struct{
  elector electores[ELECTORES_ESPERADOS];
  int cantidad;
} lso;


//Localizar LSO
/*
.Directrices
|limite inferior inclusivo, l ́ımi-
.te superior inclusivo, testigo a izquierda y segmento mas grande a la izquierda.
*/

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

/*
  ===================================

  LISTA VINCULADA ORDENADA

  ===================================
*/

typedef struct {
    elector dato;
    struct Nodo* siguiente;
}Nodo;


typedef struct {
    Nodo *acc;
    Nodo *cur;
    Nodo *aux;
}LVO;

/*
°Operaciones de la lista
|   Vinculada ordenada
*/
void InitLVO(LVO *l){
    Nodo *centinela = (Nodo*)malloc(sizeof(Nodo));

    centinela->dato.dni = MAS_INFINITO
    centinela.siguiente = NULL;

    l->acc = centinela;
    l->cur = centinela;
    l->aux = centinela;
}

void ResetLVO(LVO *l){
    l->cur = l->acc;
    l->aux = l->acc;
}

int IsEmptyLVO(LVO l){
    return (l.acc == NULL);
}

int IsFullLVO(){
    Nodo *n = (Nodo *)malloc(sizeof(Nodo));
    if(n == NULL) return 1;
    free(n);
    return 0;
}

int isOosLVO(LVO l) {
    return (l.cur == NULL)
}

void FowardsLVO(LVO *l){
    l-> aux = l-> cur;
    l->cur = l->cur->siguiente;
}

elector CopyLVO(LVO l){
    return l.cur -> dato;
}


//----------------------//
void LocalizarLVO(LVO *lista , int dni , Nodo** pos , int *exito{

    ResetLVO(lista);

    while( lista->cur->dato.dni < dni){
        FowardsLVO(lista);
    }

    *pos = lista->cur;

    if(lista->cur->dato.dni == dni){
        *exito = 1;
    }else{
        *exito = 0;
    }
}

void AltaLVO(LVO *lista,elector nuevoDato, int *exito){
    Nodo *pos;
    int encontrado; //Seria el exito prima de los apuntes

    LocalizarLVO(lista,nuevoDato.dni,&pos,&encontrado);

    if (encontrado == 1){
        *exito = 0;
        return;
    }

    Nodo *nuevoNodo = (Nodo *)malloc(sizeof(Nodo));
    if(nuevoNodo == 0){
        *exito = 0;
        return;
    }

    nuevoNodo.dato = nuevoDato;
    nuevoNodo->siguiente = pos;

    if (pos == lista->acc){
        lista->acc = nuevoNodo;
    }else{
        lista->aux->siguiente = nuevoNodo;
    }


    //Aplicar insertar ordenado.
    //L
    if(lista->cur == lista->acc){
        lista->acc = nuevoNodo;
    }else{
        lista->aux->siguiente = nuevoNodo;
    }
    *exito = 1; //el alta fue exitosa

}

void


int main(){
    //Definición de Estructuras
    //Lista secuencialmente ordenadas
    lso listaSecuencialOrdenada;

    return 0;

}
