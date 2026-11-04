// ============================================================================
// UTN FRBA - Algoritmos y Estructuras de Datos - TP grupal
// Sistema de guia de apoyo para drones (sin GPS)
//
// Compilar:   g++ -std=c++17 -Wall -o drones main.cpp
// Ejecutar:   ./drones        (en Windows: drones.exe)
//
// Convenciones / hipotesis (ponerlas tambien en el informe):
//  - Coordenadas: x = columna, y = fila, ambas de 0 a 199.
//  - Los archivos de ataque son BINARIOS: una secuencia de OrdenArchivo.
//  - "Celdas aledanas": por defecto solo arriba/abajo/izquierda/derecha.
//    Para permitir diagonales cambiar PERMITIR_DIAGONALES a true.
//  - La ruta valida arranca en la celda de despegue y sigue (siguientex,
//    siguientey) hasta una celda con aterrizaje o ataqueKamikaze.
//  - Hay exactamente UNA celda final (aterrizaje o kamikaze, no ambos).
//  - Todos los registros del archivo deben pertenecer a la ruta (un registro
//    suelto que no se alcanza desde el despegue se considera un "hueco").
//  - Los mensajes se escriben sin tildes para que se vean bien en consola.
// ============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>
#include <algorithm>

using namespace std;

// ----------------------------------------------------------------------------
// Constantes
// ----------------------------------------------------------------------------
const int N = 200;                       // tabla de 200 x 200
const bool PERMITIR_DIAGONALES = false;  // ver hipotesis
const int LINEAS_POR_PAGINA = 20;        // para el paginado de la operacion 2

// ----------------------------------------------------------------------------
// Estructuras
// ----------------------------------------------------------------------------
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

// Memoria del drone: tabla de 200x200 ordenes + marca de "hay orden cargada".
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

// La memoria del drone (global, para no desbordar la pila: ~850 KB)
static Mapa memoria;

// ----------------------------------------------------------------------------
// Entrada de datos (robusta ante entradas invalidas)
// ----------------------------------------------------------------------------
string leerLinea()
{
    string s;
    if (!getline(cin, s))
    {
        cout << "\nEntrada cerrada. Fin del programa.\n";
        exit(0);
    }
    return s;
}

string leerTexto(const string& mensaje)
{
    cout << mensaje;
    string s = leerLinea();
    // Quitar comillas (Windows las agrega al "copiar como ruta")
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);
    return s;
}

long long leerEntero(const string& mensaje, long long minimo, long long maximo)
{
    while (true)
    {
        cout << mensaje;
        string s = leerLinea();
        istringstream ss(s);
        long long v;
        if ((ss >> v) && (ss >> ws).eof() && v >= minimo && v <= maximo)
            return v;
        cout << "  Valor invalido. Ingrese un numero entre " << minimo
             << " y " << maximo << ".\n";
    }
}

bool leerSiNo(const string& mensaje)
{
    while (true)
    {
        cout << mensaje << " (s/n): ";
        string s = leerLinea();
        if (s == "s" || s == "S" || s == "1") return true;
        if (s == "n" || s == "N" || s == "0") return false;
        cout << "  Responda s o n.\n";
    }
}

bool existeArchivo(const string& ruta)
{
    ifstream f(ruta, ios::binary);
    return f.good();
}

// ----------------------------------------------------------------------------
// Conversiones y utilidades
// ----------------------------------------------------------------------------
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
    OrdenArchivo r{};
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

// Describe las acciones respetando el orden de precedencia:
// despegue, aterrizaje, granada 1, granada 2, kamikaze, esperar, mover.
string describir(const Orden& o)
{
    vector<string> acciones;
    if (o.despegue) acciones.push_back("DESPEGUE");
    if (o.aterrizaje) acciones.push_back("ATERRIZAJE");
    if (o.soltarGranada1) acciones.push_back("SOLTAR GRANADA 1");
    if (o.soltarGranada2) acciones.push_back("SOLTAR GRANADA 2");
    if (o.ataqueKamikaze) acciones.push_back("ATAQUE KAMIKAZE");
    if (o.espera > 0) acciones.push_back("ESPERAR " + to_string(o.espera));
    if (!esFinal(o))
        acciones.push_back("MOVER a (" + to_string(o.siguientex) + "," +
                           to_string(o.siguientey) + ")");

    string s;
    for (size_t i = 0; i < acciones.size(); i++)
    {
        if (i > 0) s += " + ";
        s += acciones[i];
    }
    return s;
}

void mostrarRegistro(const OrdenArchivo& r)
{
    cout << "  Registro en (x=" << r.x << ", y=" << r.y << "):\n"
         << "    despegue       : " << (r.despegue ? "si" : "no") << "\n"
         << "    aterrizaje     : " << (r.aterrizaje ? "si" : "no") << "\n"
         << "    soltarGranada1 : " << (r.soltarGranada1 ? "si" : "no") << "\n"
         << "    soltarGranada2 : " << (r.soltarGranada2 ? "si" : "no") << "\n"
         << "    ataqueKamikaze : " << (r.ataqueKamikaze ? "si" : "no") << "\n"
         << "    espera         : " << r.espera << "\n"
         << "    siguiente      : (" << r.siguientex << "," << r.siguientey << ")\n";
}

// Pide por teclado los datos de un registro (x e y ya vienen definidos).
OrdenArchivo pedirCampos(int x, int y)
{
    OrdenArchivo r{};
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

// ----------------------------------------------------------------------------
// Archivos
// ----------------------------------------------------------------------------
// Lee todos los registros de un archivo binario.
bool leerArchivo(const string& ruta, vector<OrdenArchivo>& regs, string& error)
{
    ifstream f(ruta, ios::binary);
    if (!f)
    {
        error = "No se pudo abrir el archivo '" + ruta + "'.";
        return false;
    }
    OrdenArchivo r{};
    while (f.read(reinterpret_cast<char*>(&r), sizeof(r)))
        regs.push_back(r);
    if (f.gcount() != 0)
    {
        error = "El archivo esta danado: su tamano no es multiplo del registro.";
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Validacion del ataque (reglas de la consigna)
// ----------------------------------------------------------------------------
string coord(int x, int y)
{
    return "(" + to_string(x) + "," + to_string(y) + ")";
}

bool validar(const vector<OrdenArchivo>& regs, string& error)
{
    if (regs.empty())
    {
        error = "No hay registros.";
        return false;
    }

    vector<int> indice(N * N, -1);  // celda -> posicion del registro
    int despegues = 0, finales = 0, posDespegue = -1;

    for (size_t i = 0; i < regs.size(); i++)
    {
        const OrdenArchivo& r = regs[i];

        if (!enRango(r.x, r.y))
        {
            error = "El registro " + to_string(i + 1) + " tiene coordenadas fuera de la tabla " + coord(r.x, r.y) + ".";
            return false;
        }
        int c = r.y * N + r.x;
        if (indice[c] != -1)
        {
            error = "Se repiten las coordenadas " + coord(r.x, r.y) + " (no se puede pasar dos veces por la misma celda).";
            return false;
        }
        indice[c] = (int)i;

        if (r.despegue)
        {
            despegues++;
            posDespegue = (int)i;
            if (r.soltarGranada1 || r.soltarGranada2 || r.ataqueKamikaze || r.aterrizaje)
            {
                error = "La instruccion de despegue " + coord(r.x, r.y) + " no puede estar acompanada de ataques ni de aterrizaje.";
                return false;
            }
        }
        if (r.aterrizaje && r.ataqueKamikaze)
        {
            error = "La celda " + coord(r.x, r.y) + " tiene aterrizaje y kamikaze a la vez.";
            return false;
        }
        if (r.aterrizaje || r.ataqueKamikaze) finales++;
    }

    if (despegues != 1)
    {
        error = "Debe haber una unica instruccion de despegue (hay " + to_string(despegues) + ").";
        return false;
    }
    if (finales != 1)
    {
        error = "Debe haber una unica instruccion final: aterrizaje o ataque kamikaze (hay " + to_string(finales) + ").";
        return false;
    }

    // Recorrer la ruta desde el despegue hasta la celda final
    vector<bool> visitado(N * N, false);
    int actual = posDespegue;
    size_t pasos = 0;

    while (true)
    {
        const OrdenArchivo& r = regs[actual];
        int c = r.y * N + r.x;
        if (visitado[c])
        {
            error = "La ruta pasa mas de una vez por " + coord(r.x, r.y) + ".";
            return false;
        }
        visitado[c] = true;
        pasos++;

        if (r.aterrizaje || r.ataqueKamikaze) break;  // fin de la operacion

        int nx = r.siguientex, ny = r.siguientey;
        if (!enRango(nx, ny))
        {
            error = "La celda " + coord(r.x, r.y) + " apunta fuera de la tabla " + coord(nx, ny) + ".";
            return false;
        }
        int dx = abs(nx - r.x), dy = abs(ny - r.y);
        bool aledania = PERMITIR_DIAGONALES ? (max(dx, dy) == 1) : (dx + dy == 1);
        if (!aledania)
        {
            error = "La celda " + coord(r.x, r.y) + " apunta a " + coord(nx, ny) + ", que no es una celda aledana (hueco en la ruta).";
            return false;
        }
        int sig = indice[ny * N + nx];
        if (sig == -1)
        {
            error = "La ruta llega a " + coord(nx, ny) + " pero no hay registro ahi (hueco en la ruta).";
            return false;
        }
        actual = sig;
    }

    if (pasos != regs.size())
    {
        error = to_string(regs.size() - pasos) + " registro(s) no pertenecen a la ruta despegue -> final.";
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Memoria <-> registros
// ----------------------------------------------------------------------------
vector<OrdenArchivo> memoriaARegistros()
{
    vector<OrdenArchivo> regs;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            if (memoria.hay[y][x])
                regs.push_back(aRegistro(x, y, memoria.orden[y][x]));
    return regs;
}

void cargarRegistros(Mapa& m, const vector<OrdenArchivo>& regs)
{
    m.limpiar();
    for (const OrdenArchivo& r : regs)
    {
        if (!enRango(r.x, r.y)) continue;
        if (!m.hay[r.y][r.x]) m.cantidad++;
        m.orden[r.y][r.x] = aOrden(r);
        m.hay[r.y][r.x] = true;
    }
}

// ----------------------------------------------------------------------------
// Paginado
// ----------------------------------------------------------------------------
struct Paginador
{
    int impresas = 0;
    void linea(const string& s)
    {
        if (impresas >= LINEAS_POR_PAGINA)
        {
            cout << "-- Presione ENTER para ver la siguiente pagina --";
            leerLinea();
            impresas = 0;
        }
        cout << s << "\n";
        impresas++;
    }
};

// ----------------------------------------------------------------------------
// Operacion 1: Cargar archivo de ataque en memoria
// ----------------------------------------------------------------------------
void opCargar()
{
    cout << "\n--- Cargar archivo de ataque en memoria ---\n";
    string ruta = leerTexto("Ruta y nombre del archivo: ");

    vector<OrdenArchivo> regs;
    string error;
    if (!leerArchivo(ruta, regs, error) || !validar(regs, error))
    {
        cout << "\nNO SE CARGO EL ARCHIVO. Motivo: " << error << "\n";
        return;
    }
    cargarRegistros(memoria, regs);
    cout << "\nArchivo cargado correctamente: " << memoria.cantidad << " registros en memoria.\n";
}

// ----------------------------------------------------------------------------
// Operacion 2: Mostrar ataque cargado
// ----------------------------------------------------------------------------
void opMostrar()
{
    cout << "\n--- Ataque cargado en memoria ---\n";
    if (memoria.cantidad == 0)
    {
        cout << "No hay ningun ataque cargado en memoria.\n";
        return;
    }

    Paginador p;
    vector<bool> visto(N * N, false);

    // Buscar el despegue
    int dx0 = -1, dy0 = -1;
    for (int y = 0; y < N && dx0 == -1; y++)
        for (int x = 0; x < N; x++)
            if (memoria.hay[y][x] && memoria.orden[y][x].despegue)
            {
                dx0 = x;
                dy0 = y;
                break;
            }

    p.linea("Registros en memoria: " + to_string(memoria.cantidad));
    p.linea("Orden de ejecucion:");

    int paso = 1;
    if (dx0 != -1)
    {
        int x = dx0, y = dy0;
        while (enRango(x, y) && memoria.hay[y][x] && !visto[y * N + x])
        {
            visto[y * N + x] = true;
            const Orden& o = memoria.orden[y][x];
            p.linea("  " + to_string(paso++) + ". " + coord(x, y) + " -> " + describir(o));
            if (esFinal(o)) break;
            int nx = o.siguientex, ny = o.siguientey;
            x = nx;
            y = ny;
        }
    }
    else
    {
        p.linea("  (no hay celda de despegue en memoria)");
    }

    // Registros que no pertenecen a la ruta (pueden aparecer tras corregir en memoria)
    bool encabezado = false;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            if (memoria.hay[y][x] && !visto[y * N + x])
            {
                if (!encabezado)
                {
                    p.linea("Registros fuera de la ruta:");
                    encabezado = true;
                }
                p.linea("  " + coord(x, y) + " -> " + describir(memoria.orden[y][x]));
            }
    p.linea("--- Fin del listado ---");
}

// ----------------------------------------------------------------------------
// Operacion 3: Crear un archivo de ataque nuevo
// ----------------------------------------------------------------------------
void opCrear()
{
    cout << "\n--- Crear un archivo de ataque nuevo ---\n";
    string ruta = leerTexto("Ruta y nombre del archivo a crear: ");

    if (existeArchivo(ruta) && !leerSiNo("El archivo ya existe. Desea sobrescribirlo?"))
    {
        cout << "Operacion cancelada.\n";
        return;
    }
    ofstream f(ruta, ios::binary | ios::trunc);
    if (!f)
    {
        cout << "No se pudo crear el archivo '" << ruta << "'.\n";
        return;
    }

    vector<bool> usado(N * N, false);
    int cantidad = 0;
    do
    {
        cout << "\nRegistro #" << (cantidad + 1) << "\n";
        int x = (int)leerEntero("  x (0 a 199): ", 0, N - 1);
        int y = (int)leerEntero("  y (0 a 199): ", 0, N - 1);
        if (usado[y * N + x])
        {
            cout << "  Ya ingreso un registro en " << coord(x, y) << ". No se puede repetir la celda.\n";
            continue;
        }
        OrdenArchivo r = pedirCampos(x, y);
        f.write(reinterpret_cast<const char*>(&r), sizeof(r));
        usado[y * N + x] = true;
        cantidad++;
    } while (leerSiNo("\nIngresar otro registro?"));
    f.close();

    cout << "\nArchivo creado con " << cantidad << " registro(s).\n";

    // Informar si el archivo cumple las reglas
    vector<OrdenArchivo> regs;
    string error;
    if (leerArchivo(ruta, regs, error) && validar(regs, error))
        cout << "El ataque cumple todas las reglas de validacion.\n";
    else
        cout << "AVISO: el ataque todavia NO cumple las reglas: " << error << "\n";
}

// ----------------------------------------------------------------------------
// Operacion 4: Corregir un registro del archivo
// ----------------------------------------------------------------------------
void opCorregirArchivo()
{
    cout << "\n--- Corregir un registro del archivo ---\n";
    string ruta = leerTexto("Ruta y nombre del archivo: ");
    fstream f(ruta, ios::in | ios::out | ios::binary);
    if (!f)
    {
        cout << "No se pudo abrir el archivo '" << ruta << "'.\n";
        return;
    }

    int x = (int)leerEntero("  x del registro a corregir (0 a 199): ", 0, N - 1);
    int y = (int)leerEntero("  y del registro a corregir (0 a 199): ", 0, N - 1);

    OrdenArchivo r{};
    streampos posicion;
    bool encontrado = false;
    while (true)
    {
        streampos actual = f.tellg();
        if (!f.read(reinterpret_cast<char*>(&r), sizeof(r))) break;
        if (r.x == x && r.y == y)
        {
            posicion = actual;
            encontrado = true;
            break;
        }
    }
    if (!encontrado)
    {
        cout << "No existe un registro en " << coord(x, y) << " dentro del archivo.\n";
        return;
    }

    cout << "Registro actual:\n";
    mostrarRegistro(r);
    cout << "Ingrese los nuevos datos:\n";
    OrdenArchivo nuevo = pedirCampos(x, y);

    f.clear();
    f.seekp(posicion);
    f.write(reinterpret_cast<const char*>(&nuevo), sizeof(nuevo));
    f.close();
    cout << "Registro reemplazado en el archivo.\n";

    vector<OrdenArchivo> regs;
    string error;
    if (leerArchivo(ruta, regs, error) && validar(regs, error))
        cout << "El archivo sigue cumpliendo las reglas de validacion.\n";
    else
        cout << "AVISO: el archivo ahora NO cumple las reglas: " << error << "\n";
}

// ----------------------------------------------------------------------------
// Operacion 5: Corregir un registro en memoria
// ----------------------------------------------------------------------------
void opCorregirMemoria()
{
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
        cout << "No hay un registro en memoria en " << coord(x, y) << ".\n";
        return;
    }

    cout << "Registro actual:\n";
    mostrarRegistro(aRegistro(x, y, memoria.orden[y][x]));
    cout << "Ingrese los nuevos datos:\n";
    OrdenArchivo nuevo = pedirCampos(x, y);
    memoria.orden[y][x] = aOrden(nuevo);
    cout << "Registro reemplazado en memoria.\n";

    string error;
    if (validar(memoriaARegistros(), error))
        cout << "El ataque en memoria sigue cumpliendo las reglas de validacion.\n";
    else
        cout << "AVISO: el ataque en memoria ahora NO cumple las reglas: " << error << "\n";
}

// ----------------------------------------------------------------------------
// Operacion 6: Guardar memoria en un archivo nuevo
// ----------------------------------------------------------------------------
void opGuardar()
{
    cout << "\n--- Guardar memoria en un archivo nuevo ---\n";
    if (memoria.cantidad == 0)
    {
        cout << "No hay ningun ataque cargado en memoria.\n";
        return;
    }
    string ruta = leerTexto("Ruta y nombre del archivo a crear: ");
    if (existeArchivo(ruta) && !leerSiNo("El archivo ya existe. Desea sobrescribirlo?"))
    {
        cout << "Operacion cancelada.\n";
        return;
    }

    vector<OrdenArchivo> regs = memoriaARegistros();
    string error;
    if (!validar(regs, error))
    {
        cout << "AVISO: el ataque en memoria no cumple las reglas: " << error << "\n";
        if (!leerSiNo("Desea guardarlo de todos modos?"))
        {
            cout << "Operacion cancelada.\n";
            return;
        }
    }

    ofstream f(ruta, ios::binary | ios::trunc);
    if (!f)
    {
        cout << "No se pudo crear el archivo '" << ruta << "'.\n";
        return;
    }
    for (const OrdenArchivo& r : regs)
        f.write(reinterpret_cast<const char*>(&r), sizeof(r));
    f.close();
    cout << "Se guardaron " << regs.size() << " registros en '" << ruta << "'.\n";
}

// ----------------------------------------------------------------------------
// Operacion 7: Visualizar en HTML
// ----------------------------------------------------------------------------
string flecha(int x, int y, const Orden& o)
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

bool generarHTML(const Mapa& m, const string& ruta, const string& titulo)
{
    ofstream f(ruta);
    if (!f)
    {
        cout << "No se pudo crear el archivo '" << ruta << "'.\n";
        return false;
    }

    f << "<html>\n<head>\n<meta charset=\"utf-8\">\n<title>Ataque</title>\n"
      << "<style>\n"
      << "body{font-family:sans-serif;}\n"
      << "table{border-collapse:collapse;}\n"
      << "td,th{border:1px solid #bbb;width:26px;height:22px;font-size:9px;text-align:center;padding:0;}\n"
      << "th{background:#eee;}\n"
      << ".d{background:#ffd966;font-weight:bold;}\n"   // despegue
      << ".a{background:#9fc5e8;font-weight:bold;}\n"   // aterrizaje
      << ".k{background:#ea4335;color:white;font-weight:bold;}\n"  // kamikaze
      << ".g{background:#f6b26b;}\n"                    // granadas
      << ".e{background:#fff2cc;}\n"                    // espera
      << ".m{background:#d9ead3;}\n"                    // movimiento
      << "</style>\n</head>\n<body>\n";
    f << "<h1>Vista del ataque</h1>\n";
    f << "<p>Origen: " << titulo << " - registros: " << m.cantidad << " - tabla de "
      << N << "x" << N << " (x = columna, y = fila)</p>\n";
    f << "<p>D = despegue, A = aterrizaje, K = kamikaze, G1/G2 = granadas, E# = espera, "
      << "flechas = hacia donde sigue el dron.</p>\n";

    f << "<table>\n<tr><th></th>";
    for (int x = 0; x < N; x++) f << "<th>" << x << "</th>";
    f << "</tr>\n";

    for (int y = 0; y < N; y++)
    {
        f << "<tr><th>" << y << "</th>";
        for (int x = 0; x < N; x++)
        {
            if (!m.hay[y][x])
            {
                f << "<td></td>";
                continue;
            }
            const Orden& o = m.orden[y][x];
            string clase = "m";
            if (o.despegue) clase = "d";
            else if (o.aterrizaje) clase = "a";
            else if (o.soltarGranada1 || o.soltarGranada2) clase = "g";
            else if (o.ataqueKamikaze) clase = "k";
            else if (o.espera > 0) clase = "e";

            string texto;
            if (o.despegue) texto += "D ";
            if (o.aterrizaje) texto += "A ";
            if (o.soltarGranada1) texto += "G1 ";
            if (o.soltarGranada2) texto += "G2 ";
            if (o.ataqueKamikaze) texto += "K ";
            if (o.espera > 0) texto += "E" + to_string(o.espera) + " ";
            if (!esFinal(o)) texto += flecha(x, y, o);

            f << "<td class=\"" << clase << "\" title=\"" << coord(x, y) << ": "
              << describir(o) << "\">" << texto << "</td>";
        }
        f << "</tr>\n";
    }
    f << "</table>\n</body>\n</html>\n";
    f.close();
    return true;
}

void opHTML()
{
    cout << "\n--- Visualizar ataque en HTML ---\n";
    cout << "  1. Visualizar el ataque cargado en memoria\n"
         << "  2. Visualizar un archivo de ataque\n";
    int fuente = (int)leerEntero("Opcion: ", 1, 2);

    unique_ptr<Mapa> temporal;
    const Mapa* origen = &memoria;
    string titulo = "memoria";

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
        string rutaIn = leerTexto("Ruta y nombre del archivo de ataque: ");
        vector<OrdenArchivo> regs;
        string error;
        if (!leerArchivo(rutaIn, regs, error))
        {
            cout << error << "\n";
            return;
        }
        temporal.reset(new Mapa());
        cargarRegistros(*temporal, regs);
        origen = temporal.get();
        titulo = rutaIn;
    }

    string rutaOut = leerTexto("Ruta y nombre del HTML a generar (ej: ataque.html): ");
    if (generarHTML(*origen, rutaOut, titulo))
        cout << "HTML generado en '" << rutaOut << "'. Abralo con el navegador.\n";
}

// ----------------------------------------------------------------------------
// Menu principal
// ----------------------------------------------------------------------------
void mostrarMenu()
{
    cout << "\n=========== GUIA DE APOYO PARA DRONES ===========\n"
         << " 1. Cargar archivo de ataque en memoria\n"
         << " 2. Mostrar ataque cargado\n"
         << " 3. Crear un archivo de ataque nuevo\n"
         << " 4. Corregir un registro del archivo\n"
         << " 5. Corregir un registro en memoria\n"
         << " 6. Guardar memoria en un archivo nuevo\n"
         << " 7. Visualizar un archivo de ataque en html\n"
         << " 0. Salir\n"
         << "==================================================\n";
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
