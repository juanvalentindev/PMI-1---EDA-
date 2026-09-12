
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
   long dni;                 // El D.N.I. es un entero (usamos long por el tamaño del número)
   char nombreApellido[50];  // Secuencia de hasta 50 caracteres
   char domicilio[80];       // Secuencia de hasta 80 caracteres
   int codigoPostal;         // Es un entero
   int numeroMesa;           // Es un entero
   int circuito;             // Es un entero
} elector;

/*
|
| OTROS
|
|   */


int CompararNombresNoCaseSensitive(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        // tolower() convierte cada caracter a minúscula en el momento de comparar
        if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) {
            return 0;
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

int CompararNuplas(elector e1, elector e2) {
    // Comparamos los campos numéricos con ==
    if (e1.dni == e2.dni &&
        e1.codigoPostal == e2.codigoPostal &&
        e1.numeroMesa == e2.numeroMesa &&
        e1.circuito == e2.circuito &&
        // Comparamos las cadenas ignorando mayúsculas/minúsculas
        CompararNombresNoCaseSensitive(e1.nombreApellido, e2.nombreApellido) == 1 &&
        CompararNombresNoCaseSensitive(e1.domicilio, e2.domicilio) == 1) {

        return 1; // Las nuplas son idénticas
    }

    return 0; // Hay diferencias, no son la misma persona
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

void LocalizarLSO(lso *lista, int *pos, long dni, int *exito, float *costoConsultasLSO){
    int li = 0;
    int ls = (lista->cantidad - 1);
    int t = (li + ls) / 2;

    while (li <= ls) {
        (*costoConsultasLSO)++; // Sumamos 1 por consultar la celda t

        if (lista->electores[t].dni == dni) {
            break;
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
void BajaLSO(lso *lista, elector eBorrar, int *exito, float *costoCorrimientosLSO){
    int pos;

    // 1. Buscamos por la clave X (DNI)
    LocalizarLSO(lista, &pos, eBorrar.dni, exito, costoCorrimientosLSO);

    // 2. Si se encontró el DNI, confirmamos por código comparando toda la nupla
    if (*exito == 1) {
        elector almacenado = lista->electores[pos];

        // AQUÍ INVOCAMOS A TU FUNCIÓN RECICLABLE
        if (CompararNuplas(almacenado, eBorrar) == 1) {

            // Si todo coincide, hacemos la baja con sus corrimientos
            for (int j = pos; j < lista->cantidad - 1; j++) {
                lista->electores[j] = lista->electores[j + 1];
                (*costoCorrimientosLSO)++;
            }
            lista->cantidad--;
            *exito = 1;

        } else {
            *exito = 0; // Fracasa porque el resto de la nupla no coincide
        }
    }
}
//Alta LSO
void AltaLSO(lso *lista, elector nuevoElector, int *exito, float *costoCorrimientosLSO){
    int pos;
    int exitoPrima;

    LocalizarLSO(lista, &pos, nuevoElector.dni, &exitoPrima, costoCorrimientosLSO);

    if (exitoPrima == 1) {
        *exito = 2; // Repetido
    } else if (lista->cantidad == ELECTORES_ESPERADOS) {
        *exito = 3; // Sin espacio
    } else {
        for (int j = lista->cantidad; j > pos; j--) {
            lista->electores[j] = lista->electores[j - 1];
            (*costoCorrimientosLSO)++;
        }
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

    masInfinito->dato.dni = MAS_INFINITO;
    masInfinito->siguiente = NULL;

    l->acc = masInfinito;
    l->cur = masInfinito;
    l->aux = masInfinito;
}

void ResetLVO(LVO *l){
    l->cur = l->acc;
    l->aux = l->acc;
}

int IsEmptyLVO(LVO l){
    return (l.acc->dato.dni == MAS_INFINITO);
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

void BajaLVO(LVO *lista, elector nuplaBaja,int *exito){
    Nodo *pos;
    int encontrado;

    LocalizarLVO(lista,nuplaBaja.dni,&pos,&encontrado);

    if(encontrado == 1){
        if(CompararNuplasCompletas(pos->dato,nuplaBaja) == 1){
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
            free(pos);
            *exito = 1;
        }
        *exito = 0; //El dni coincidia pero la nupla no,



    }else{
        *exito = 0;
        return;
    }

}

/*
  ===================================

  3. ARBOL BINARIO ORDENADO

  ===================================
*/

typedef struct NodoArbol {
    elector valor;
    struct NodoArbol *hi; // Puntero al hijo izquierdo
    struct NodoArbol *hd; // Puntero al hijo derecho
} NodoArbol;


typedef struct {
    NodoArbol *raiz; // Puntero de inicio del árbol
} ABB;

void LocalizarABB(ABB *arbol, int x, NodoArbol **pos,int *exito,NodoArbol **padreRetornar){

    NodoArbol *p = arbol->raiz;
    NodoArbol *padre = NULL;

    while(p != NULL && p->valor.dni != x){
        padre = p;
        if(p->valor.dni < x){
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

    if(padreRetornar !=NULL){
        *padreRetornar = padre; //Retorno el padre para poder utilizarlo en la baja
    }
}


void AltaABB(ABB *arbol, elector nuevoElector , int *exito){
    NodoArbol *pos;
    int encontrado;
    LocalizarABB(arbol,nuevoElector.dni,&pos,&encontrado,NULL);

    if(encontrado == 1){
        *exito = 0;
        return;
    }else{
        NodoArbol *nuevoNodo = (NodoArbol*)malloc(sizeof(NodoArbol));
        if(nuevoNodo != NULL){
            nuevoNodo->valor = nuevoElector;
            nuevoNodo->hi = NULL;
            nuevoNodo->hd = NULL;
            if(pos == NULL){
                arbol->raiz = nuevoNodo;
            }else{
                if (nuevoElector.dni<pos->valor.dni){
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

NodoArbol *hijoNoNULLPos(NodoArbol *nodo){
    if(nodo->hi!=NULL){
        return nodo->hi;
    }else{
        return nodo->hd;
    }
}

void BajaABB(ABB *arbol,elector electorBaja,int *exito ){
    NodoArbol *pos;
    NodoArbol *aux; //este es para el caso heavy metal (buscar el mayor de los menores)
    NodoArbol *padreAux;
    NodoArbol *padre;
    int encontrado;

    LocalizarABB(arbol,electorBaja.dni,&pos,&encontrado,&padre);

    if(encontrado == 1){ //Existe una nupla con ese x
        if(CompararNuplasCompletas(pos->valor,electorBaja)){ //Comprobamos que sea la nupla
                //Aca seria lo de modificación para la baja
                //aca empieza el kilombo :(
            //Caso 1: tengo un nodo padre con dos hijos
            if(pos->hi != NULL && pos->hd != NULL){
                 if(padre != NULL){ //NO ES LA RAIZ
                    if((pos->valor.dni) < (padre->valor.dni)){
                        padre->hi=NULL;
                    }else{
                        padre->hd=NULL;
                    }
                }else{
                    arbol->raiz = NULL; //es la raiz
                }
                free(pos);

            }else if(pos->hd == NULL || pos->hi == NULL){ //Caso 2: el nodo que queremos eliminar tiene hijos por una de sus ramas
                 if (padre != NULL){
                    if((pos->valor.dni) < (padre->valor.dni)){
                        padre->hi=hijoNoNULLPos(pos);
                    }else{
                        padre->hd=hijoNoNULLPos(pos);
                    }
                 }else{
                    arbol->raiz = hijoNoNULLPos(pos);
                 }
                 free(pos);                   //Eliminamos izquierda.
            } else { //Caso con 2 hijos, el mas heavy metal chabon
                aux = pos->hd; //Doy un paso a al derecha

                while(aux->hi != NULL){ // y bajo todo a la izquierda
                    padreAux = aux;
                    aux = aux->hi;
                }
                pos->valor = aux->valor;

                if (padreAux == pos){
                    //Aux no tenia hijos a izquierda
                    padreAux->hd = aux->hd;
                }else{
                    //Aux era un hijo izquierdo profundo por lo cual , su padre adopta a posible hijo derecho
                    padreAux->hi = aux->hd;
                }

                free(aux);


            }
            *exito = 1;

        }else{ //No es la nupla que buscamos
            *exito = 0;
            return;
        }

    }else{ //No existe nupla con ese x
        *exito = 0;
        return;
    }
}


/*
===========
Memorización
==========
*/

int memorizarDesdeArchivo(){


}


  int main() {
    // Inicializamos el árbol vacío
    ABB miArbol;
    miArbol.raiz = NULL;
    int exito;

    // --- 1. GENERACIÓN DE DATOS DE PRUEBA ---
    // Usamos DNIs pequeños y redondos para que mentalmente te sea fácil graficar el árbol.
    elector e50 = {50000000, "Ana", "Gomez", "Calle A 123", 5700, 10, 1040};
    elector e30 = {30000000, "Luis", "Perez", "Calle B 456", 5700, 10, 1040};
    elector e70 = {70000000, "Maria", "Sosa", "Calle C 789", 5700, 10, 1040};
    elector e60 = {60000000, "Juan", "Diaz", "Calle D 101", 5700, 10, 1040};
    elector e80 = {80000000, "Pedro", "Luna", "Calle E 202", 5700, 10, 1040};

    // Elector trampa: Mismo DNI que e70, pero distinto nombre.
    elector e70_falso = {70000000, "FALSO", "Sosa", "Calle C 789", 5700, 10, 1040};
    // Elector inexistente
    elector e99 = {99000000, "Nadie", "Ninguno", "Nada", 0, 0, 0};

    printf("=== INICIANDO PRUEBAS DE ABB ===\n\n");

    // --- 2. PRUEBAS DE ALTA ---
    printf("--- ALTAS ---\n");
    AltaABB(&miArbol, e50, &exito);
    printf("Alta Raiz (DNI 50M): %s\n", exito == 1 ? "EXITO" : "FALLO");

    AltaABB(&miArbol, e30, &exito);
    printf("Alta Hijo Izq (DNI 30M): %s\n", exito == 1 ? "EXITO" : "FALLO");

    AltaABB(&miArbol, e70, &exito);
    printf("Alta Hijo Der (DNI 70M): %s\n", exito == 1 ? "EXITO" : "FALLO");

    AltaABB(&miArbol, e60, &exito);
    AltaABB(&miArbol, e80, &exito);
    printf("Alta Nodos Profundos (60M y 80M): EXITO\n");

    AltaABB(&miArbol, e50, &exito); // Intento de duplicado
    printf("Alta REPETIDO (DNI 50M): %s (Esperado: FALLO)\n\n", exito == 1 ? "EXITO" : "FALLO");


    // --- 3. PRUEBAS DE BAJA ---
    printf("--- BAJAS ---\n");

    // Baja Fallida: Nupla no coincide
    BajaABB(&miArbol, e70_falso, &exito);
    printf("Baja Nupla Incorrecta (DNI 70M): %s (Esperado: FALLO)\n", exito == 1 ? "EXITO" : "FALLO");

    // Baja Fallida: No existe
    BajaABB(&miArbol, e99, &exito);
    printf("Baja Inexistente (DNI 99M): %s (Esperado: FALLO)\n", exito == 1 ? "EXITO" : "FALLO");

    // Baja Exitosa: Nodo Hoja (Sin hijos)
    BajaABB(&miArbol, e80, &exito);
    printf("Baja Nodo Hoja (DNI 80M): %s (Esperado: EXITO)\n", exito == 1 ? "EXITO" : "FALLO");

    // Baja Exitosa: Nodo con 2 hijos (Prueba la política de reemplazo)
    // El nodo 50 (Raíz) tiene a 30 por izquierda y a 70 por derecha.
    BajaABB(&miArbol, e50, &exito);
    printf("Baja Nodo 2 Hijos / Raiz (DNI 50M): %s (Esperado: EXITO)\n", exito == 1 ? "EXITO" : "FALLO");

    return 0;
}


