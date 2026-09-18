// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

//Constantes
#define ELECTORES_ESPERADOS 2000
#define MAS_INFINITO 999999999

//Definicion de struc's Patron
typedef struct {
    long dni;                 // El D.N.I. es un entero (usamos long por el tamaño del número)
    char nombreApellido[50];  // Secuencia de hasta 50 caracteres
    char domicilio[80];       // Secuencia de hasta 80 caracteres
    int codigoPostal;         // Es un entero
    int numeroMesa;           // Es un entero
    int circuito;             // Es un entero
} elector;

// --- VARIABLES GLOBALES PARA MEDIR COSTOS ---
float metrica_celdas_LSO = 0;
float metrica_corrimientos_LSO = 0;

// --- VARIABLES GLOBALES (VECTOR DE MARCAS Y BANCO HISTÓRICO) ---
elector bancoElectores[ELECTORES_ESPERADOS];
int marcas[ELECTORES_ESPERADOS];
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

void InicializarMetricas(MetricasOp *m) {
    m->cantidad = 0;
    m->costo_acu = 0.0;
    m->costo_max = 0.0;
}

void RegistrarCosto(MetricasOp *m, float costo_individual) {
    m->cantidad++;
    m->costo_acu += costo_individual;
    if (costo_individual > m->costo_max) {
        m->costo_max = costo_individual;
    }
}

// --- FUNCIONES DE COMPARACIÓN ---
int CompararNombresNoCaseSensitive(const char *str1, const char *str2) {
    while (*str1 && *str2) {
        if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) return 0;
        str1++;
        str2++;
    }
    return (*str1 == *str2) ? 1 : 0;
}

int CompararNuplas(elector e1, elector e2) {
    if (e1.dni == e2.dni &&
        e1.codigoPostal == e2.codigoPostal &&
        e1.numeroMesa == e2.numeroMesa &&
        e1.circuito == e2.circuito &&
        CompararNombresNoCaseSensitive(e1.nombreApellido, e2.nombreApellido) == 1 &&
        CompararNombresNoCaseSensitive(e1.domicilio, e2.domicilio) == 1) {
        return 1;
    }
    return 0;
}

// --- ACTUALIZACIÓN DEL VECTOR DE MARCAS ---
void ActualizarMarca(long dni, elector e, int operacion, int exito) {
    if (exito != 1) return; // Solo registramos si modificó la estructura

    if (operacion == 1) { // Alta exitosa
        for (int i = 0; i < totalHistorico; i++) {
            if (bancoElectores[i].dni == dni) {
                marcas[i] = 1;
                return;
            }
        }
        if (totalHistorico < ELECTORES_ESPERADOS) {
            bancoElectores[totalHistorico] = e;
            marcas[totalHistorico] = 1;
            totalHistorico++;
        }
    } else if (operacion == 2) { // Baja exitosa
        for (int i = 0; i < totalHistorico; i++) {
            if (bancoElectores[i].dni == dni) {
                marcas[i] = 0; // Desmarcado (Pasará a sumar a Fracaso)
                return;
            }
        }
    }
}

/* ==========================================================================
   1. Lista Secuencialmente Ordenada (LSO)
   ========================================================================== */

typedef struct{
  elector electores[ELECTORES_ESPERADOS];
  int cantidad;
} lso;

void LocalizarLSO(lso *lista, int *pos, long dni, int *exito) {
    if (lista->cantidad == 0) {
        *exito = 0;
        *pos = 0;
        return;
    }

    int li = 0;
    int ls = lista->cantidad - 1;
    int t;

    int consultada[ELECTORES_ESPERADOS] = {0};

    while (li < ls) {
        t = (li + ls) / 2; // Testigo a la izquierda

        if (!consultada[t]) {
            metrica_celdas_LSO++;
            consultada[t] = 1;
        }

        if (dni > lista->electores[t].dni) {
            li = t + 1;
        } else {
            ls = t;
        }
    }

    if (!consultada[li]) {
        metrica_celdas_LSO++;
        consultada[li] = 1;
    }

    if (lista->electores[li].dni == dni) {
        *exito = 1;
        *pos = li;
    } else {
        *exito = 0;
        if (dni > lista->electores[li].dni) {
            *pos = li + 1;
        } else {
            *pos = li;
        }
    }
}

void EvocarLSO(lso *lista, long dniBuscar, elector *eRecuperado, int *exito) {
    int pos;
    LocalizarLSO(lista, &pos, dniBuscar, exito);
    if (*exito == 1) {
        *eRecuperado = lista->electores[pos];
    }
}

void BajaLSO(lso *lista, elector eBorrar, int *exito){
    int pos;
    LocalizarLSO(lista, &pos, eBorrar.dni, exito);

    if (*exito == 1) {
        if (CompararNuplas(lista->electores[pos], eBorrar) == 1) {
            for (int j = pos; j < lista->cantidad - 1; j++) {
                lista->electores[j] = lista->electores[j + 1];
                metrica_corrimientos_LSO++; // Incremento de corrimientos
            }
            lista->cantidad--;
            *exito = 1;
        } else {
            *exito = 0;
        }
    }
}

void AltaLSO(lso *lista, elector nuevoElector, int *exito){
    int pos;
    int exitoPrima;

    LocalizarLSO(lista, &pos, nuevoElector.dni, &exitoPrima);

    if (exitoPrima == 1) {
        *exito = 2; // Repetido
    } else if (lista->cantidad == ELECTORES_ESPERADOS) {
        *exito = 3; // Lleno
    } else {
        for (int j = lista->cantidad; j > pos; j--) {
            lista->electores[j] = lista->electores[j - 1];
            metrica_corrimientos_LSO++; // Incremento de corrimientos
        }
        lista->electores[pos] = nuevoElector;
        lista->cantidad++;
        *exito = 1;
    }
}

void EvaluarCostosLocalizar(lso *listaLSO) {
    float costoTotalExito = 0, costoMaximoExito = 0;
    int cantExitos = 0;

    float costoTotalFracaso = 0, costoMaximoFracaso = 0;
    int cantFracasos = 0;

    for (int i = 0; i < totalHistorico; i++) {
        int pos;
        int exito;
        metrica_celdas_LSO = 0;
        LocalizarLSO(listaLSO, &pos, bancoElectores[i].dni, &exito);

        if (marcas[i] == 1) {
            costoTotalExito += metrica_celdas_LSO;
            if (metrica_celdas_LSO > costoMaximoExito) costoMaximoExito = metrica_celdas_LSO;
            cantExitos++;
        } else {
            costoTotalFracaso += metrica_celdas_LSO;
            if (metrica_celdas_LSO > costoMaximoFracaso) costoMaximoFracaso = metrica_celdas_LSO;
            cantFracasos++;
        }
    }

    printf("\n\n=== METRICAS PURAS DE 'LOCALIZAR' (USANDO VECTOR DE MARCAS) ===\n");
    if (cantExitos > 0) {
        printf("Localizacion Exitosa (N = %d):\n", cantExitos);
        printf("  - Costo Medio (Esperado): %.2f celdas\n", costoTotalExito / cantExitos);
        printf("  - Peor Escenario (Max): %.2f celdas\n", costoMaximoExito);
    }
    if (cantFracasos > 0) {
        printf("\nLocalizacion Fallida (N = %d):\n", cantFracasos);
        printf("  - Costo Medio (Esperado): %.2f celdas\n", costoTotalFracaso / cantFracasos);
        printf("  - Peor Escenario (Max): %.2f celdas\n", costoMaximoFracaso);
    }
    printf("=================================================================\n");
}

/* ==========================================================================
   2. LISTA VINCULADA ORDENADA
   ========================================================================== */

typedef struct Nodo{
    elector dato;
    struct Nodo* siguiente;
}Nodo;

typedef struct {
    Nodo *acc;
    Nodo *cur;
    Nodo *aux;
}LVO;

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
    return l.cur->dato;
}

<<<<<<< HEAD
void LocalizarLVO(LVO *lista , int dni , Nodo** pos , int *exito,float *costo){
    *costo =0;
    ResetLVO(lista);
=======

//----------------------//
void LocalizarLVO(LVO *lista , long dni , Nodo** pos , int *exito,float *costo){

    ResetLVO(lista);
    *costo = 0;

>>>>>>> 9a8685f (a)
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

            if (pos == lista->acc){
                lista->acc = nuevoNodo;
                *costo += 0.5;
            }else{
                lista->aux->siguiente = nuevoNodo;
                *costo += 0.5;
            }
            *exito = 1;
        }else{
            *exito = 0;
            return;
        }
    }
}

void BajaLVO(LVO *lista, elector nuplaBaja,int *exito,float *costo){
    Nodo *pos;
    int encontrado;
    float costoLocalizar;

    LocalizarLVO(lista,nuplaBaja.dni,&pos,&encontrado,&costoLocalizar);
    *costo = 0.0;
    if(encontrado == 1){
        if(CompararNuplas(pos->dato,nuplaBaja) == 1){
            if(pos == lista->acc){
                lista->acc = pos->siguiente;
                *costo += 0.5;
            }else{
                lista->aux->siguiente = pos->siguiente;
                *costo += 0.5;
            }
            free(pos);
            *exito = 1;
        } else {
            *exito = 0; //El dni coincidia pero la nupla no
        }
    }else{
        *exito = 0;
        return;
    }
}

void EvocarLVO(LVO *lista, long dniBuscar, elector *eRecuperado, int *exito,float *costo){
    Nodo *posLVO;
    LocalizarLVO(lista,dniBuscar,&posLVO,exito,costo);
    if(*exito == 1){
        *eRecuperado = posLVO->dato;
    }
}

/* ==========================================================================
   3. ARBOL BINARIO ORDENADO
   ========================================================================== */

typedef struct NodoArbol {
    elector valor;
    struct NodoArbol *hi; // Puntero al hijo izquierdo
    struct NodoArbol *hd; // Puntero al hijo derecho
} NodoArbol;

typedef struct {
    NodoArbol *raiz; // Puntero de inicio del árbol
} ABB;

void LocalizarABB(ABB *arbol, int x, NodoArbol **pos,int *exito,NodoArbol **padreRetornar,float *costo){
    NodoArbol *p = arbol->raiz;
    NodoArbol *padre = NULL;
    *costo = 0.0;

    while(p != NULL && p->valor.dni != x){
        (*costo)++;
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

void AltaABB(ABB *arbol, elector nuevoElector , int *exito,float *costo){
    NodoArbol *pos;
    int encontrado;
    float costoArbol;
    LocalizarABB(arbol,nuevoElector.dni,&pos,&encontrado,NULL,&costoArbol);
    *costo = 0.0;

    if(encontrado == 1){
        *exito = 0;
        return;
    }else{
        NodoArbol *nuevoNodo = (NodoArbol*)malloc(sizeof(NodoArbol));
        if(nuevoNodo != NULL){
            nuevoNodo->valor = nuevoElector;
            nuevoNodo->hi = NULL;
            *costo += 0.5;
            nuevoNodo->hd = NULL;
            *costo += 0.5;

            if(pos == NULL){
                arbol->raiz = nuevoNodo;
                *costo += 0.5;
            }else{
                if (nuevoElector.dni<pos->valor.dni){
                    pos->hi = nuevoNodo;
                    *costo += 0.5;
                }else{
                    pos->hd = nuevoNodo;
                    *costo += 0.5;
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

void BajaABB(ABB *arbol,elector electorBaja,int *exito,float *costo){
    NodoArbol *pos;
    NodoArbol *aux;
    NodoArbol *padreAux;
    NodoArbol *padre;
    int encontrado;
    *costo = 0.0;
    float costoArbol;
    LocalizarABB(arbol,electorBaja.dni,&pos,&encontrado,&padre,&costoArbol);

    if(encontrado == 1){
        if(CompararNuplas(pos->valor,electorBaja)){
            if(pos->hi == NULL && pos->hd == NULL){
                 if(padre != NULL){
                    if((pos->valor.dni) < (padre->valor.dni)){
                        padre->hi=NULL;
                    }else{
                        padre->hd=NULL;
                    }
                }else{
                    arbol->raiz = NULL;
                }
                free(pos);
                *costo += 0.5;
            }else if(pos->hd == NULL || pos->hi == NULL){
                 if (padre != NULL){
                    if((pos->valor.dni) < (padre->valor.dni)){
                        padre->hi=hijoNoNULLPos(pos);
                    }else{
                        padre->hd=hijoNoNULLPos(pos);
                    }
                 }else{
                    arbol->raiz = hijoNoNULLPos(pos);
                 }
                 free(pos);
                 *costo += 0.5;
            } else {
                padreAux = pos;
                aux = pos->hd;
                while(aux->hi != NULL){
                    padreAux = aux;
                    aux = aux->hi;
                }
                pos->valor = aux->valor;
                if (padreAux == pos){
                    padreAux->hd = aux->hd;
                }else{
                    padreAux->hi = aux->hd;
                }
                free(aux);
                *costo += 1.5;
            }
            *exito = 1;
        }else{
            *exito = 0;
            return;
        }
    }else{
        *exito = 0;
        return;
    }
}

/* ==========================================================================
   MEMORIZACIÓN DESDE ARCHIVO
   ========================================================================== */
int memorizarDesdeArchivo(lso *miLista, LVO *lvo, ABB *abb, Estadisticas *statsLSO, Estadisticas *statsLVO, Estadisticas *statsABB){
    //Inicialización de Datos
    miLista->cantidad = 0;
    InitLVO(lvo);
    abb->raiz = NULL;
    totalHistorico = 0;

    //Definción de variables
    elector eTemp;
    int exitoLSO, exitoLVO, exitoABB;
    float costoLSO, costoLVO, costoABB;

    //Variables para la lectura de archivo
    FILE *archivo = fopen("Operaciones_Padron.txt", "r");
    char buffer[150];

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
        int codigoOp = atoi(buffer);

        if(codigoOp == 1 || codigoOp == 2){ // Alta o baja

            // Dni
            if (fgets(buffer, sizeof(buffer), archivo) == NULL) break;
            eTemp.dni = atol(buffer);

            // Nombre
            fgets(buffer, sizeof(buffer), archivo);
            buffer[strcspn(buffer, "\r\n")] = 0;
            strcpy(eTemp.nombreApellido, buffer);

            // Domicilio
            fgets(buffer, sizeof(buffer), archivo);
            buffer[strcspn(buffer, "\r\n")] = 0;
            strcpy(eTemp.domicilio, buffer);

            // CP
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.codigoPostal = atoi(buffer);

            // Mesa
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.numeroMesa = atoi(buffer);

            // Circuito
            fgets(buffer, sizeof(buffer), archivo);
            eTemp.circuito = atoi(buffer);

            if(codigoOp == 1){
                //1. ALTA LSO
                metrica_corrimientos_LSO = 0;
                AltaLSO(miLista, eTemp, &exitoLSO);
                if (exitoLSO == 1) RegistrarCosto(&statsLSO->alta, metrica_corrimientos_LSO);
                ActualizarMarca(eTemp.dni, eTemp, 1, exitoLSO);

                //2. ALTA LVO
                AltaLVO(lvo, eTemp, &exitoLVO, &costoLVO);
                if (exitoLVO == 1) RegistrarCosto(&statsLVO->alta, costoLVO);

                //3. ALTA ABB
                AltaABB(abb, eTemp, &exitoABB, &costoABB);
                if (exitoABB == 1) RegistrarCosto(&statsABB->alta, costoABB);

            }else{
                //1. BAJA LSO
                metrica_corrimientos_LSO = 0;
                BajaLSO(miLista, eTemp, &exitoLSO);
                if (exitoLSO == 1) RegistrarCosto(&statsLSO->baja, metrica_corrimientos_LSO);
                ActualizarMarca(eTemp.dni, eTemp, 2, exitoLSO);

                //2. BAJA LVO
                BajaLVO(lvo, eTemp, &exitoLVO, &costoLVO);
                if (exitoLVO == 1) RegistrarCosto(&statsLVO->baja, costoLVO);

                //3. BAJA ABB
                BajaABB(abb, eTemp, &exitoABB, &costoABB);
                if (exitoABB == 1) RegistrarCosto(&statsABB->baja, costoABB);
            }

        } else if(codigoOp == 3){ // Evocación

            if (fgets(buffer, sizeof(buffer), archivo) == NULL) break;
            long dniEvocar = atol(buffer);

            //1. EVOCACION LSO
            metrica_celdas_LSO = 0;
            EvocarLSO(miLista, dniEvocar, &eTemp, &exitoLSO);
            if (exitoLSO == 1) {
                RegistrarCosto(&statsLSO->evocar_exito, metrica_celdas_LSO);
            } else {
                RegistrarCosto(&statsLSO->evocar_fracaso, metrica_celdas_LSO);
            }

            //2. EVOCACION LVO
            EvocarLVO(lvo, dniEvocar, &eTemp, &exitoLVO, &costoLVO);
            if (exitoLVO == 1) {
                RegistrarCosto(&statsLVO->evocar_exito, costoLVO);
            } else {
                RegistrarCosto(&statsLVO->evocar_fracaso, costoLVO);
            }

            //3. EVOCACION ABB
            NodoArbol *posABB, *padreABB;
            LocalizarABB(abb, dniEvocar, &posABB, &exitoABB, &padreABB, &costoABB);
            if (exitoABB == 1) {
                RegistrarCosto(&statsABB->evocar_exito, costoABB);
            } else {
                RegistrarCosto(&statsABB->evocar_fracaso, costoABB);
            }
        }
    }

    fclose(archivo);
    return 1;
}

/* ==========================================================================
   MAIN (FUSIONADO Y FORMATEADO)


/* AGREGAR AL MAIN DESPUES
/ Estructuras de control
lso miLista;
LVO lvoPadron;
ABB abbPadron;

// Métricas
Estadisticas statsLSO, statsLVO, statsABB;

// la función de memorizar desde archivo
memorizarDesdeArchivo(&miLista, &lvoPadron, &abbPadron, &statsLSO, &statsLVO, &statsABB);
*/


int main() {
    // Inicialización de Estructuras (LSO, LVO, ABB)
    lso miLista;
    LVO lvoPadron;
    ABB abbPadron;

    // Declaración de las Estructuras de Estadísticas
    Estadisticas statsLSO, statsLVO, statsABB;

    printf("Iniciando procesamiento del archivo de operaciones...\n");

    // Llamamos a la función que lee el archivo y llena las estructuras y métricas
    int cargaExitosa = memorizarDesdeArchivo(&miLista, &lvoPadron, &abbPadron,
                                             &statsLSO, &statsLVO, &statsABB);

    if (cargaExitosa == 0) {
        printf("Se aborto la ejecucion debido a un error en el archivo.\n");
        return 1; // Terminamos con error
    }

    printf("Procesamiento exitoso.\n");

    printf("\n-----------------------------|--------------|--------------|--------------|\n");
    printf("                             |   LVO +inf   |     LSOBB    |      ABB     |\n");
    printf("-----------------------------|--------------|--------------|--------------|\n");

    // --- ALTA ---
    printf("Alta                         |              |              |              |\n");
    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.alta.cantidad, statsLSO.alta.cantidad, statsABB.alta.cantidad);
    printf("  Costo Acumulado            | %12.2f | %12.2f | %12.2f |\n", statsLVO.alta.costo_acu, statsLSO.alta.costo_acu, statsABB.alta.costo_acu);
    printf("  Costo Maximo               | %12.2f | %12.2f | %12.2f |\n", statsLVO.alta.costo_max, statsLSO.alta.costo_max, statsABB.alta.costo_max);
    printf("  Costo Promedio             | %12.2f | %12.2f | %12.2f |\n",
        statsLVO.alta.cantidad > 0 ? statsLVO.alta.costo_acu / statsLVO.alta.cantidad : 0,
        statsLSO.alta.cantidad > 0 ? statsLSO.alta.costo_acu / statsLSO.alta.cantidad : 0,
        statsABB.alta.cantidad > 0 ? statsABB.alta.costo_acu / statsABB.alta.cantidad : 0);
    printf("-----------------------------|--------------|--------------|--------------|\n");

    // --- BAJA ---
    printf("Baja                         |              |              |              |\n");
    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.baja.cantidad, statsLSO.baja.cantidad, statsABB.baja.cantidad);
    printf("  Costo Acumulado            | %12.2f | %12.2f | %12.2f |\n", statsLVO.baja.costo_acu, statsLSO.baja.costo_acu, statsABB.baja.costo_acu);
    printf("  Costo Maximo               | %12.2f | %12.2f | %12.2f |\n", statsLVO.baja.costo_max, statsLSO.baja.costo_max, statsABB.baja.costo_max);
    printf("  Costo Promedio             | %12.2f | %12.2f | %12.2f |\n",
        statsLVO.baja.cantidad > 0 ? statsLVO.baja.costo_acu / statsLVO.baja.cantidad : 0,
        statsLSO.baja.cantidad > 0 ? statsLSO.baja.costo_acu / statsLSO.baja.cantidad : 0,
        statsABB.baja.cantidad > 0 ? statsABB.baja.costo_acu / statsABB.baja.cantidad : 0);
    printf("-----------------------------|--------------|--------------|--------------|\n");

    // --- EVOCAR EXITOSO ---
    printf("Evocar exitoso               |              |              |              |\n");
    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.evocar_exito.cantidad, statsLSO.evocar_exito.cantidad, statsABB.evocar_exito.cantidad);
    printf("  Costo Acumulado            | %12.2f | %12.2f | %12.2f |\n", statsLVO.evocar_exito.costo_acu, statsLSO.evocar_exito.costo_acu, statsABB.evocar_exito.costo_acu);
    printf("  Costo Maximo               | %12.2f | %12.2f | %12.2f |\n", statsLVO.evocar_exito.costo_max, statsLSO.evocar_exito.costo_max, statsABB.evocar_exito.costo_max);
    printf("  Costo Promedio             | %12.2f | %12.2f | %12.2f |\n",
        statsLVO.evocar_exito.cantidad > 0 ? statsLVO.evocar_exito.costo_acu / statsLVO.evocar_exito.cantidad : 0,
        statsLSO.evocar_exito.cantidad > 0 ? statsLSO.evocar_exito.costo_acu / statsLSO.evocar_exito.cantidad : 0,
        statsABB.evocar_exito.cantidad > 0 ? statsABB.evocar_exito.costo_acu / statsABB.evocar_exito.cantidad : 0);
    printf("-----------------------------|--------------|--------------|--------------|\n");

    // --- EVOCAR FRACASO ---
    printf("Evocar fracaso               |              |              |              |\n");
    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.evocar_fracaso.cantidad, statsLSO.evocar_fracaso.cantidad, statsABB.evocar_fracaso.cantidad);
    printf("  Costo Acumulado            | %12.2f | %12.2f | %12.2f |\n", statsLVO.evocar_fracaso.costo_acu, statsLSO.evocar_fracaso.costo_acu, statsABB.evocar_fracaso.costo_acu);
    printf("  Costo Maximo               | %12.2f | %12.2f | %12.2f |\n", statsLVO.evocar_fracaso.costo_max, statsLSO.evocar_fracaso.costo_max, statsABB.evocar_fracaso.costo_max);
    printf("  Costo Promedio             | %12.2f | %12.2f | %12.2f |\n",
        statsLVO.evocar_fracaso.cantidad > 0 ? statsLVO.evocar_fracaso.costo_acu / statsLVO.evocar_fracaso.cantidad : 0,
        statsLSO.evocar_fracaso.cantidad > 0 ? statsLSO.evocar_fracaso.costo_acu / statsLSO.evocar_fracaso.cantidad : 0,
        statsABB.evocar_fracaso.cantidad > 0 ? statsABB.evocar_fracaso.costo_acu / statsABB.evocar_fracaso.cantidad : 0);
    printf("-----------------------------|--------------|--------------|--------------|\n");

    // --- PRUEBA TEÓRICA DE LA FUNCIÓN "LOCALIZAR" ---
    EvaluarCostosLocalizar(&miLista);

    return 0;
}
