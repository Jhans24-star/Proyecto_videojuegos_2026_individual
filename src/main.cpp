#include <GL/freeglut.h>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb/stb_image.h"

// Constantes de la ventana
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "Escape del Interestelar - Prototipo";

// Parámetros de la nave
const float SHIP_WIDTH = 64.0f;
const float SHIP_HEIGHT = 64.0f;
const float SHIP_SPEED = 300.0f;

// Parámetros base de los asteroides
const float ASTEROID_BASE_SPEED = 150.0f;
const float ASTEROID_BASE_RADIUS = 15.0f;          // radio base para colisión
const float ASTEROID_BASE_SPRITE_WIDTH = 32.0f;    // ancho base del sprite
const float ASTEROID_BASE_SPRITE_HEIGHT = 32.0f;   // alto base del sprite
const float ASTEROID_ANGULAR_SPEED = 90.0f;        // grados por segundo
const int MAX_ASTEROIDS = 50;

// Variables globales de la nave
float shipX = (WINDOW_WIDTH - SHIP_WIDTH) / 2.0f;
float shipY = WINDOW_HEIGHT - 80.0f;

bool moveLeft = false;
bool moveRight = false;

// Delta time
int previousTime = 0;
float deltaTime = 0.0f;
int tiempoInicioNivel = 0;

bool jugando = true;

// =====================================
// VARIABLES DE RENDIMIENTO
// =====================================

int frames = 0;
float fps = 0.0f;

int tiempoFPS = 0;

float msPorFrame = 0.0f;

float tiempoRender = 0.0f;

enum EstadoJuego
{
    MENU,
    INSTRUCCIONES,
    TRANSICION_NIVEL,
    JUGANDO,
    GAME_OVER,
    VICTORIA
};

EstadoJuego estadoActual = MENU;

int asteroidesEsquivados = 0;
int asteroidesGenerados = 0;
int nivelActual = 1;

bool nivelCompletado = false;

const int ASTEROIDES_PARA_GANAR = 10;

// Estructura de asteroide con escala individual
struct Asteroide {
    float x, y;
    float velocidad;
    float radio; // radio actual (base * escala)
    float escala; // factor de tamaño (1.0 a 2.0)
    float angulo;
};
std::vector<Asteroide> asteroides;

// Variables para texturas
GLuint texFondoEstatico = 0;
GLuint texFondoMovil = 0;
GLuint texNave = 0;
GLuint texAsteroide = 0;
GLuint texMenu = 0;
GLuint texInstrucciones = 0;

// Desplazamiento del fondo móvil
float fondoScrollY = 0.0f;
const float FONDOVEL = 40.0f;

// Funciones de carga de texturas
GLuint cargarTextura(const char* rutaArchivo) {
    int ancho, alto, canales;
    unsigned char* datos = stbi_load(rutaArchivo, &ancho, &alto, &canales, 4);
    if (!datos) {
        printf("Error cargando textura: %s\n", rutaArchivo);
        return 0;
    }

    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ancho, alto, 0, GL_RGBA, GL_UNSIGNED_BYTE, datos);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(datos);
    return texId;
}

// Funciones de dibujo
void dibujarFondo(GLuint textura, float desplazamientoY = 0.0f) {
    if (textura == 0) return;
    glBindTexture(GL_TEXTURE_2D, textura);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, desplazamientoY / 600.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, desplazamientoY / 600.0f);
        glVertex2f(WINDOW_WIDTH, 0.0f);
        glTexCoord2f(1.0f, 1.0f + desplazamientoY / 600.0f);
        glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
        glTexCoord2f(0.0f, 1.0f + desplazamientoY / 600.0f);
        glVertex2f(0.0f, WINDOW_HEIGHT);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void dibujarPantallaCompleta(GLuint textura)
{
    if (textura == 0) return;

    glBindTexture(GL_TEXTURE_2D, textura);
    glEnable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);

        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0, 0);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(WINDOW_WIDTH, 0);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0, WINDOW_HEIGHT);

    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void dibujarNave(float x, float y, float ancho, float alto) {
    if (texNave == 0) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + ancho, y);
            glVertex2f(x + ancho, y + alto);
            glVertex2f(x, y + alto);
        glEnd();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, texNave);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(x + ancho, y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(x + ancho, y + alto);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + alto);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void dibujarAsteroide(float x, float y, float radioBase, float anchoBase, float altoBase, float escala, float angulo) {
    if (texAsteroide == 0) {
        // Fallback círculo gris (tamaño escalado)
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(x, y);
            for (int i = 0; i <= 360; i += 15) {
                float rad = i * 3.14159f / 180.0f;
                float dx = radioBase * escala * cos(rad);
                float dy = radioBase * escala * sin(rad);
                glVertex2f(x + dx, y + dy);
            }
        glEnd();
        return;
    }

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(angulo, 0.0f, 0.0f, 1.0f);
    glScalef(escala, escala, 1.0f);   // Escala del sprite
    glBindTexture(GL_TEXTURE_2D, texAsteroide);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    float halfW = anchoBase / 2.0f;
    float halfH = altoBase / 2.0f;
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-halfW, -halfH);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( halfW, -halfH);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( halfW,  halfH);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-halfW,  halfH);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void dibujarTexto(float x, float y, const char* texto) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    for (const char* c = texto; *c != '\0'; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}


// Lógica del juego
bool colisionNaveAsteroide(float naveX, float naveY, float naveAncho, float naveAlto,
                           float astX, float astY, float astRadio) {
    float closestX = std::max(naveX, std::min(astX, naveX + naveAncho));
    float closestY = std::max(naveY, std::min(astY, naveY + naveAlto));
    float dx = astX - closestX;
    float dy = astY - closestY;
    return (dx*dx + dy*dy) < (astRadio*astRadio);
}

void generarAsteroide() {
    if (asteroidesGenerados >= ASTEROIDES_PARA_GANAR)
    return;
    Asteroide a;
    // Tamaño variable entre 1.0 y 2.0
    a.escala = 1.0f + (rand() % 100) / 100.0f;
    // Alternativa para hasta 2.0
    a.radio = ASTEROID_BASE_RADIUS * a.escala;
    // a.velocidad = ASTEROID_BASE_SPEED / a.escala; // Velocidad proporcional al tamaño
    a.velocidad = ASTEROID_BASE_SPEED; // velocidad constante
    a.x = a.radio + (rand() % (WINDOW_WIDTH - 2 * (int)a.radio));
    a.y = -a.radio;
    a.angulo = rand() % 360;
    asteroides.push_back(a);
    asteroidesGenerados++;
}

void reiniciarJuego()
{
    jugando = true;

    estadoActual = JUGANDO;

    shipX = (WINDOW_WIDTH - SHIP_WIDTH) / 2.0f;

    asteroides.clear();

    previousTime = glutGet(GLUT_ELAPSED_TIME);

    moveLeft = false;
    moveRight = false;

    fondoScrollY = 0.0f;

    asteroidesEsquivados = 0;

    nivelCompletado = false;

    asteroidesGenerados = 0;
}

// Callbacks de OpenGL
void display() {
    int inicioRender = glutGet(GLUT_ELAPSED_TIME);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (estadoActual == MENU)
    {
        dibujarPantallaCompleta(texMenu);

        glColor3f(0.2f, 0.2f, 0.8f);

        glBegin(GL_QUADS);

            glVertex2f(300, 450);
            glVertex2f(500, 450);
            glVertex2f(500, 520);
            glVertex2f(300, 520);

        glEnd();

        dibujarTexto(365, 492, "JUGAR");

        frames++;

    int tiempoActual = glutGet(GLUT_ELAPSED_TIME);

    if (tiempoActual - tiempoFPS > 1000)
    {
        fps = frames * 1000.0f /
              (tiempoActual - tiempoFPS);

        msPorFrame = 1000.0f / fps;

        frames = 0;
        tiempoFPS = tiempoActual;
    }

        glutSwapBuffers();
        return;
    }

    if (estadoActual == INSTRUCCIONES)
    {
        dibujarPantallaCompleta(texInstrucciones);

        glColor3f(0.2f, 0.2f, 0.8f);

        glBegin(GL_QUADS);

            glVertex2f(300, 530);
            glVertex2f(500, 530);
            glVertex2f(500, 590);
            glVertex2f(300, 590);

        glEnd();

        dibujarTexto(370, 568, "INICIO");

        glutSwapBuffers();
        return;
    }

    if (estadoActual == TRANSICION_NIVEL)
    {
        dibujarFondo(texFondoEstatico);

        dibujarTexto(
            WINDOW_WIDTH / 2 - 50,
            WINDOW_HEIGHT / 2,
            "NIVEL 1"
        );

        glutSwapBuffers();
        return;
    }

    dibujarFondo(texFondoEstatico, 0.0f);
    dibujarFondo(texFondoMovil, fondoScrollY);
    dibujarNave(shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT);

    for (const auto& a : asteroides)
        dibujarAsteroide(a.x, a.y, ASTEROID_BASE_RADIUS,
                         ASTEROID_BASE_SPRITE_WIDTH, ASTEROID_BASE_SPRITE_HEIGHT,
                         a.escala, a.angulo);

    // =========================================
    // PANTALLA GAME OVER
    // =========================================
    if (estadoActual == GAME_OVER)
    {
        dibujarTexto(
            WINDOW_WIDTH / 2 - 60,
            WINDOW_HEIGHT / 2,
            "GAME OVER"
        );

        dibujarTexto(
            WINDOW_WIDTH / 2 - 100,
            WINDOW_HEIGHT / 2 + 40,
            "Presiona R para reiniciar"
        );
    }

    // =========================================
    // PANTALLA VICTORIA
    // =========================================
    if (estadoActual == VICTORIA)
    {
        dibujarTexto(
            WINDOW_WIDTH / 2 - 110,
            WINDOW_HEIGHT / 2,
            "Has completado el nivel 1"
        );

        dibujarTexto(
            WINDOW_WIDTH / 2 - 110,
            WINDOW_HEIGHT / 2 + 40,
            "Presiona R para continuar"
        );
    }

    std::string textoNivel =
        "Nivel: " + std::to_string(nivelActual);

    dibujarTexto(20, 30, textoNivel.c_str());

    std::string textoContador =
        "Esquivados: " +
        std::to_string(asteroidesEsquivados) +
        "/10";

    dibujarTexto(20, 60, textoContador.c_str());

    // =====================================
    // INFORMACION DE RENDIMIENTO
    // =====================================

    char buffer[100];

    sprintf(buffer,
            "FPS: %.0f",
            fps);

    dibujarTexto(20, 90, buffer);

    sprintf(buffer,
            "Frame: %.2f ms",
            msPorFrame);

    dibujarTexto(20, 120, buffer);

    sprintf(buffer,
            "Asteroides: %d",
            (int)asteroides.size());

    dibujarTexto(20, 150, buffer);

    int finRender = glutGet(GLUT_ELAPSED_TIME);

    tiempoRender =
        finRender - inicioRender;

    sprintf(buffer,
            "Render: %.2f ms",
            tiempoRender);

    dibujarTexto(20, 180, buffer);

    frames++;

    int tiempoActualFPS =
        glutGet(GLUT_ELAPSED_TIME);

    if (tiempoActualFPS - tiempoFPS >= 1000)
    {
        fps =
            frames * 1000.0f /
            (tiempoActualFPS - tiempoFPS);

        msPorFrame =
            1000.0f / fps;

        frames = 0;

        tiempoFPS =
            tiempoActualFPS;
    }

    glutSwapBuffers();
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0);
    glMatrixMode(GL_MODELVIEW);
}

void update(int value) {

    if (estadoActual == TRANSICION_NIVEL)
    {
        int tiempoActual = glutGet(GLUT_ELAPSED_TIME);

        if (tiempoActual - tiempoInicioNivel >= 3000)
        {
            estadoActual = JUGANDO;

            reiniciarJuego();
        }

        glutPostRedisplay();
        glutTimerFunc(16, update, 0);

        return;
    }

    (void)value;
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;

    if (asteroidesEsquivados >= ASTEROIDES_PARA_GANAR)
    {
        nivelCompletado = true;

        jugando = false;

        estadoActual = VICTORIA;

        moveLeft = false;
        moveRight = false;
    }

    if (estadoActual == JUGANDO && jugando) {
        // Movimiento nave
        if (moveLeft) shipX -= SHIP_SPEED * deltaTime;
        if (moveRight) shipX += SHIP_SPEED * deltaTime;
        if (shipX < 0) shipX = 0;
        if (shipX > WINDOW_WIDTH - SHIP_WIDTH) shipX = WINDOW_WIDTH - SHIP_WIDTH;

        // Generar asteroides
        if (asteroides.empty())
        {
            generarAsteroide();
        }
        else
        {
            Asteroide& ultimo = asteroides.back();

            if (ultimo.y >= WINDOW_HEIGHT * 0.75f)
            {
                generarAsteroide();
            }
        }


        // Mover asteroides y actualizar rotación
        for (auto& a : asteroides) {
            a.y += a.velocidad * deltaTime;
            a.angulo += ASTEROID_ANGULAR_SPEED * deltaTime;
            if (a.angulo >= 360.0f) a.angulo -= 360.0f;
        }

        for (auto& a : asteroides)
        {
            if (a.y > WINDOW_HEIGHT + a.radio)
            {
                asteroidesEsquivados++;
            }
        }

        asteroides.erase(
            remove_if(asteroides.begin(), asteroides.end(),
                [](const Asteroide& a)
                {
                    return a.y > WINDOW_HEIGHT + a.radio;
                }),
            asteroides.end()
        );

        // Colisiones
        for (const auto& a : asteroides) {
            if (colisionNaveAsteroide(shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT,
                                      a.x, a.y, a.radio)) {
                jugando = false;

                estadoActual = GAME_OVER;

                moveLeft = false;
                moveRight = false;

                break;
            }
        }

        // Desplazamiento fondo móvil
        fondoScrollY -= FONDOVEL * deltaTime;
        if (fondoScrollY < 0) fondoScrollY += WINDOW_HEIGHT;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboardDown(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case 'a': case 'A': moveLeft = true; break;
        case 'd': case 'D': moveRight = true; break;
        case 'r':
        case 'R':

            if (estadoActual == GAME_OVER ||
                estadoActual == VICTORIA)
            {
                reiniciarJuego();
            }

            break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case 'a': case 'A': moveLeft = false; break;
        case 'd': case 'D': moveRight = false; break;
    }
}

void mouseClick(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        // BOTON JUGAR
        if (estadoActual == MENU)
        {
            if (x >= 300 && x <= 500 &&
                y >= 450 && y <= 520)
            {
                estadoActual = INSTRUCCIONES;
            }
        }

        // BOTON INICIO
        else if (estadoActual == INSTRUCCIONES)
        {
            if (x >= 300 && x <= 500 &&
                y >= 530 && y <= 590)
            {
                estadoActual = TRANSICION_NIVEL;

                tiempoInicioNivel =
                    glutGet(GLUT_ELAPSED_TIME);
            }
        }
    }
}

// Inicialización
void initTexturas() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    texFondoEstatico = cargarTextura("assets/espacio_lejano.png");
    texFondoMovil = cargarTextura("assets/estrellas_movil.png");
    texNave = cargarTextura("assets/nave.png");
    texAsteroide = cargarTextura("assets/asteroide_01.png");
    texMenu = cargarTextura("assets/menu.png");
    texInstrucciones = cargarTextura("assets/instrucciones.png");
}

void initGLUT(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow(WINDOW_TITLE);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseClick);
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    tiempoFPS =
        glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, update, 0);
    srand(static_cast<unsigned>(time(nullptr)));

    initTexturas();
}

int main(int argc, char** argv) {
    initGLUT(argc, argv);
    glutMainLoop();
    return 0;
}
