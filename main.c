
// Librerias
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

//Constantes
#define ELECTORES_ESPERADOS 2000
#define MAS_INFINITO 999999999

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



/*
 =====================================

1. Lista Secuencialmente Ordenada (LSO)

================================
*/

typedef struct{
  elector electores[ELECTORES_ESPERADOS];
  int cantidad;
} lso;

void LocalizarLSO(lso *lista, int *pos, long dni, int *exito) {
    // Protección si la lista está vacía
    if (lista->cantidad == 0) {
        *exito = 0;
        *pos = 0;
        return;
    }

    int li = 0;
    int ls = lista->cantidad - 1;
    int t;

    // Arreglo local para no contabilizar dos veces la misma celda
    int consultada[ELECTORES_ESPERADOS] = {0};

    while (li < ls) {
        t = (li + ls) / 2; // Testigo a la izquierda

        if (!consultada[t]) {
            metrica_celdas_LSO++;
            consultada[t] = 1;
        }

        if (dni > lista->electores[t].dni) {
            li = t + 1;     // Descartamos la mitad izquierda
        } else {
            ls = t;         // Retenemos el testigo en el subrango izquierdo
        }
    }

    // Al salir del while, li == ls (queda exactamente 1 celda candidata)
    if (!consultada[li]) {
        metrica_celdas_LSO++;
        consultada[li] = 1;
    }

    // Única comprobación de igualdad al final
    if (lista->electores[li].dni == dni) {
        *exito = 1;
        *pos = li;
    } else {
        *exito = 0;
        // Si no se encontró, determinamos la posición exacta de inserción
        if (dni > lista->electores[li].dni) {
            *pos = li + 1; // Debe insertarse al final
        } else {
            *pos = li;     // Debe insertarse desplazando este elemento
        }
    }
}

void EvocarLSO(lso *lista, long dniBuscar, elector *eRecuperado, int *exito) {
    int pos;
    // 1. Usa la búsqueda interna
    LocalizarLSO(lista, &pos, dniBuscar, exito);

    // 2. Si hubo éxito, extrae y entrega la información asociada Y
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

/* ==========================================================================
   EVALUACIÓN DEL "LOCALIZAR" USANDO EL VECTOR DE MARCAS
   ========================================================================== */
void EvaluarCostosLocalizar(lso *listaLSO) {
    float costoTotalExito = 0, costoMaximoExito = 0;
    int cantExitos = 0;

    float costoTotalFracaso = 0, costoMaximoFracaso = 0;
    int cantFracasos = 0;

    for (int i = 0; i < totalHistorico; i++) {
        int pos;
        int exito;

        // 1. Reiniciamos el contador global a cero para esta consulta aislada
        metrica_celdas_LSO = 0;

        // 2. Ejecutamos la búsqueda (que internamente sumará puntos a la métrica)
        LocalizarLSO(listaLSO, &pos, bancoElectores[i].dni, &exito);

        // 3. Utilizamos SOLO el vector de marcas para clasificar este costo
        if (marcas[i] == 1) {
            // Pertenece a la estructura (Éxito Puro)
            costoTotalExito += metrica_celdas_LSO;
            if (metrica_celdas_LSO > costoMaximoExito) costoMaximoExito = metrica_celdas_LSO;
            cantExitos++;
        } else {
            // Fue dado de baja (Fracaso Puro)
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
void LocalizarLVO(LVO *lista , int dni , Nodo** pos , int *exito,float *costo){

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
            //*costo += 0.5;

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

void BajaLVO(LVO *lista, elector nuplaBaja,int *exito,float *costo){
    Nodo *pos;
    int encontrado;
    float costoLocalizar;

    LocalizarLVO(lista,nuplaBaja.dni,&pos,&encontrado,&costoLocalizar);
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

    LocalizarLVO(lista,dniBuscar,&posLVO,exito,costo);

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
        //(*costo)++; //Esta seria la ultima comparación del arbol
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
    NodoArbol *aux; //este es para el caso heavy metal (buscar el mayor de los menores)
    NodoArbol *padreAux;
    NodoArbol *padre;
    int encontrado;
    *costo = 0.0;
    float costoArbol;
    LocalizarABB(arbol,electorBaja.dni,&pos,&encontrado,&padre,&costoArbol);

    if(encontrado == 1){ //Existe una nupla con ese x

        if(CompararNuplas(pos->valor,electorBaja)){ //Comprobamos que sea la nupla
                //Aca seria lo de modificación para la baja
                //aca empieza el kilombo :(
            //Caso 1: tengo un nodo padre con dos hijos
            if(pos->hi == NULL && pos->hd == NULL){
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
                *costo += 0.5; // 1 sola modificación de puntero al padre

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
                 *costo += 0.5; // 1 sola modificación de puntero al padre o raíz
            } else { //Caso con 2 hijos, el mas heavy metal chabon
                 padreAux = pos;
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
                *costo = 1.5;


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
    abb->raiz= NULL;

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
        //Aca cargamos el elector

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
                AltaLSO(lso, eTemp, &exitoLSO);
                RegistrarCosto(&statsLSO->alta, costoLSO);

                //2. ALTA LVO
                AltaLVO(lvo, eTemp, &exitoLVO, &costoLVO);
                RegistrarCosto(&statsLVO->alta, costoLVO);


                //3. ALTA ABB
                AltaABB(abb, eTemp, &exitoABB, &costoABB);
                RegistrarCosto(&statsABB->alta, costoABB);

            }else{
                //1. BAJA LSO
                BajaLSO(lso, eTemp, &exitoLSO);
                RegistrarCosto(&statsLSO->baja, costoLSO);

                //2. BAJA LVO
                BajaLVO(lvo, eTemp, &exitoLVO, &costoLVO);
                RegistrarCosto(&statsLVO->baja, costoLVO);


                //3. BAJA ABB
                BajaABB(abb, eTemp, &exitoABB, &costoABB);
                RegistrarCosto(&statsABB->baja, costoABB);
            }

        }else if(codigoOp == 3){ //Es una evocación

            //Solo chequeamos el dni
            fgets(buffer, sizeof(buffer), archivo);
            long dniEvocar = atol(buffer);


            int posLSO; //LSO: Variable para ralmacenar la posición
            NodoArbol *posABB,*padreABB; //ABB: Nodos a retornar del evocar



            //1. EVOCACION LSO
            LocalizarLSO(lso, &posLSO, dniEvocar, &exitoLSO);
            if (exitoLSO == 1) {
                RegistrarCosto(&statsLSO->evocar_exito, costoLSO);
            } else {
                RegistrarCosto(&statsLSO->evocar_fracaso, costoLSO);
            }

            //2. EVOCACION LVO
            EvocarLVO(lvo, dniEvocar, &eTemp, &exitoLVO, &costoLVO);
            if (exitoLVO == 1) {
                RegistrarCosto(&statsLVO->evocar_exito, costoLVO);
            } else {
                RegistrarCosto(&statsLVO->evocar_fracaso, costoLVO);
            }


            //3. EVOCACION ABB
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
    lso miLista;
    miLista.cantidad = 0;

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
    int operacion, exito;
    elector eTemp;

    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        if (sscanf(buffer, "%d", &operacion) != 1) continue;

        switch(operacion) {
            case 1: // ALTA
                if (fgets(buffer, sizeof(buffer), archivo) == NULL) break;
                sscanf(buffer, "%ld", &eTemp.dni);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.nombreApellido, buffer);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.domicilio, buffer);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.codigoPostal);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.numeroMesa);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.circuito);

                metrica_corrimientos_LSO = 0;
                AltaLSO(&miLista, eTemp, &exito);

                // CANDADO: Solo registrar costo si realmente se insertó
                if (exito == 1) {
                    RegistrarCosto(&statsLSO.alta, metrica_corrimientos_LSO);
                }
                ActualizarMarca(eTemp.dni, eTemp, 1, exito);
                break;

            case 2: // BAJA
                if (fgets(buffer, sizeof(buffer), archivo) == NULL) break;
                sscanf(buffer, "%ld", &eTemp.dni);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.nombreApellido, buffer);
                fgets(buffer, sizeof(buffer), archivo); buffer[strcspn(buffer, "\r\n")] = 0; strcpy(eTemp.domicilio, buffer);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.codigoPostal);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.numeroMesa);
                fgets(buffer, sizeof(buffer), archivo); sscanf(buffer, "%d", &eTemp.circuito);

                metrica_corrimientos_LSO = 0;
                BajaLSO(&miLista, eTemp, &exito);

                // CANDADO: Solo registrar costo si realmente se eliminó
                if (exito == 1) {
                    RegistrarCosto(&statsLSO.baja, metrica_corrimientos_LSO);
                }
                ActualizarMarca(eTemp.dni, eTemp, 2, exito);
                break;

            case 3: // EVOCACION DESDE EL ARCHIVO
                if (fgets(buffer, sizeof(buffer), archivo) == NULL) break;
                if (sscanf(buffer, "%ld", &eTemp.dni) != 1) break; // Evita el fantasma del EOF

                int pos;
                metrica_celdas_LSO = 0;
                EvocarLSO(&miLista, eTemp.dni, &eTemp, &exito);

                if (exito == 1) {
                    RegistrarCosto(&statsLSO.evocar_exito, metrica_celdas_LSO);
                } else {
                    RegistrarCosto(&statsLSO.evocar_fracaso, metrica_celdas_LSO);
                }
                break;
        }
    }
    fclose(archivo);

    // --- TABLA DE RESULTADOS DE LAS OPERACIONES ---
    printf("\n---------------------------|--------------|");
    printf("\n                           |     LSOBB    |");
    printf("\n---------------------------|--------------|");

    printf("\nAlta                       |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.alta.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.alta.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.alta.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.alta.cantidad > 0 ? statsLSO.alta.costo_acu / statsLSO.alta.cantidad : 0);
    printf("\n---------------------------|--------------|");

    printf("\nBaja                       |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.baja.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.baja.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.baja.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.baja.cantidad > 0 ? statsLSO.baja.costo_acu / statsLSO.baja.cantidad : 0);
    printf("\n---------------------------|--------------|");

    printf("\nEvocar exitoso             |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.evocar_exito.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.evocar_exito.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.evocar_exito.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.evocar_exito.cantidad > 0 ? statsLSO.evocar_exito.costo_acu / statsLSO.evocar_exito.cantidad : 0);
    printf("\n---------------------------|--------------|");

    printf("\nEvocar fracaso             |");
    printf("\n  Cantidad                 | %10d   |", statsLSO.evocar_fracaso.cantidad);
    printf("\n  Costo Acumulado          | %12.2f |", statsLSO.evocar_fracaso.costo_acu);
    printf("\n  Costo Maximo             | %12.2f |", statsLSO.evocar_fracaso.costo_max);
    printf("\n  Costo Promedio           | %12.2f |", statsLSO.evocar_fracaso.cantidad > 0 ? statsLSO.evocar_fracaso.costo_acu / statsLSO.evocar_fracaso.cantidad : 0);
    printf("\n---------------------------|--------------|\n");

    // --- PRUEBA TEÓRICA DE LA FUNCIÓN "LOCALIZAR" ---
    EvaluarCostosLocalizar(&miLista);

    return 0;
}


