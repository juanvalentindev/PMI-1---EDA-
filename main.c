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
        printf("  - Costo Medio (Esperado): %.3f celdas\n", costoTotalExito / cantExitos);
        printf("  - Peor Escenario (Max): %.3f celdas\n", costoMaximoExito);
    }
    if (cantFracasos > 0) {
        printf("\nLocalizacion Fallida (N = %d):\n", cantFracasos);
        printf("  - Costo Medio (Esperado): %.3f celdas\n", costoTotalFracaso / cantFracasos);
        printf("  - Peor Escenario (Max): %.3f celdas\n", costoMaximoFracaso);
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

//----------------------//
void LocalizarLVO(LVO *lista , long dni , Nodo** pos , int *exito,float *costo){

    ResetLVO(lista);
    *costo = 0;


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
    *costo = 0.0;

    LocalizarLVO(lista,nuplaBaja.dni,&pos,&encontrado,&costoLocalizar);

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

void LocalizarABB(ABB *arbol, long x, NodoArbol **pos, int *exito, NodoArbol **padreRetornar, float *costo){
    NodoArbol *p = arbol->raiz;
    NodoArbol *padre = NULL;
    *costo = 0.0;

    while(p != NULL){
        (*costo)++; // 1. Cobramos la celda que acabamos de pisar

        if (p->valor.dni == x) {
            break;  // 2. Lo encontramos, detenemos la búsqueda inmediatamente
        }

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

    if(padreRetornar != NULL){
        *padreRetornar = padre; // Retorno el padre para poder utilizarlo en la baja
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
            //*costo += 0.5;
            nuevoNodo->hd = NULL;
            //*costo += 0.5;

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



void EvocarABB(ABB *arbol, long dniBuscado, elector *electorRetornado, int *exito, float *costo){
    NodoArbol *pos = NULL;

    // Llamamos al localizar
    LocalizarABB(arbol, dniBuscado, &pos, exito, NULL, costo);

    if(*exito == 1){
        *electorRetornado = pos->valor;
    }
}





void MostrarEstructuraABB(ABB *arbol){


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
            EvocarABB(abb, dniEvocar, &eTemp, &exitoABB, &costoABB);
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


/*
========================
FUNCIONES PARA DESTRUIR ESTRUCTURA
========================
*/

void DestruirNodosABB(NodoArbol *nodo) {
    if (nodo != NULL) {
        DestruirNodosABB(nodo->hi); // Liberar subárbol izquierdo
        DestruirNodosABB(nodo->hd); // Liberar subárbol derecho
        free(nodo);                 // Liberar el nodo actual
    }
}

void DestruirABB(ABB *arbol) {
    if (arbol != NULL) {
        DestruirNodosABB(arbol->raiz);
        arbol->raiz = NULL;
    }

}


void DestruirLVO(LVO *l) {
    Nodo *actual = l->acc;
    Nodo *siguienteNodo;

    // Recorremos la lista hasta llegar al final (NULL)
    while (actual != NULL) {
        siguienteNodo = actual->siguiente; // Guardamos el enganche
        free(actual);                      // Liberamos el nodo actual
        actual = siguienteNodo;            // Avanzamos al siguiente
    }

    // Por seguridad, reseteamos los punteros de la estructura
    ResetLVO(l);
}

void DestruirLSO(lso *lista) {
    lista->cantidad = 0;
}





/* ==========================================================================
    FUNCIONES PARA MOSTRAR LAS ESTRUCTURAS PAPÁ
   ==========================================================================
*/

void MostrarLSO(lso *lista) {
    if (lista->cantidad == 0) {
        printf("\nLa estructura LSOBB se encuentra vacia.\n");
        return;
    }

    // Limpieza de buffer inicial
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("\n====================================================================\n");
    printf("         PADRON DE ELECTORES (LSOBB) - Total: %d electores\n", lista->cantidad);
    printf("====================================================================\n");

    for (int i = 0; i < lista->cantidad; i++) {
        printf("[%4d] DNI: %-10ld | %-25.25s | %-15.15s | CP: %-4d | Mesa: %-4d | Circ.: %-4d\n",
               i + 1,
               lista->electores[i].dni,
               lista->electores[i].nombreApellido,
               lista->electores[i].domicilio,
               lista->electores[i].codigoPostal,
               lista->electores[i].numeroMesa,
               lista->electores[i].circuito);

        //Mostramos 20
        if ((i + 1) % 20 == 0 && (i + 1) < lista->cantidad) {
            printf("\n--- Mostrando %d de %d. Presione ENTER para continuar o 'Q' para salir ---", i + 1, lista->cantidad);

            char opcion = getchar();

            if (opcion == 'q' || opcion == 'Q') {
                // Limpiamos el ENTER que quedó en el buffer tras presionar la Q
                while ((c = getchar()) != '\n' && c != EOF);
                printf("\nListado abortado por el usuario.\n");
                break;
            }
            // Si presionó cualquier otra tecla antes del ENTER (por error), limpiamos el buffer
            else if (opcion != '\n') {
                while ((c = getchar()) != '\n' && c != EOF);
            }
        }
    }
    printf("====================================================================\n");
    printf("Fin del listado.\n");
}


void MostrarLVO(LVO *lista) {
    // Verificamos si la estructura está vacía
    if (lista->acc->dato.dni == MAS_INFINITO) {
        printf("\nLa estructura LVO se encuentra vacia.\n");
        return;
    }

    // Limpieza de buffer inicial
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("\n====================================================================\n");
    printf("                  PADRON DE ELECTORES (LVO +inf)\n");
    printf("====================================================================\n");

    // Iniciamos el recorrido desde el primer nodo (acc)
    Nodo *actual = lista->acc;
    int contador = 0; // Como no hay un "for" con "i", llevamos la cuenta manualmente

    // Recorremos mientras el DNI del nodo actual NO sea el +infinito
    while (actual->dato.dni != MAS_INFINITO) {
        contador++;

        printf("[%4d] DNI: %-10ld | %-25.25s | %-15.15s | CP: %-4d | Mesa: %-4d | Circ.: %-4d\n",
               contador,
               actual->dato.dni,
               actual->dato.nombreApellido,
               actual->dato.domicilio,
               actual->dato.codigoPostal,
               actual->dato.numeroMesa,
               actual->dato.circuito);

        // Paginamos la muestra cada 20 registros
        if (contador % 20 == 0) {
            printf("\n--- Mostrando %d registros. Presione ENTER para continuar o 'Q' para salir ---", contador);

            char opcion = getchar();

            if (opcion == 'q' || opcion == 'Q') {
                while ((c = getchar()) != '\n' && c != EOF);
                printf("\nListado abortado por el usuario.\n");
                break;
            }
            else if (opcion != '\n') {
                while ((c = getchar()) != '\n' && c != EOF);
            }
        }

        // Saltamos al siguiente nodo de la memoria
        actual = actual->siguiente;
    }

    printf("====================================================================\n");
    printf("Fin del listado. Total mostrados: %d electores\n", contador);
}


void MostrarABB(ABB *arbol) {
    // Verificamos si la estructura está vacía chequeando la raíz
    if (arbol->raiz == NULL) {
        printf("\nLa estructura ABB se encuentra vacia.\n");
        return;
    }

    // Limpieza de buffer inicial
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    printf("\n==================================================================================\n");
    printf("           PADRON DE ELECTORES (ABB - Recorrido Preorden Iterativo)\n");
    printf("==================================================================================\n");

    // Creación de la Pila para hacer el recorrido Iterativo
    // Y un árbol degenerado (todos en fila) de 2000 elementos ocuparía 2000 lugares en la pila.
    NodoArbol* pila[2005];
    int tope = -1;

    // Iniciamos apilando la raíz
    pila[++tope] = arbol->raiz;
    int contador = 0;

    // Mientras la pila no esté vacía
    while (tope >= 0) {
        // Desapilamos el nodo actual para procesarlo (Visitar Raíz)
        NodoArbol* actual = pila[tope--];
        contador++;

        // Preparamos textos para mostrar los DNI de los hijos
        char infoHI[25] = "HI: No tiene";
        char infoHD[25] = "HD: No tiene";

        if (actual->hi != NULL) {
            sprintf(infoHI, "HI: %ld", actual->hi->valor.dni);
        }
        if (actual->hd != NULL) {
            sprintf(infoHD, "HD: %ld", actual->hd->valor.dni);
        }

        // Mostramos el elector corriente junto a sus conexiones
        printf("[%4d] DNI: %-10ld | %-20.20s | %-14s | %-14s\n",
               contador,
               actual->valor.dni,
               actual->valor.nombreApellido,
               infoHI,
               infoHD);

        // Skip con "Q"
        if (contador % 20 == 0) {
            printf("\n--- Mostrando %d registros. Presione ENTER para continuar o 'Q' para salir ---", contador);

            char opcion = getchar();
            if (opcion == 'q' || opcion == 'Q') {
                while ((c = getchar()) != '\n' && c != EOF);
                printf("\nListado abortado por el usuario.\n");
                break;
            } else if (opcion != '\n') {
                while ((c = getchar()) != '\n' && c != EOF);
            }
        }

        // Apilamos los hijos.
        // Como las pilas son LIFO (Last In, First Out), apilamos PRIMERO el DERECHO y LUEGO el IZQUIERDO.
        // Así, al desapilar en la siguiente vuelta, el Izquierdo sale primero.
        if (actual->hd != NULL) {
            pila[++tope] = actual->hd;
        }
        if (actual->hi != NULL) {
            pila[++tope] = actual->hi;
        }
    }

    printf("==================================================================================\n");
    printf("Fin del listado. Total mostrados: %d electores\n", contador);
}


void MostrarNodosABB(const NodoArbol *nodo, int *contador) {
    if (nodo != NULL) {
        MostrarNodosABB(nodo->hi, contador); // 1. Subárbol izquierdo

        printf("%-10ld | %-25s | %-20s | %-4d | %-5d | %-8d\n",
               nodo->valor.dni,
               nodo->valor.nombreApellido,
               nodo->valor.domicilio,
               nodo->valor.codigoPostal,
               nodo->valor.numeroMesa,
               nodo->valor.circuito);

        (*contador)++; // Sumamos 1 al contador

        // Pausar cada 15 registros
        if (*contador % 15 == 0) {
            printf("\n--- Mostrados: %d registros. Presiona ENTER para continuar ---", *contador);
            getchar();


            printf("\n%-10s|%-25s|%-20s|%-4s|%-5s|%-8s\n",
                   "DNI", "NOMBRE Y APELLIDO", "DOMICILIO", "CP", "MESA", "CIRCUITO");
            printf("--------------------------------------------------------------------------------------\n");
        }

        MostrarNodosABB(nodo->hd, contador);  //Subárbol derecho
    }
}

void MostrarRecABB(const ABB *arbol) {
    int contador = 0; // Inicializamos el contador en 0

    if (arbol != NULL && arbol->raiz != NULL) {
        printf("--- Listado de Electores ABB ---\n");

        fflush(stdin);

        // Llamamos a la recursiva pasando la dirección de memoria del contador
        MostrarNodosABB(arbol->raiz, &contador);

        printf("\n---------------------------------------------------------\n");
        printf("Fin del listado. Total de electores mostrados: %d\n", contador);
    } else {
        printf("El árbol está vacío.\n");
    }
}


/* ==========================================================================
   MAIN (FUSIONADO Y FORMATEADO)
   ==========================================================================
*/

int main() {
   //Declaración de Estructuras
    lso miLista;
    LVO lvoPadron;
    ABB abbPadron;

    // Declaración de las Estructuras de Estadísticas
    Estadisticas statsLSO, statsLVO, statsABB;

    int opcion;
    int estructurasCargadas = 0; // Bandera para saber si ya se procesó el archivo

    do {
        printf("\n======================================================\n");
        printf("               MENU PRINCIPAL - PADRON                \n");
        printf("======================================================\n");
        printf(" 1. Comparacion de Estructuras (Procesar Archivo)\n");
        printf(" 2. Mostrar Estructura\n");
        printf(" 3. Salir\n");
        printf("======================================================\n");
        printf("Ingrese una opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1:
                printf("\nIniciando procesamiento del archivo de operaciones...\n");

                DestruirABB(&abbPadron);
                DestruirLSO(&miLista);
                DestruirLVO(&lvoPadron);

                int cargaExitosa = memorizarDesdeArchivo(&miLista, &lvoPadron, &abbPadron, &statsLSO, &statsLVO, &statsABB);

                if (cargaExitosa == 0) {
                    printf("Se aborto la ejecucion debido a un error en el archivo.\n");
                } else {
                    estructurasCargadas = 1; // Marcamos que los datos quedaron almacenados
                    printf("Procesamiento exitoso.\n");

                    // --- IMPRESIÓN DE LA TABLA DE COMPARACIÓN DE COSTOS ---
                    printf("\n-----------------------------|--------------|--------------|--------------|\n");
                    printf("                             |   LVO +inf   |     LSOBB    |      ABB     |\n");
                    printf("-----------------------------|--------------|--------------|--------------|\n");

                    // --- ALTA ---
                    printf("Alta                         |              |              |              |\n");
                    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.alta.cantidad, statsLSO.alta.cantidad, statsABB.alta.cantidad);
                    printf("  Costo Acumulado            | %12.3f | %12.3f | %12.3f |\n", statsLVO.alta.costo_acu, statsLSO.alta.costo_acu, statsABB.alta.costo_acu);
                    printf("  Costo Maximo               | %12.3f | %12.3f | %12.3f |\n", statsLVO.alta.costo_max, statsLSO.alta.costo_max, statsABB.alta.costo_max);
                    printf("  Costo Promedio             | %12.3f | %12.3f | %12.3f |\n",
                        statsLVO.alta.cantidad > 0 ? statsLVO.alta.costo_acu / statsLVO.alta.cantidad : 0,
                        statsLSO.alta.cantidad > 0 ? statsLSO.alta.costo_acu / statsLSO.alta.cantidad : 0,
                        statsABB.alta.cantidad > 0 ? statsABB.alta.costo_acu / statsABB.alta.cantidad : 0);
                    printf("-----------------------------|--------------|--------------|--------------|\n");

                    // --- BAJA ---
                    printf("Baja                         |              |              |              |\n");
                    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.baja.cantidad, statsLSO.baja.cantidad, statsABB.baja.cantidad);
                    printf("  Costo Acumulado            | %12.3f | %12.3f | %12.3f |\n", statsLVO.baja.costo_acu, statsLSO.baja.costo_acu, statsABB.baja.costo_acu);
                    printf("  Costo Maximo               | %12.3f | %12.3f | %12.3f |\n", statsLVO.baja.costo_max, statsLSO.baja.costo_max, statsABB.baja.costo_max);
                    printf("  Costo Promedio             | %12.3f | %12.3f | %12.3f |\n",
                        statsLVO.baja.cantidad > 0 ? statsLVO.baja.costo_acu / statsLVO.baja.cantidad : 0,
                        statsLSO.baja.cantidad > 0 ? statsLSO.baja.costo_acu / statsLSO.baja.cantidad : 0,
                        statsABB.baja.cantidad > 0 ? statsABB.baja.costo_acu / statsABB.baja.cantidad : 0);
                    printf("-----------------------------|--------------|--------------|--------------|\n");

                    // --- EVOCAR EXITOSO ---
                    printf("Evocar exitoso               |              |              |              |\n");
                    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.evocar_exito.cantidad, statsLSO.evocar_exito.cantidad, statsABB.evocar_exito.cantidad);
                    printf("  Costo Acumulado            | %12.3f | %12.3f | %12.3f |\n", statsLVO.evocar_exito.costo_acu, statsLSO.evocar_exito.costo_acu, statsABB.evocar_exito.costo_acu);
                    printf("  Costo Maximo               | %12.3f | %12.3f | %12.3f |\n", statsLVO.evocar_exito.costo_max, statsLSO.evocar_exito.costo_max, statsABB.evocar_exito.costo_max);
                    printf("  Costo Promedio             | %12.3f | %12.3f | %12.3f |\n",
                        statsLVO.evocar_exito.cantidad > 0 ? statsLVO.evocar_exito.costo_acu / statsLVO.evocar_exito.cantidad : 0,
                        statsLSO.evocar_exito.cantidad > 0 ? statsLSO.evocar_exito.costo_acu / statsLSO.evocar_exito.cantidad : 0,
                        statsABB.evocar_exito.cantidad > 0 ? statsABB.evocar_exito.costo_acu / statsABB.evocar_exito.cantidad : 0);
                    printf("-----------------------------|--------------|--------------|--------------|\n");

                    // --- EVOCAR FRACASO ---
                    printf("Evocar fracaso               |              |              |              |\n");
                    printf("  Cantidad                   | %12d | %12d | %12d |\n", statsLVO.evocar_fracaso.cantidad, statsLSO.evocar_fracaso.cantidad, statsABB.evocar_fracaso.cantidad);
                    printf("  Costo Acumulado            | %12.3f | %12.3f | %12.3f |\n", statsLVO.evocar_fracaso.costo_acu, statsLSO.evocar_fracaso.costo_acu, statsABB.evocar_fracaso.costo_acu);
                    printf("  Costo Maximo               | %12.3f | %12.3f | %12.3f |\n", statsLVO.evocar_fracaso.costo_max, statsLSO.evocar_fracaso.costo_max, statsABB.evocar_fracaso.costo_max);
                    printf("  Costo Promedio             | %12.3f | %12.3f | %12.3f |\n",
                        statsLVO.evocar_fracaso.cantidad > 0 ? statsLVO.evocar_fracaso.costo_acu / statsLVO.evocar_fracaso.cantidad : 0,
                        statsLSO.evocar_fracaso.cantidad > 0 ? statsLSO.evocar_fracaso.costo_acu / statsLSO.evocar_fracaso.cantidad : 0,
                        statsABB.evocar_fracaso.cantidad > 0 ? statsABB.evocar_fracaso.costo_acu / statsABB.evocar_fracaso.cantidad : 0);
                    printf("-----------------------------|--------------|--------------|--------------|\n");

                    // Prueba Teórica solicitada en la corrección
                    EvaluarCostosLocalizar(&miLista);
                }
                break;

            case 2:
                if (estructurasCargadas == 0) {
                    printf("\nError: Debe ejecutar la 'Comparacion de Estructuras' (Opcion 1) primero para cargar los datos.\n");
                } else {
                    int subOpcion;
                    printf("\n--- MOSTRAR ESTRUCTURA ---\n");
                    printf("1. Lista Secuencial Ordenada (LSO)\n");
                    printf("2. Lista Vinculada Ordenada (LVO)\n");
                    printf("3. Arbol Binario de Busqueda (ABB)\n");
                    printf("Elija estructura a mostrar: ");
                    scanf("%d", &subOpcion);


                    if (subOpcion == 1) {
                        MostrarLSO(&miLista);
                    } else if (subOpcion == 2) {
                        MostrarLVO(&lvoPadron);
                    } else if (subOpcion == 3) {
                        //MostrarABB(&abbPadron);
                        MostrarRecABB(&abbPadron);
                    } else {
                        printf("\nOpcion invalida.\n");
                    }
                }
                break;

            case 3:
                printf("\nFinalizando programa. Cuidate Masterr\n");
                break;

            default:
                printf("\nOpcion no valida. Intente nuevamente.\n");
        }

    } while(opcion != 3);

    return 0;
}
