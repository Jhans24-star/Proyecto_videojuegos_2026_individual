#include <GL/freeglut.h>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <ctime>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
const float SPAWN_INTERVAL = 0.6f;
const int MAX_ASTEROIDS = 50;

// Variables globales de la nave
float shipX = (WINDOW_WIDTH - SHIP_WIDTH) / 2.0f;
float shipY = WINDOW_HEIGHT - 80.0f;

bool moveLeft = false;
bool moveRight = false;

// Delta time
int previousTime = 0;
float deltaTime = 0.0f;
float spawnAccumulator = 0.0f;

bool jugando = true;

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
    if (asteroides.size() >= MAX_ASTEROIDS) return;
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
}

void reiniciarJuego() {
    jugando = true;
    shipX = (WINDOW_WIDTH - SHIP_WIDTH) / 2.0f;
    asteroides.clear();
    spawnAccumulator = 0.0f;
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    moveLeft = false;
    moveRight = false;
    fondoScrollY = 0.0f;
}

// Callbacks de OpenGL
void display() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    dibujarFondo(texFondoEstatico, 0.0f);
    dibujarFondo(texFondoMovil, fondoScrollY);
    dibujarNave(shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT);

    for (const auto& a : asteroides)
        dibujarAsteroide(a.x, a.y, ASTEROID_BASE_RADIUS,
                         ASTEROID_BASE_SPRITE_WIDTH, ASTEROID_BASE_SPRITE_HEIGHT,
                         a.escala, a.angulo);

    if (!jugando) {
        dibujarTexto(WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2, "GAME OVER");
        dibujarTexto(WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 30, "Presiona R para reiniciar");
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
    (void)value;
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = (currentTime - previousTime) / 1000.0f;
    previousTime = currentTime;

    if (jugando) {
        // Movimiento nave
        if (moveLeft) shipX -= SHIP_SPEED * deltaTime;
        if (moveRight) shipX += SHIP_SPEED * deltaTime;
        if (shipX < 0) shipX = 0;
        if (shipX > WINDOW_WIDTH - SHIP_WIDTH) shipX = WINDOW_WIDTH - SHIP_WIDTH;

        // Generación asteroides
        spawnAccumulator += deltaTime;
        while (spawnAccumulator >= SPAWN_INTERVAL) {
            generarAsteroide();
            spawnAccumulator -= SPAWN_INTERVAL;
        }

        // Mover asteroides y actualizar rotación
        for (auto& a : asteroides) {
            a.y += a.velocidad * deltaTime;
            a.angulo += ASTEROID_ANGULAR_SPEED * deltaTime;
            if (a.angulo >= 360.0f) a.angulo -= 360.0f;
        }

        // Eliminar fuera de pantalla
        asteroides.erase(std::remove_if(asteroides.begin(), asteroides.end(),
            [](const Asteroide& a) { return a.y > WINDOW_HEIGHT + a.radio; }),
            asteroides.end());

        // Colisiones
        for (const auto& a : asteroides) {
            if (colisionNaveAsteroide(shipX, shipY, SHIP_WIDTH, SHIP_HEIGHT,
                                      a.x, a.y, a.radio)) {
                jugando = false;
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
        case 'r': case 'R':
            if (!jugando) reiniciarJuego();
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

// Inicialización
void initTexturas() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    texFondoEstatico = cargarTextura("assets/espacio_lejano.png");
    texFondoMovil = cargarTextura("assets/estrellas_movil.png");
    texNave = cargarTextura("assets/nave.png");
    texAsteroide = cargarTextura("assets/asteroide_01.png");
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
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, update, 0);
    srand(static_cast<unsigned>(time(nullptr)));

    initTexturas();
}

int main(int argc, char** argv) {
    initGLUT(argc, argv);
    glutMainLoop();
    return 0;
}
