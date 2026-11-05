// UTN FRBA - Algoritmos y Estructuras de Datos - TP grupal
// Sistema de guia de apoyo para drones (sin GPS)

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

const int N = 200;
const bool PERMITIR_DIAGONALES = false;
const int LINEAS_POR_PAGINA = 20;
const int TAM_TEXTO = 256;

struct Orden
{
    unsigned int espera = 0;
    bool soltarGranada1 = false;
    bool soltarGranada2 = false;
    bool ataqueKamikaze = false;
    bool aterrizaje = false;
    bool despegue = false;
    int siguientex = 0;
    int siguientey = 0;
};

struct OrdenArchivo
{
    int x = 0;
    int y = 0;
    unsigned int espera = 0;
    bool soltarGranada1 = false;
    bool soltarGranada2 = false;
    bool ataqueKamikaze = false;
    bool aterrizaje = false;
    bool despegue = false;
    int siguientex = 0;
    int siguientey = 0;
};

// Se accede como orden[y][x] / hay[y][x].
struct Mapa
{
    Orden orden[N][N];
    bool hay[N][N];
    int cantidad = 0;

    Mapa() { limpiar(); }

    void limpiar()
    {
        for (int y = 0; y < N; y++)
            for (int x = 0; x < N; x++)
            {
                orden[y][x] = Orden();
                hay[y][x] = false;
            }
        cantidad = 0;
    }
};


Mapa memoria;
Mapa temporal;   // para leer y validar archivos antes de pasarlos a memoria


// Lee una linea completa del teclado (admite espacios) en 'destino'.
void leerLinea(char* destino, int tam)
{
    cin.getline(destino, tam);

    if (cin.eof() && strlen(destino) == 0)
    {
        cout << "\nEntrada cerrada. Fin del programa.\n";
        exit(0);
    }
    if (cin.fail() && !cin.eof())
    {
        cin.clear();
        cin.ignore(1000000, '\n');
    }

    int largo = (int)strlen(destino);
    if (largo > 0 && destino[largo - 1] == '\r')  // por si el texto viene con fin de linea de Windows
        destino[largo - 1] = '\0';
}

void leerTexto(const char* mensaje, char* destino, int tam)
{
    cout << mensaje;
    leerLinea(destino, tam);

    // Sacar comillas
    int largo = (int)strlen(destino);
    if (largo >= 2 && destino[0] == '"' && destino[largo - 1] == '"')
    {
        memmove(destino, destino + 1, largo - 2);
        destino[largo - 2] = '\0';
    }
}

// Convierte un texto a numero. Solo admite digitos. Devuelve false si el texto no es un numero.
bool convertirEntero(const char* texto, long long& valor)
{
    int i = 0;
    while (texto[i] == ' ') i++;
    if (texto[i] < '0' || texto[i] > '9') return false;

    valor = 0;
    while (texto[i] >= '0' && texto[i] <= '9')
    {
        valor = valor * 10 + (texto[i] - '0');
        if (valor > 99999999999LL) return false;
        i++;
    }
    while (texto[i] == ' ') i++;
    return texto[i] == '\0';
}

long long leerEntero(const char* mensaje, long long minimo, long long maximo)
{
    char buffer[TAM_TEXTO];
    long long valor;
    while (true)
    {
        cout << mensaje;
        leerLinea(buffer, TAM_TEXTO);
        if (convertirEntero(buffer, valor) && valor >= minimo && valor <= maximo)
            return valor;
        cout << "  Valor invalido. Ingrese un numero entre " << minimo << " y " << maximo << ".\n";
    }
}

bool leerSiNo(const char* mensaje)
{
    char buffer[TAM_TEXTO];
    while (true)
    {
        cout << mensaje << " (s/n): ";
        leerLinea(buffer, TAM_TEXTO);
        if (strcmp(buffer, "s") == 0 || strcmp(buffer, "S") == 0 || strcmp(buffer, "1") == 0)
            return true;
        if (strcmp(buffer, "n") == 0 || strcmp(buffer, "N") == 0 || strcmp(buffer, "0") == 0)
            return false;
        cout << "  Responda s o n.\n";
    }
}

bool existeArchivo(const char* ruta)
{
    FILE* f = fopen(ruta, "rb");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

bool enRango(int x, int y)
{
    return x >= 0 && x < N && y >= 0 && y < N;
}

Orden aOrden(const OrdenArchivo& r)
{
    Orden o;
    o.espera = r.espera;
    o.soltarGranada1 = r.soltarGranada1;
    o.soltarGranada2 = r.soltarGranada2;
    o.ataqueKamikaze = r.ataqueKamikaze;
    o.aterrizaje = r.aterrizaje;
    o.despegue = r.despegue;
    o.siguientex = r.siguientex;
    o.siguientey = r.siguientey;
    return o;
}

OrdenArchivo aRegistro(int x, int y, const Orden& o)
{
    OrdenArchivo r;
    r.x = x;
    r.y = y;
    r.espera = o.espera;
    r.soltarGranada1 = o.soltarGranada1;
    r.soltarGranada2 = o.soltarGranada2;
    r.ataqueKamikaze = o.ataqueKamikaze;
    r.aterrizaje = o.aterrizaje;
    r.despegue = o.despegue;
    r.siguientex = o.siguientex;
    r.siguientey = o.siguientey;
    return r;
}

bool esFinal(const Orden& o)
{
    return o.aterrizaje || o.ataqueKamikaze;
}

// Agrega un texto a 'destino', separado con " + " si ya habia algo.
void agregar(char* destino, const char* texto)
{
    if (destino[0] != '\0') strcat(destino, " + ");
    strcat(destino, texto);
}

// Describe las acciones respetando el orden de precedencia
void describir(const Orden& o, char* resultado)
{
    char parcial[64];
    resultado[0] = '\0';
    if (o.despegue) agregar(resultado, "DESPEGUE");
    if (o.aterrizaje) agregar(resultado, "ATERRIZAJE");
    if (o.soltarGranada1) agregar(resultado, "SOLTAR GRANADA 1");
    if (o.soltarGranada2) agregar(resultado, "SOLTAR GRANADA 2");
    if (o.ataqueKamikaze) agregar(resultado, "ATAQUE KAMIKAZE");
    if (o.espera > 0)
    {
        sprintf(parcial, "ESPERAR %u", o.espera);
        agregar(resultado, parcial);
    }
    if (!esFinal(o))
    {
        sprintf(parcial, "MOVER a (%d,%d)", o.siguientex, o.siguientey);
        agregar(resultado, parcial);
    }
}

void mostrarRegistro(const OrdenArchivo& r)
{
    cout << "  Registro en (x=" << r.x << ", y=" << r.y << "):\n";
    cout << "    despegue       : " << (r.despegue ? "si" : "no") << "\n";
    cout << "    aterrizaje     : " << (r.aterrizaje ? "si" : "no") << "\n";
    cout << "    soltarGranada1 : " << (r.soltarGranada1 ? "si" : "no") << "\n";
    cout << "    soltarGranada2 : " << (r.soltarGranada2 ? "si" : "no") << "\n";
    cout << "    ataqueKamikaze : " << (r.ataqueKamikaze ? "si" : "no") << "\n";
    cout << "    espera         : " << r.espera << "\n";
    cout << "    siguiente      : (" << r.siguientex << "," << r.siguientey << ")\n";
}

// Pide por teclado los datos de un registro (x e y ya vienen definidos).
OrdenArchivo pedirCampos(int x, int y)
{
    OrdenArchivo r;
    r.x = x;
    r.y = y;
    r.despegue = leerSiNo("  Despegue?");
    r.aterrizaje = leerSiNo("  Aterrizaje?");
    r.soltarGranada1 = leerSiNo("  Soltar granada 1?");
    r.soltarGranada2 = leerSiNo("  Soltar granada 2?");
    r.ataqueKamikaze = leerSiNo("  Ataque kamikaze?");
    r.espera = (unsigned int)leerEntero("  Espera (0 a 4294967295): ", 0, 4294967295LL);
    if (!esFinal(aOrden(r)))
    {
        r.siguientex = (int)leerEntero("  Siguiente x (0 a 199): ", 0, N - 1);
        r.siguientey = (int)leerEntero("  Siguiente y (0 a 199): ", 0, N - 1);
    }
    else
    {
        cout << "  (celda final: no se pide siguiente posicion)\n";
    }
    return r;
}


// Lee un archivo binario y lo ubica en un Mapa.
bool leerArchivoEnMapa(const char* ruta, Mapa& m, char* error)
{
    m.limpiar();

    FILE* f = fopen(ruta, "rb");
    if (f == NULL)
    {
        sprintf(error, "No se pudo abrir el archivo '%.200s'.", ruta);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long tamano = ftell(f);
    rewind(f);
    if (tamano % (long)sizeof(OrdenArchivo) != 0)
    {
        fclose(f);
        sprintf(error, "El archivo esta danado: su tamano no es multiplo del tamano del registro.");
        return false;
    }

    OrdenArchivo r;
    int numero = 0;
    while (fread(&r, sizeof(OrdenArchivo), 1, f) == 1)
    {
        numero++;
        if (!enRango(r.x, r.y))
        {
            sprintf(error, "El registro %d tiene coordenadas fuera de la tabla (%d,%d).", numero, r.x, r.y);
            fclose(f);
            return false;
        }
        if (m.hay[r.y][r.x])
        {
            sprintf(error, "Se repiten las coordenadas (%d,%d) (no se puede pasar dos veces por la misma celda).", r.x, r.y);
            fclose(f);
            return false;
        }
        m.orden[r.y][r.x] = aOrden(r);
        m.hay[r.y][r.x] = true;
        m.cantidad++;
    }
    fclose(f);
    return true;
}

bool validarMapa(const Mapa& m, char* error)
{
    if (m.cantidad == 0)
    {
        sprintf(error, "No hay registros.");
        return false;
    }

    int despegues = 0, finales = 0;
    int despegueX = -1, despegueY = -1;

    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            if (!m.hay[y][x]) continue;
            const Orden& o = m.orden[y][x];

            if (o.despegue)
            {
                despegues++;
                despegueX = x;
                despegueY = y;
                if (o.soltarGranada1 || o.soltarGranada2 || o.ataqueKamikaze || o.aterrizaje)
                {
                    sprintf(error, "La instruccion de despegue (%d,%d) no puede estar acompanada de ataques ni de aterrizaje.", x, y);
                    return false;
                }
            }
            if (o.aterrizaje && o.ataqueKamikaze)
            {
                sprintf(error, "La celda (%d,%d) tiene aterrizaje y kamikaze a la vez.", x, y);
                return false;
            }
            if (esFinal(o)) finales++;
        }

    if (despegues != 1)
    {
        sprintf(error, "Debe haber una unica instruccion de despegue (hay %d).", despegues);
        return false;
    }
    if (finales != 1)
    {
        sprintf(error, "Debe haber una unica instruccion final: aterrizaje o ataque kamikaze (hay %d).", finales);
        return false;
    }

    bool visitado[N][N] = {};
    int x = despegueX, y = despegueY;
    int pasos = 0;

    while (true)
    {
        if (visitado[y][x])
        {
            sprintf(error, "La ruta pasa mas de una vez por (%d,%d).", x, y);
            return false;
        }
        visitado[y][x] = true;
        pasos++;

        const Orden& o = m.orden[y][x];
        if (esFinal(o)) break;  // fin de la operacion

        int nx = o.siguientex, ny = o.siguientey;
        if (!enRango(nx, ny))
        {
            sprintf(error, "La celda (%d,%d) apunta fuera de la tabla (%d,%d).", x, y, nx, ny);
            return false;
        }
        int difx = abs(nx - x), dify = abs(ny - y);
        int mayor = (difx > dify) ? difx : dify;
        bool aledania = PERMITIR_DIAGONALES ? (mayor == 1) : (difx + dify == 1);
        if (!aledania)
        {
            sprintf(error, "La celda (%d,%d) apunta a (%d,%d), que no es una celda aledana (hueco en la ruta).", x, y, nx, ny);
            return false;
        }
        if (!m.hay[ny][nx])
        {
            sprintf(error, "La ruta llega a (%d,%d) pero no hay registro ahi (hueco en la ruta).", nx, ny);
            return false;
        }
        x = nx;
        y = ny;
    }

    if (pasos != m.cantidad)
    {
        sprintf(error, "%d registro(s) no pertenecen a la ruta despegue -> final.", m.cantidad - pasos);
        return false;
    }
    return true;
}

// Lee un archivo y lo valida por completo. Devuelve true si cumple todo.
bool archivoEsValido(const char* ruta, char* error)
{
    return leerArchivoEnMapa(ruta, temporal, error) && validarMapa(temporal, error);
}

void controlarPagina(int& lineas)
{
    if (lineas >= LINEAS_POR_PAGINA)
    {
        char buffer[TAM_TEXTO];
        cout << "-- Presione ENTER para ver la siguiente pagina --";
        leerLinea(buffer, TAM_TEXTO);
        lineas = 0;
    }
    lineas++;
}

// Operacion 1: Cargar archivo de ataque en memoria
void opCargar()
{
    char ruta[TAM_TEXTO], error[TAM_TEXTO];

    cout << "\n--- Cargar archivo de ataque en memoria ---\n";
    leerTexto("Ruta y nombre del archivo: ", ruta, TAM_TEXTO);

    if (!archivoEsValido(ruta, error))
    {
        cout << "\nNO SE CARGO EL ARCHIVO. Motivo: " << error << "\n";
        return;
    }
    memoria = temporal;  // copia la tabla completa a la memoria del drone
    cout << "\nArchivo cargado correctamente: " << memoria.cantidad << " registros en memoria.\n";
}

// Operacion 2: Mostrar ataque cargado
void opMostrar()
{
    char texto[TAM_TEXTO];
    int lineas = 0;

    cout << "\n--- Ataque cargado en memoria ---\n";
    if (memoria.cantidad == 0)
    {
        cout << "No hay ningun ataque cargado en memoria.\n";
        return;
    }

    bool visto[N][N] = {};

    // Buscar el despegue
    int x0 = -1, y0 = -1;
    for (int y = 0; y < N && x0 == -1; y++)
        for (int x = 0; x < N; x++)
            if (memoria.hay[y][x] && memoria.orden[y][x].despegue)
            {
                x0 = x;
                y0 = y;
                break;
            }

    controlarPagina(lineas);
    cout << "Registros en memoria: " << memoria.cantidad << "\n";
    controlarPagina(lineas);
    cout << "Orden de ejecucion:\n";

    if (x0 != -1)
    {
        int paso = 1;
        int x = x0, y = y0;
        while (enRango(x, y) && memoria.hay[y][x] && !visto[y][x])
        {
            visto[y][x] = true;
            const Orden& o = memoria.orden[y][x];
            describir(o, texto);
            controlarPagina(lineas);
            cout << "  " << paso << ". (" << x << "," << y << ") -> " << texto << "\n";
            paso++;
            if (esFinal(o)) break;
            int nx = o.siguientex, ny = o.siguientey;
            x = nx;
            y = ny;
        }
    }
    else
    {
        controlarPagina(lineas);
        cout << "  (no hay celda de despegue en memoria)\n";
    }

    // Registros que no pertenecen a la ruta (pueden aparecer tras corregir en memoria)
    bool encabezado = false;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            if (memoria.hay[y][x] && !visto[y][x])
            {
                if (!encabezado)
                {
                    controlarPagina(lineas);
                    cout << "Registros fuera de la ruta:\n";
                    encabezado = true;
                }
                describir(memoria.orden[y][x], texto);
                controlarPagina(lineas);
                cout << "  (" << x << "," << y << ") -> " << texto << "\n";
            }

    controlarPagina(lineas);
    cout << "--- Fin del listado ---\n";
}

// Operacion 3: Crear un archivo de ataque nuevo
void opCrear()
{
    char ruta[TAM_TEXTO], error[TAM_TEXTO];

    cout << "\n--- Crear un archivo de ataque nuevo ---\n";
    leerTexto("Ruta y nombre del archivo a crear: ", ruta, TAM_TEXTO);

    if (existeArchivo(ruta) && !leerSiNo("El archivo ya existe. Desea sobrescribirlo?"))
    {
        cout << "Operacion cancelada.\n";
        return;
    }
    FILE* f = fopen(ruta, "wb");
    if (f == NULL)
    {
        cout << "No se pudo crear el archivo '" << ruta << "'.\n";
        return;
    }

    bool usado[N][N] = {};
    int cantidad = 0;
    do
    {
        cout << "\nRegistro #" << (cantidad + 1) << "\n";

        int x, y;
        while (true)
        {
            x = (int)leerEntero("  x (0 a 199): ", 0, N - 1);
            y = (int)leerEntero("  y (0 a 199): ", 0, N - 1);
            if (!usado[y][x]) break;
            cout << "  Ya ingreso un registro en (" << x << "," << y << "). No se puede repetir la celda.\n";
        }

        OrdenArchivo r = pedirCampos(x, y);
        fwrite(&r, sizeof(OrdenArchivo), 1, f);
        usado[y][x] = true;
        cantidad++;
    } while (leerSiNo("\nIngresar otro registro?"));
    fclose(f);

    cout << "\nArchivo creado con " << cantidad << " registro(s).\n";

    // Informar si el archivo cumple las reglas
    if (archivoEsValido(ruta, error))
        cout << "El ataque cumple todas las reglas de validacion.\n";
    else
        cout << "AVISO: el ataque todavia NO cumple las reglas: " << error << "\n";
}

// Operacion 4: Corregir un registro del archivo
void opCorregirArchivo()
{
    char ruta[TAM_TEXTO], error[TAM_TEXTO];

    cout << "\n--- Corregir un registro del archivo ---\n";
    leerTexto("Ruta y nombre del archivo: ", ruta, TAM_TEXTO);

    FILE* f = fopen(ruta, "r+b");  // lectura y escritura, binario
    if (f == NULL)
    {
        cout << "No se pudo abrir el archivo '" << ruta << "'.\n";
        return;
    }

    int x = (int)leerEntero("  x del registro a corregir (0 a 199): ", 0, N - 1);
    int y = (int)leerEntero("  y del registro a corregir (0 a 199): ", 0, N - 1);

    OrdenArchivo r;
    long posicion = 0;
    bool encontrado = false;
    while (!encontrado)
    {
        long actual = ftell(f);
        if (fread(&r, sizeof(OrdenArchivo), 1, f) != 1) break;
        if (r.x == x && r.y == y)
        {
            posicion = actual;
            encontrado = true;
        }
    }
    if (!encontrado)
    {
        cout << "No existe un registro en (" << x << "," << y << ") dentro del archivo.\n";
        fclose(f);
        return;
    }

    cout << "Registro actual:\n";
    mostrarRegistro(r);
    cout << "Ingrese los nuevos datos:\n";
    OrdenArchivo nuevo = pedirCampos(x, y);

    fseek(f, posicion, SEEK_SET);  // volver al inicio del registro y pisarlo
    fwrite(&nuevo, sizeof(OrdenArchivo), 1, f);
    fclose(f);
    cout << "Registro reemplazado en el archivo.\n";

    if (archivoEsValido(ruta, error))
        cout << "El archivo sigue cumpliendo las reglas de validacion.\n";
    else
        cout << "AVISO: el archivo ahora NO cumple las reglas: " << error << "\n";
}

// Operacion 5: Corregir un registro en memoria
void opCorregirMemoria()
{
    char error[TAM_TEXTO];

    cout << "\n--- Corregir un registro en memoria ---\n";
    if (memoria.cantidad == 0)
    {
        cout << "No hay ningun ataque cargado en memoria.\n";
        return;
    }
    int x = (int)leerEntero("  x del registro a corregir (0 a 199): ", 0, N - 1);
    int y = (int)leerEntero("  y del registro a corregir (0 a 199): ", 0, N - 1);
    if (!memoria.hay[y][x])
    {
        cout << "No hay un registro en memoria en (" << x << "," << y << ").\n";
        return;
    }

    cout << "Registro actual:\n";
    mostrarRegistro(aRegistro(x, y, memoria.orden[y][x]));
    cout << "Ingrese los nuevos datos:\n";
    OrdenArchivo nuevo = pedirCampos(x, y);
    memoria.orden[y][x] = aOrden(nuevo);
    cout << "Registro reemplazado en memoria.\n";

    if (validarMapa(memoria, error))
        cout << "El ataque en memoria sigue cumpliendo las reglas de validacion.\n";
    else
        cout << "AVISO: el ataque en memoria ahora NO cumple las reglas: " << error << "\n";
}

// Operacion 6: Guardar memoria en un archivo nuevo
void opGuardar()
{
    char ruta[TAM_TEXTO], error[TAM_TEXTO];

    cout << "\n--- Guardar memoria en un archivo nuevo ---\n";
    if (memoria.cantidad == 0)
    {
        cout << "No hay ningun ataque cargado en memoria.\n";
        return;
    }
    leerTexto("Ruta y nombre del archivo a crear: ", ruta, TAM_TEXTO);
    if (existeArchivo(ruta) && !leerSiNo("El archivo ya existe. Desea sobrescribirlo?"))
    {
        cout << "Operacion cancelada.\n";
        return;
    }

    if (!validarMapa(memoria, error))
    {
        cout << "AVISO: el ataque en memoria no cumple las reglas: " << error << "\n";
        if (!leerSiNo("Desea guardarlo de todos modos?"))
        {
            cout << "Operacion cancelada.\n";
            return;
        }
    }

    FILE* f = fopen(ruta, "wb");
    if (f == NULL)
    {
        cout << "No se pudo crear el archivo '" << ruta << "'.\n";
        return;
    }
    int guardados = 0;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            if (memoria.hay[y][x])
            {
                OrdenArchivo r = aRegistro(x, y, memoria.orden[y][x]);
                fwrite(&r, sizeof(OrdenArchivo), 1, f);
                guardados++;
            }
    fclose(f);
    cout << "Se guardaron " << guardados << " registros en '" << ruta << "'.\n";
}

// Operacion 7: Visualizar en HTML
const char* flecha(int x, int y, const Orden& o)
{
    int dx = o.siguientex - x, dy = o.siguientey - y;
    if (dx == 1 && dy == 0) return "&rarr;";
    if (dx == -1 && dy == 0) return "&larr;";
    if (dx == 0 && dy == 1) return "&darr;";
    if (dx == 0 && dy == -1) return "&uarr;";
    if (dx == 1 && dy == -1) return "&#8599;";
    if (dx == 1 && dy == 1) return "&#8600;";
    if (dx == -1 && dy == 1) return "&#8601;";
    if (dx == -1 && dy == -1) return "&#8598;";
    return "?";
}

const char* sino(bool valor)
{
    return valor ? "si" : "no";
}

// El HTML es un archivo de texto: se escribe con fopen / fprintf / fclose (Anexo 2).
// Es minimalista: la tabla de 200x200 y, debajo, el registro de cada orden.
bool generarHTML(const Mapa& m, const char* ruta, const char* titulo)
{
    FILE* f = fopen(ruta, "w");
    if (f == NULL)
    {
        cout << "No se pudo crear el archivo '" << ruta << "'.\n";
        return false;
    }

    fprintf(f, "<html>\n");
    fprintf(f, "<head><title>Ataque</title></head>\n");
    fprintf(f, "<body>\n");
    fprintf(f, "<p>Origen: %s - registros: %d (x = columna, y = fila)</p>\n", titulo, m.cantidad);
    fprintf(f, "<p>D = despegue, A = aterrizaje, K = kamikaze, G1 / G2 = granadas, "
               "E# = espera, flecha = hacia donde sigue el dron</p>\n");

    // Tabla de 200 x 200
    fprintf(f, "<table border=\"1\">\n");
    fprintf(f, "<tr><th></th>");
    for (int x = 0; x < N; x++) fprintf(f, "<th>%d</th>", x);
    fprintf(f, "</tr>\n");

    char texto[TAM_TEXTO], parcial[64];
    for (int y = 0; y < N; y++)
    {
        fprintf(f, "<tr><th>%d</th>", y);
        for (int x = 0; x < N; x++)
        {
            fprintf(f, "<td>");
            if (m.hay[y][x])
            {
                const Orden& o = m.orden[y][x];
                texto[0] = '\0';
                if (o.despegue) strcat(texto, "D ");
                if (o.aterrizaje) strcat(texto, "A ");
                if (o.soltarGranada1) strcat(texto, "G1 ");
                if (o.soltarGranada2) strcat(texto, "G2 ");
                if (o.ataqueKamikaze) strcat(texto, "K ");
                if (o.espera > 0)
                {
                    sprintf(parcial, "E%u ", o.espera);
                    strcat(texto, parcial);
                }
                if (!esFinal(o)) strcat(texto, flecha(x, y, o));
                fprintf(f, "%s", texto);
            }
            fprintf(f, "</td>");
        }
        fprintf(f, "</tr>\n");
    }
    fprintf(f, "</table>\n");

    // Registro de cada orden, debajo de la tabla
    fprintf(f, "<h2>Registro</h2>\n");
    fprintf(f, "<table border=\"1\">\n");
    fprintf(f, "<tr><th>x</th><th>y</th><th>espera</th><th>soltarGranada1</th><th>soltarGranada2</th>"
               "<th>ataqueKamikaze</th><th>aterrizaje</th><th>despegue</th>"
               "<th>siguientex</th><th>siguientey</th></tr>\n");
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            if (m.hay[y][x])
            {
                const Orden& o = m.orden[y][x];
                fprintf(f, "<tr><td>%d</td><td>%d</td><td>%u</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td>",
                        x, y, o.espera, sino(o.soltarGranada1), sino(o.soltarGranada2),
                        sino(o.ataqueKamikaze), sino(o.aterrizaje), sino(o.despegue));
                if (esFinal(o))
                    fprintf(f, "<td>-</td><td>-</td></tr>\n");   // celda final: no hay siguiente
                else
                    fprintf(f, "<td>%d</td><td>%d</td></tr>\n", o.siguientex, o.siguientey);
            }
    fprintf(f, "</table>\n");

    fprintf(f, "</body>\n");
    fprintf(f, "</html>\n");
    fclose(f);
    return true;
}

void opHTML()
{
    char rutaEntrada[TAM_TEXTO], rutaSalida[TAM_TEXTO], error[TAM_TEXTO];

    cout << "\n--- Visualizar ataque en HTML ---\n";
    cout << "  1. Visualizar el ataque cargado en memoria\n";
    cout << "  2. Visualizar un archivo de ataque\n";
    int fuente = (int)leerEntero("Opcion: ", 1, 2);

    const Mapa* origen = &memoria;
    const char* titulo = "memoria";

    if (fuente == 1)
    {
        if (memoria.cantidad == 0)
        {
            cout << "No hay ningun ataque cargado en memoria.\n";
            return;
        }
    }
    else
    {
        leerTexto("Ruta y nombre del archivo de ataque: ", rutaEntrada, TAM_TEXTO);
        if (!leerArchivoEnMapa(rutaEntrada, temporal, error))
        {
            cout << error << "\n";
            return;
        }
        origen = &temporal;
        titulo = rutaEntrada;
    }

    leerTexto("Ruta y nombre del HTML a generar (ej: ataque.html): ", rutaSalida, TAM_TEXTO);
    if (generarHTML(*origen, rutaSalida, titulo))
        cout << "HTML generado en '" << rutaSalida << "'. Abralo con el navegador.\n";
}

// Menu principal
void mostrarMenu()
{
    cout << "\n=========== GUIA DE APOYO PARA DRONES ===========\n";
    cout << " 1. Cargar archivo de ataque en memoria\n";
    cout << " 2. Mostrar ataque cargado\n";
    cout << " 3. Crear un archivo de ataque nuevo\n";
    cout << " 4. Corregir un registro del archivo\n";
    cout << " 5. Corregir un registro en memoria\n";
    cout << " 6. Guardar memoria en un archivo nuevo\n";
    cout << " 7. Visualizar un archivo de ataque en html\n";
    cout << " 0. Salir\n";
    cout << "==================================================\n";
}

int main()
{
    int opcion;
    do
    {
        mostrarMenu();
        opcion = (int)leerEntero("Seleccione una opcion: ", 0, 7);
        switch (opcion)
        {
            case 1: opCargar(); break;
            case 2: opMostrar(); break;
            case 3: opCrear(); break;
            case 4: opCorregirArchivo(); break;
            case 5: opCorregirMemoria(); break;
            case 6: opGuardar(); break;
            case 7: opHTML(); break;
            case 0: cout << "Hasta luego.\n"; break;
        }
    } while (opcion != 0);
    return 0;
}
