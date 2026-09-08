
// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

//Constantes
#define ELECTORES_ESPERADOS 2000
#define MAS_INFINITO 999999999

static float costoTotalLSO = 0;

//=== PARA BUSCAR ESTRUCTURAS
//1. LSO
//2. LVO
//3. ABB

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
|
| OTROS
|
|   */


int CompararNombresNoCaseSensitive(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        // tolower() convierte cada caracter a minúscula en el momento de comparar
        if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) {
            return false;
        }
        str1++;
        str2++;
    }
    // Si ambas cadenas terminaron al mismo tiempo, son iguales
   if(*str1 == *str2){
    return 1;
   }else{
    return 0;
   }
}

/*
 =====================================

1. Lista Secuencialmente Ordenada (LSO)

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

    LocalizarLSO(lista, &pos, dniBuscar, exito,costoTotalLSO);

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

    LocalizarLSO(lista, &pos, nuevoElector.dni, &exitoPrima,costoTotalLSO);

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

  2. LISTA VINCULADA ORDENADA

  ===================================
*/

typedef struct Nodo{
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
    Nodo *masInfinito= (Nodo*)malloc(sizeof(Nodo));

    masInfinito=l->dato.dni = MAS_INFINITO;
    masInfinito=l->siguiente = NULL;

    l->acc = masInfinito;
    l->cur = masInfinito;
    l->aux = masInfinito;
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
    return (l.cur == NULL);
}

void FowardsLVO(LVO *l){
    l->aux = l->cur;
    l->cur = l->cur->siguiente;
}

elector CopyLVO(LVO l){
    return l.cur -> dato;
}


//----------------------//
void LocalizarLVO(LVO *lista , int dni , Nodo** pos , int *exito){

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
    }else{
        Nodo *nuevoNodo = (Nodo *)malloc(sizeof(Nodo));
        if(nuevoNodo != NULL){
            nuevoNodo->dato = nuevoDato;
            nuevoNodo->siguiente = pos;

            if (pos == lista->acc){
                nuevoNodo->siguiente = pos;
                lista->acc = nuevoNodo;
            }else{
                lista->aux->siguiente = nuevoNodo;
            }

            *exito = 1; //el alta fue exitosa

        }else{
            *exito = 0;
            return;
        }

    }
}

void BajaLVO(LVO *lista, int dniBaja,int *exito){
    Nodo *pos;
    int encontrado;
    LocalizarLVO(lista,dniBaja,&pos,&encontrado);

    if (encontrado == 0){
        *exito = 0;
        return;
    }else{
        if(pos == lista->acc){
            //Supress en la primera posición
            lista->acc=pos->siguiente;

            /*
            Supuestamente esto esta mal ?¿
            lista->acc=lista->cur->siguiente;
            free(lista->cur);
            lista->aux = lista->acc;
            lista->cur = lista->acc;*/
        }else{
            //Supress en el medio o primera posición
            lista->aux->siguiente = pos->siguiente;
            /*
            lista->cursor = lista->cursor->siguiente;
            free(lista->aux->siguiente);
            lista->aux->siguiente = lista->cur;*/
        }
    }

    free(pos);
    *exito = 1;
}

/*
  ===================================

  3. ARBOL BINARIO ORDENADO

  ===================================
*/

typedef struct NodoArbol {
    int valor;
    struct NodoArbol *hi; // Puntero al hijo izquierdo
    struct NodoArbol *hd; // Puntero al hijo derecho
} NodoArbol;


typedef struct {
    NodoArbol *raiz; // Puntero de inicio del árbol
} ABB;

void LocalizarABB(ABB *arbol, int x, NodoArbol **pos,int *exito){

    NodoArbol *p = arbol->raiz;
    NodoArbol *padre = NULL;

    while(p != NULL && p->valor != x){
        padre = p;
        if(p->valor < x){
            p = p->hd;
        }else{
            p = p->hi;
        }
    }

    if(p != NULL){
        *exito = 1;
        *pos = p;
    }else{
        *exito = 0;
        *pos = padre;
    }

}

void AltaABB(ABB *arbol, long x , int *exito){
    NodoArbol *pos;
    int encontrado;
    LocalizarABB(arbol,x,&pos,&encontrado);
    if(encontrado == 1){
        *exito = 0;
        return;
    }else{
        NodoArbol *nuevoNodo = (NodoArbol*)malloc(sizeof(NodoArbol));
        if(nuevoNodo != NULL){
            nuevoNodo->valor = x;
            nuevoNodo->hi = NULL;
            nuevoNodo->hd = NULL;
            if(pos == NULL){
                arbol->raiz = nuevoNodo;
            }else{
                if (x<pos->valor){
                    pos->hi = nuevoNodo;
                }else{
                    pos->hd = nuevoNodo;
                }
            }
            *exito = 1;
        }else{
            *exito = 0;
            return;
        }
    }
}

int CompararNuplasCompletasArbol(elector nuplaArbol, elector nuplaBajar) {
    // 1. Verificamos la parte X (Identificador)
    if (nuplaArbol.dni != nuplaBajar.dni) {return 0;}
    // 2. Verificamos la parte Y (Resto de los atributos
    if (CompararNombresNoCaseSensitive(nuplaArbol.nombre, nuplaBajar.nombre) != 0) {return 0;}
    if (CompararNombresNoCaseSensitive(nuplaArbol.apellido, nuplaBajar.apellido) != 0) {return 0;}
    if (CompararNombresNoCaseSensitive(nuplaArbol.domicilio, nuplaBajar.domicilio) != 0) {return 0;}
    // Para tipos numéricos nativos, usamos el operador tradicional
    if (nuplaArbol.codigoPostal != nuplaBajar.codigoPostal) {return 0;}
    if (nuplaArbol.numeroMesa != nuplaBajar.numeroMesa) {return 0;}
    if (nuplaArbol.circuito != nuplaBajar.circuito) {return 0;}
    return 1; // Son iguales
}


void BajaABB(ABB *arbol,long dniBuscado,int *exito ){
    NodoArbol **pos;
    int encontrado;

    LocalizarABB(arbol,dniBuscado,pos,&encontrado);

    if(encontrado == 0){
        *exito = 0;
        return;
    }else{
        //MAX en la baja del arbol es : 1.5;

    }
}

int main(){
    //Definición de Estructuras
    //Lista secuencialmente ordenadas
    lso listaSecuencialOrdenada;

    return 0;

}
