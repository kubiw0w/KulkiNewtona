/**
 * @file main.cpp
 * @brief Symulacja Wahadła Newtona w 3D z użyciem GLFW i OpenGL.
 *
 * Program realizuje wizualną i fizyczną symulację Wahadła Newtona.
 * Użytkownik może chwytać kulki myszką, przeciągać je i puszczać,
 * inicjując ruch zgodny z zasadami zachowania pędu.
 */

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

 /**
  * @def M_PI
  * @brief Stała matematyczna PI.
  */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

  /**
   * @brief Rysuje kulę metodą parametryczną.
   *
   * Funkcja generuje kulę wykorzystując
   * współrzędne sferyczne i prymitywy GL_QUAD_STRIP.
   *
   * @param radius Promień sfery
   * @param slices Liczba podziałów poziomych
   * @param stacks Liczba podziałów pionowych
   */
void drawSphere(float radius, int slices, int stacks) {
    for (int i = 0; i < stacks; ++i) {
        float lat0 = M_PI * (-0.5f + (float)i / stacks);
        float lat1 = M_PI * (-0.5f + (float)(i + 1) / stacks);
        float z0 = radius * sin(lat0);
        float zr0 = radius * cos(lat0);
        float z1 = radius * sin(lat1);
        float zr1 = radius * cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * M_PI * (float)j / slices;
            float x = cos(lng);
            float y = sin(lng);

            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(x * zr0, y * zr0, z0);

            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(x * zr1, y * zr1, z1);
        }
        glEnd();
    }
}

/**
 * @brief Rysuje podwójną ramę Wahadła Newtona.
 *
 * Rama składa się z dwóch pionowych nóg oraz
 * dwóch górnych poprzeczek przedniej i tylnej.
 *
 * @param startX Pozycja lewej nogi
 * @param endX Pozycja prawej nogi
 * @param topY Wysokość górnej belki
 * @param baseY Wysokość podstawy
 * @param zOffset Odsunięcie w osi Z
 */
void drawDoubleFrame(float startX, float endX, float topY, float baseY, float zOffset) {
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINES);

    float frontZ = zOffset;
    float backZ = -zOffset;

    glVertex3f(startX, baseY, frontZ); glVertex3f(startX, topY, frontZ);
    glVertex3f(startX, baseY, backZ);  glVertex3f(startX, topY, backZ);

    glVertex3f(endX, baseY, frontZ);   glVertex3f(endX, topY, frontZ);
    glVertex3f(endX, baseY, backZ);    glVertex3f(endX, topY, backZ);

    glVertex3f(startX, topY, frontZ);  glVertex3f(endX, topY, frontZ);
    glVertex3f(startX, topY, backZ);   glVertex3f(endX, topY, backZ);

    glEnd();
}

/**
 * @struct PendulumBall
 * @brief Reprezentuje pojedynczą kulkę Wahadła Newtona.
 *
 * Struktura zawiera wszystkie dane fizyczne i wizualne
 * potrzebne do symulacji ruchu wahadła.
 */
struct PendulumBall {

    /** Punkt zawieszenia kulki */
    glm::vec3 pivot;

    /** Długość nici */
    float length;

    /** Aktualny kąt wychylenia */
    float angle;

    /** Prędkość kątowa */
    float angularVel;

    /** Przyspieszenie kątowe */
    float angularAcc;

    /** Promień kulki */
    float radius;

    /** Masa kulki */
    float mass;

    /** Aktualna pozycja kulki w przestrzeni */
    glm::vec3 pos;

    /**
     * @brief Aktualizuje stan fizyczny kulki.
     *
     * Implementuje równanie ruchu wahadła matematycznego
     * z niewielkim tłumieniem.
     *
     * @param dt Krok czasowy symulacji
     */
    void update(float dt) {
        float g = 1.0f;
        angularAcc = (-g / length) * sin(angle);
        angularVel += angularAcc * dt;
        angle += angularVel * dt;
        angularVel *= 0.9999999f;

        pos = pivot + glm::vec3(sin(angle), -cos(angle), 0.0f) * length;
    }

    /**
     * @brief Rysuje kulkę oraz jej zawieszenie.
     *
     * Rysuje dwie nici oraz kulę 3D.
     */
    void draw() {
        float stringOffset = 10.0f;

        glm::vec3 leftAnchor = glm::vec3(pivot.x - stringOffset, pivot.y, pivot.z - 30.0f);
        glm::vec3 rightAnchor = glm::vec3(pivot.x + stringOffset, pivot.y, pivot.z + 30.0f);

        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_LINES);
        glVertex3f(leftAnchor.x, leftAnchor.y, leftAnchor.z);
        glVertex3f(pos.x, pos.y, pos.z);
        glVertex3f(rightAnchor.x, rightAnchor.y, rightAnchor.z);
        glVertex3f(pos.x, pos.y, pos.z);
        glEnd();

        glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        glColor3f(0.3f, 0.6f, 0.9f);
        drawSphere(radius, 20, 20);
        glPopMatrix();
    }
};

/** Lista wszystkich kulek wahadła */
std::vector<PendulumBall> balls;

/** Czy trwa przeciąganie kulki */
bool dragging = false;

/** Indeks aktualnie przeciąganej kulki */
int draggedIndex = -1;

/** Aktualna pozycja myszy w przestrzeni */
glm::vec3 mousePos;

/**
 * @brief Konwertuje pozycję kursora na współrzędne świata 3D.
 *
 * @param window Okno GLFW
 * @return Pozycja myszy w przestrzeni świata
 */
glm::vec3 getMousePos(GLFWwindow* window) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    y = height - y;

    float ndcX = (2.0f * x) / width - 1.0f;
    float ndcY = (2.0f * y) / height - 1.0f;

    float aspect = width / (float)height;
    float scale = tan(45.0f * 0.5 * M_PI / 180.0f);

    return glm::vec3(
        400.0f + ndcX * aspect * scale * 600.0f,
        300.0f + ndcY * scale * 600.0f,
        0.0f
    );
}

/**
 * @brief Callback obsługujący kliknięcia myszy.
 *
 * Umożliwia chwytanie i puszczanie kulek.
 *
 * @param window Okno GLFW
 * @param button Przycisk myszy
 * @param action Akcja myszki
 * @param mods Modyfikatory klawiatury
 */
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_PRESS) {
        mousePos = getMousePos(window);
        for (int i = 0; i < balls.size(); ++i) {
            if (glm::distance(mousePos, balls[i].pos) < balls[i].radius) {
                dragging = true;
                draggedIndex = i;
                balls[i].angularVel = 0.0f;
                break;
            }
        }
    }
    else if (action == GLFW_RELEASE) {
        dragging = false;
        draggedIndex = -1;
    }
}

/**
 * @brief Rozwiązuje kolizję pomiędzy dwiema kulkami.
 *
 * Implementuje uproszczone zachowanie sprężyste
 * na osi X.
 *
 * @param a Pierwsza kulka
 * @param b Druga kulka
 */
void resolvePendulumCollision(PendulumBall& a, PendulumBall& b) {
    float dist = glm::distance(a.pos, b.pos);
    float minDist = a.radius + b.radius;

    if (dist < minDist) {
        float dir = (b.pos.x - a.pos.x) > 0 ? 1.0f : -1.0f;

        float avgVel = (a.angularVel + b.angularVel) * 0.5f;
        float diffVel = (a.angularVel - b.angularVel) * 0.5f;

        a.angularVel = avgVel - diffVel;
        b.angularVel = avgVel + diffVel;

        float overlap = 0.5f * (minDist - dist);
        a.pos.x -= overlap * dir;
        b.pos.x += overlap * dir;

        a.angle = atan2(a.pos.x - a.pivot.x, a.pivot.y - a.pos.y);
        b.angle = atan2(b.pos.x - b.pivot.x, b.pivot.y - b.pos.y);
    }
}

/**
 * @brief Inicjalizuje oświetlenie i test głębokości.
 */
void setupScene3D() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 0.0f, 500.0f, 300.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat lightDiffuse[] = { 0.9f, 0.9f, 0.9f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
}

/**
 * @brief Punkt wejścia programu.
 *
 * Inicjalizuje GLFW, tworzy scenę,
 * uruchamia pętlę renderującą
 * oraz obsługuje interakcję użytkownika.
 *
 * @return Kod zakończenia programu
 */
int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "Wahadlo Newtona", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 2000.0f);
    glLoadMatrixf(glm::value_ptr(projection));

    setupScene3D();

    int count = 5;
    float spacing = 40.0f;
    float startX = 400.0f - (count - 1) * 0.5f * spacing;

    for (int i = 0; i < count; ++i) {
        PendulumBall b;
        b.pivot = glm::vec3(startX + i * spacing, 500, 0);
        b.length = 150.0f;
        b.radius = 20.0f;
        b.mass = 1.0f;
        b.angle = 0.0f;
        b.angularVel = 0.0f;
        b.pos = b.pivot + glm::vec3(0, -b.length, 0);
        balls.push_back(b);
    }

    float dt = 0.016f;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glm::mat4 view = glm::lookAt(
            glm::vec3(600, 300, 600),
            glm::vec3(400, 300, 0),
            glm::vec3(0, 1, 0)
        );
        glLoadMatrixf(glm::value_ptr(view));

        drawDoubleFrame(startX - 30, startX + (count - 1) * spacing + 30, 500, 200, 30);

        mousePos = getMousePos(window);
        if (dragging && draggedIndex >= 0) {
            PendulumBall& b = balls[draggedIndex];
            glm::vec3 delta = mousePos - b.pivot;
            b.angle = atan2(delta.x, -delta.y);
            b.angularVel = 0.0f;
            b.pos = b.pivot + glm::vec3(sin(b.angle), -cos(b.angle), 0) * b.length;
        }

        for (int i = 0; i < balls.size(); ++i)
            if (!dragging || i != draggedIndex)
                balls[i].update(dt);

        for (size_t i = 0; i < balls.size(); ++i)
            for (size_t j = i + 1; j < balls.size(); ++j)
                resolvePendulumCollision(balls[i], balls[j]);

        for (auto& b : balls)
            b.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
