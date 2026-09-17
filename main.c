
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

//Por si quiero crear variables Globales
elector bancoElectores[ELECTORES_ESPERADOS];
int marcas[ELECTORES_ESPERADOS]; // 1 = Está en la estructura | 0 = No está (o fue dado de baja)
int totalHistorico = 0;

typedef struct {
    int cantidad;
    float costo_acu;
    float costo_max;
} MetricasOp;

typedef struct {
    MetricasOp alta;
    MetricasOp baja;
    MetricasOp evocar_exito;
    MetricasOp evocar_fracaso;
} Estadisticas;

// Función para inicializar los contadores en cero
void InicializarMetricas(MetricasOp *m) {
    m->cantidad = 0;
    m->costo_acu = 0.0;
    m->costo_max = 0.0;
}

// Función para registrar el costo de UNA operación individual
void RegistrarCosto(MetricasOp *m, float costo_individual) {
    m->cantidad++;
    m->costo_acu += costo_individual;
    if (costo_individual > m->costo_max) {
        m->costo_max = costo_individual; // Actualizamos el peor caso (Máximo)
    }
}



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

void EvocarLSO(lso *lista, long dniBuscar, elector *eRecuperado, int *exito, float *costoConsultasLSO) {
    int pos;

    LocalizarLSO(lista, &pos, dniBuscar, exito, costoConsultasLSO);
    if (*exito == 1) {
        *eRecuperado = lista->electores[pos];
    }
}

// Baja LSO
void BajaLSO(lso *lista, elector eBorrar, int *exito, float *costoCorrimientosLSO){
    int pos;
    float costoConsultaInterna = 0; // Variable auxiliar descartable

    // 1. Buscamos por DNI sin alterar el acumulador de corrimientos
    LocalizarLSO(lista, &pos, eBorrar.dni, exito, &costoConsultaInterna);

    // 2. Confirmamos por código comparando toda la nupla
    if (*exito == 1) {
        elector almacenado = lista->electores[pos];

        if (CompararNuplas(almacenado, eBorrar) == 1) {
            for (int j = pos; j < lista->cantidad - 1; j++) {
                lista->electores[j] = lista->electores[j + 1];
                (*costoCorrimientosLSO)++;
            }
            lista->cantidad--;
            *exito = 1;
        } else {
            *exito = 0; // Fracasa: la nupla no coincide
        }
    }
}

// Alta LSO corregida
void AltaLSO(lso *lista, elector nuevoElector, int *exito, float *costoCorrimientosLSO){
    int pos;
    int exitoPrima;
    float costoConsultaInterna = 0; // Variable auxiliar descartable

    LocalizarLSO(lista, &pos, nuevoElector.dni, &exitoPrima, &costoConsultaInterna);

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
//Funcion auxiliar de LSO para obtener los costos de Evocar
void EvaluarCostosEvocacionLSO(lso *lista, elector banco[], int marcas[], int total) {
    float costoTotalExito = 0;
    float costoMaximoExito = 0;
    int cantExitos = 0;

    float costoTotalFracaso = 0;
    float costoMaximoFracaso = 0;
    int cantFracasos = 0;

    for (int i = 0; i < total; i++) {
        int pos;
        int exito;
        float costoConsulta = 0;

        // Invocamos a la localización midiendo el costo individual de esta consulta
        LocalizarLSO(lista, &pos, banco[i].dni, &exito, &costoConsulta);

        if (marcas[i] == 1) {
            // Caso: Evocación Exitosa (debería estar en la estructura)
            costoTotalExito += costoConsulta;
            if (costoConsulta > costoMaximoExito) {
                costoMaximoExito = costoConsulta;
            }
            cantExitos++;
        } else {
            // Caso: Evocación No Exitosa (fue dado de baja o rechazado)
            costoTotalFracaso += costoConsulta;
            if (costoConsulta > costoMaximoFracaso) {
                costoMaximoFracaso = costoConsulta;
            }
            cantFracasos++;
        }
    }

    printf("\n--- METRICAS DE CONSULTA A POSTERIORI (LSO) ---\n");
    if (cantExitos > 0) {
        printf("Evocacion Exitosa:\n");
        printf("  - Costo Medio (Esperado): %.2f celdas\n", costoTotalExito / cantExitos);
        printf("  - Peor Escenario (Maximo): %.2f celdas\n", costoMaximoExito);
    }
    if (cantFracasos > 0) {
        printf("Evocacion No Exitosa (Fracaso):\n");
        printf("  - Costo Medio (Esperado): %.2f celdas\n", costoTotalFracaso / cantFracasos);
        printf("  - Peor Escenario (Maximo): %.2f celdas\n", costoMaximoFracaso);
    }
}

void MostrarEstructuraLSO(lso *lista) {
    if (lista->cantidad == 0) {
        printf("\nLa estructura LSO se encuentra vacia.\n");
        return;
    }

    printf("\n=== PADRON DE ELECTORES (LSO) - Total: %d ===\n", lista->cantidad);
    for (int i = 0; i < lista->cantidad; i++) {
        printf("[%d] DNI: %ld | %s | %s | CP: %d | Mesa: %d | Circuito: %d\n",
               i + 1,
               lista->electores[i].dni,
               lista->electores[i].nombreApellido,
               lista->electores[i].domicilio,
               lista->electores[i].codigoPostal,
               lista->electores[i].numeroMesa,
               lista->electores[i].circuito);

        // Paginado cada 20 registros
        if ((i + 1) % 20 == 0 && (i + 1) < lista->cantidad) {
            printf("\n--- Presione ENTER para ver los siguientes electores ---");
            while (getchar() != '\n'); // Limpia el buffer y espera un Enter
        }
    }
    printf("==============================================\n");
}

void ActualizarMarca(long dni, elector e, int operacion, int exito) {
    // Si la operación fracasó, no afecta las marcas de pertenencia
    if (exito != 1) return;

    if (operacion == 1) { // Alta exitosa
        // Buscar si ya existía en el historial
        for (int i = 0; i < totalHistorico; i++) {
            if (bancoElectores[i].dni == dni) {
                marcas[i] = 1;
                return;
            }
        }
        // Si es nuevo, lo agregamos al banco
        if (totalHistorico < ELECTORES_ESPERADOS) {
            bancoElectores[totalHistorico] = e;
            marcas[totalHistorico] = 1;
            totalHistorico++;
        }
    } else if (operacion == 2) { // Baja exitosa
        for (int i = 0; i < totalHistorico; i++) {
            if (bancoElectores[i].dni == dni) {
                marcas[i] = 0; // Desmarcado
                return;
            }
        }
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
void LocalizarLVO(LVO *lista , int dni , Nodo** pos , int *exito,int *costo){

    ResetLVO(lista);


    while(lista->cur->dato.dni < dni){
        (*costo)++; //Aumento porque consulto
        FowardsLVO(lista);
    }

    (*costo)++; //Compruebo si el dato es el correcto

    *pos = lista->cur;

    if(lista->cur->dato.dni == dni){
        *exito = 1;
    }else{
        *exito = 0;
    }
}

void AltaLVO(LVO *lista,elector nuevoDato, int *exito,float *costo){
    Nodo *pos;
    int encontrado; //Seria el exito prima de los apuntes
    float costoLocalizar = 0;
    *costo = 0.0;

    LocalizarLVO(lista,nuevoDato.dni,&pos,&encontrado,&costoLocalizar);

    if (encontrado == 1){
        *exito = 0;
        return;
    }else{
        Nodo *nuevoNodo = (Nodo *)malloc(sizeof(Nodo));
        if(nuevoNodo != NULL){
            nuevoNodo->dato = nuevoDato;
            nuevoNodo->siguiente = pos;
            *costo += 0.5;

            if (pos == lista->acc){
                lista->acc = nuevoNodo;
                *costo += 0.5;
            }else{
                lista->aux->siguiente = nuevoNodo;
                *costo += 0.5;
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
    *costo = 0.0;
    if(encontrado == 1){
        if(CompararNuplas(pos->dato,nuplaBaja) == 1){
            if(pos == lista->acc){
            //Supress en la primera posición
            lista->acc = pos->siguiente;

            /*
            Supuestamente esto esta mal ?¿
            lista->acc=lista->cur->siguiente;
            free(lista->cur);
            lista->aux = lista->acc;
            lista->cur = lista->acc;*/
            *costo += 0.5;
            }else{
            //Supress en el medio o primera posición
            lista->aux->siguiente = pos->siguiente;
            *costo += 0.5;

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

/*void EvocarLSO(lso *lista, long dniBuscar, elector *eRecuperado, int *exito, float *costoConsultasLSO) {
    int pos;

    LocalizarLSO(lista, &pos, dniBuscar, exito, costoConsultasLSO);
    if (*exito == 1) {
        *eRecuperado = lista->electores[pos];
    }
}*/
void EvocarLVO(LVO *lista, long dniBuscar, elector *eRecuperado, int *exito,float *costo){

    Nodo *posLVO;

    LocalizarLVO()SO(lista,&posLVO,dniBuscar,exito,costo);

    if(*exito == 1){
        *eRecuperado = posLVO->dato;
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
        if(CompararNuplas(pos->valor,electorBaja)){ //Comprobamos que sea la nupla
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

int memorizarDesdeArchivo(lso lso[],LVO *lvo,ABB *abb, Estadisticas *statsLSO, Estadisticas *statsLVO, Estadisticas *statsABB){
    //Inicialización de Datos
    lso->cantidad = 0;
    InitLVO(lvo);
    *abb->raiz= NULL;

    //Definción de variables
    int exito;
    elector eTemp; //Como pivote para para las operaciónes de alta y baja
    int exitoLSO, exitoLVO, exitoABB;
    float costoLSO, costoLVO, costoABB;


    //Variables para la lectura de archivo
    FILE *archivo = fopen("Operaciones_Padron.txt", "r");
    char buffer[150]; //La linea que va a ser leida por el puntero file


    // Inicialización de Estadísticas LSO
    InicializarMetricas(&statsLSO->alta);
    InicializarMetricas(&statsLSO->baja);
    InicializarMetricas(&statsLSO->evocar_exito);
    InicializarMetricas(&statsLSO->evocar_fracaso);

    // Inicialización de Estadísticas LVO
    InicializarMetricas(&statsLVO->alta);
    InicializarMetricas(&statsLVO->baja);
    InicializarMetricas(&statsLVO->evocar_exito);
    InicializarMetricas(&statsLVO->evocar_fracaso);

    // Inicialización de Estadísticas ABB
    InicializarMetricas(&statsABB->alta);
    InicializarMetricas(&statsABB->baja);
    InicializarMetricas(&statsABB->evocar_exito);
    InicializarMetricas(&statsABB->evocar_fracaso);

    //Apertura de Archivo
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo.\n");
        return 0;
    }

    //Lectura de Archivo
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        //Se lee el codigo de operación
        int codigoOp = atoi(buffer);

        //LVO: se comprueba que se pueda crear un nuevo nodo.
        Nodo *aux =(Nodo *)malloc(sizeof(Nodo));
        if(aux == NULL){
            printf("ERROR: no hay memoria suficiente");
            break;
        }

        if(codigoOp == 1 | codigoOp == 2){ //Es un alta o una baja.
        //Creo el elector en la variable temporal.

            // Dni
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.dni = atoi(buffer);

            //Nombre y Apellido
            fgets(buffer, sizeof(buffer), archivo);
            buffer[strcspn(buffer, "\r\n")] = 0; // Limpiamos el salto de línea
            strcpy(eTemp.nombreApellido, buffer);

            //Domicilio
            fgets(buffer, sizeof(buffer), archivo);
            buffer[strcspn(buffer, "\r\n")] = 0;
            strcpy(eTemp.domicilio, buffer);

            //Código Postal
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.codigoPostal = atoi(buffer);

            //Mesa
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.numeroMesa = atoi(buffer);

            //Circuito
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.circuito = atoi(buffer);

            if(codigoOp == 1){

                //1. ALTA LSO
                AltaLSO(lista,eTemp)

                //2. ALTA LVO
                AltaLVO(lvo, eTemp, &exitoLVO, &costoLVO);
                RegistrarCosto(&statsLVO->alta, costoLVO);


                //3. ALTA ABB
                AltaABB();

            }else{
                //1. ALTA LSO
                BajaLSO();

                //2. ALTA LVO

                BajaLVO();


                //3. ALTA ABB
                BajaABB();

            }

        }else{ //Es una evocación

            fgets(linea, sizeof(linea), archivo);
            long dniEvocar = atol(linea);

            //1. EVOCACION LSO
            AltaLSO(lista,eTemp)

            //2. EVOCACION LVO

            AltaLVO();


            //3. EVOCACION ABB
            AltaABB();
        }





    }



}


int main() {
    ///Inicializaciónde Estructuras
    lso miLista;
    miLista.cantidad = 0;
    LVO lvoPadron;
    ABB abbPadron;


    // 1. Inicializamos todas nuestras estadísticas para la LSO
    Estadisticas statsLSO;
    InicializarMetricas(&statsLSO.alta);
    InicializarMetricas(&statsLSO.baja);
    InicializarMetricas(&statsLSO.evocar_exito);
    InicializarMetricas(&statsLSO.evocar_fracaso);

    FILE *archivo = fopen("Operaciones_Padron.txt", "r");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo.\n");
        return 1;
    }

    printf("Procesando archivo...\n");

    char buffer[150];
    int operacion;
    elector eTemp;
    int exito;

    // 2. LECTURA DEL ARCHIVO (El "Menú" automático)
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {

        // Leemos el código de operación (1, 2 o 3)
        if (sscanf(buffer, "%d", &operacion) != 1) continue;

        // Variable para medir el costo de ESTA operación en particular
        float costoOp = 0;

        switch(operacion) {
            case 1: // ALTA
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%ld", &eTemp.dni);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.nombreApellido, buffer);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.domicilio, buffer);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.codigoPostal);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.numeroMesa);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.circuito);

                // Mandamos a dar de alta y registramos sus corrimientos
                AltaLSO(&miLista, eTemp, &exito, &costoOp);
                RegistrarCosto(&statsLSO.alta, costoOp);
                break;

            case 2: // BAJA
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%ld", &eTemp.dni);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.nombreApellido, buffer);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.domicilio, buffer);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.codigoPostal);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.numeroMesa);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.circuito);

                // Mandamos a dar de baja y registramos sus corrimientos
                BajaLSO(&miLista, eTemp, &exito, &costoOp);
                RegistrarCosto(&statsLSO.baja, costoOp);
                break;

            case 3: // EVOCACION
                // La evocación SOLO trae el DNI en el archivo
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%ld", &eTemp.dni);

                int pos;
                // La evocación mide celdas consultadas
                LocalizarLSO(&miLista, &pos, eTemp.dni, &exito, &costoOp);

                if (exito == 1) {
                    RegistrarCosto(&statsLSO.evocar_exito, costoOp);
                } else {
                    RegistrarCosto(&statsLSO.evocar_fracaso, costoOp);
                }
                break;

            default:
                break;
        }
    }

    fclose(archivo);

    // 3. MOSTRAR LA TABLA DE COSTOS (Exactamente como en tu imagen)
    printf("\n---------------------------|--------------|");
    printf("\n                           |     LSOBB    |");
    printf("\n---------------------------|--------------|");

    // Altas
    printf("\nAlta                       |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.alta.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.alta.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.alta.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.alta.cantidad > 0 ? statsLSO.alta.costo_acu / statsLSO.alta.cantidad : 0);
    printf("\n---------------------------|--------------|");

    // Bajas
    printf("\nBaja                       |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.baja.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.baja.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.baja.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.baja.cantidad > 0 ? statsLSO.baja.costo_acu / statsLSO.baja.cantidad : 0);
    printf("\n---------------------------|--------------|");

    // Evocar Exitoso
    printf("\nEvocar exitoso             |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.evocar_exito.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.evocar_exito.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.evocar_exito.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.evocar_exito.cantidad > 0 ? statsLSO.evocar_exito.costo_acu / statsLSO.evocar_exito.cantidad : 0);
    printf("\n---------------------------|--------------|");

    // Evocar Fracaso
    printf("\nEvocar fracaso             |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.evocar_fracaso.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.evocar_fracaso.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.evocar_fracaso.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.evocar_fracaso.cantidad > 0 ? statsLSO.evocar_fracaso.costo_acu / statsLSO.evocar_fracaso.cantidad : 0);
    printf("\n---------------------------|--------------|\n");

    return 0;
}


