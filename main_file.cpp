// Grafika Komputerowa i Wizualizacja 2023
// Akwarium


#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "./include/myCube.h"
#include "./include/constants.h"
#include "./include/allmodels.h"
#include "./include/lodepng.h"
#include "./include/shaderprogram.h"
#include "./include/ObjLoader.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "./include/myTeapot.h"

#define FISH 0  //przypisanie poszczegolnym modelom i teksturom 'pseudonimow' uzytych pozniej w kodzie
#define TANK 1
#define ROCK1 2
#define ROCK2 3
#define AKWAR 4
#define FISH1 5
#define FISH2 6
#define PLANTS 7

#define TEX_FISH 0
#define TEX_TANK 1
#define TEX_ROCK1 2 
#define TEX_ROCK2 3
#define TEX_BOTTOM 4
#define TEX_FISH1 5
#define TEX_PLANT 6
#define TEX_FISH2 7

glm::vec4 light1 = glm::vec4(-2.5, 6, 0, 1);
glm::vec4 light2 = glm::vec4(2.5, 6, 0, 1);
float speed = 1.5;

struct MyVertex { //do wczytywania modeli z assimp
	std::vector<glm::vec4> Vertices; //wierzcholki
	std::vector<glm::vec4> Normals; //normalne
	std::vector<glm::vec2> TexCoords; //wspolrzedne tekstur
	std::vector<unsigned int> Indices; //indeksy wierzcholkow
};

std::vector<MyVertex> models;//wektor do modeli
std::vector<GLuint> texs;//wektor do tekstur

ShaderProgram* waterShader;
ShaderProgram* phongShader;
ShaderProgram* glassShader;


void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

glm::vec3 cameraPos = glm::vec3(0.0f,2.0f, 3.0f);//poczatkowe ustawienia kamery
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mod) {//feedback z klawiszy
	const float cameraSpeed = 0.13f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}
//poczatkowe ustawienia myszki
float lastX = 400, lastY = 300;
float yaw = -90.0f;
float pitch;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)//feedback z myszki
{
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

void loadModel(std::string filename, int model_i) { //biblioteka assimp
	models.push_back(MyVertex()); //nowe miejsce w wektorze do przechowywania modeli
	Assimp::Importer importer; //strukt klasy importer uzyty do wczytywania pliku

	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);
	//wczytuje model z pliku o podanej sciezce , konwertowanie trojkatne, odwrocenie wspolrzednych uv generowanie gladnkich normalnych

	aiMesh* mesh = scene->mMeshes[0]; //przypisanie wskaznika do siatki w mMeshes
	for (int i = 0; i < mesh->mNumVertices; i++) {//przejscie przez kazdy wierzcholek

		aiVector3D vertex = mesh->mVertices[i]; //pobierz pozycje wierzcholka (struktura aiVector3D sluzy do przechowywania trojwymiarowych obiektow)
		models[model_i].Vertices.push_back(glm::vec4(vertex.x, vertex.y, vertex.z, 1)); //dodaje do wektora wierzcholek jako vec4; czwarta - 1 to punkt w przestrzeni 3d
		aiVector3D normal = mesh->mNormals[i]; //pobierz normalna wierzcholka
		models[model_i].Normals.push_back(glm::vec4(normal.x, normal.y, normal.z, 0)); //dodaje do wektora normalne jako vec4; czwarta - 0 to wektor w przestrzeni 3d


		aiVector3D texCoord = mesh->mTextureCoords[0][i]; //wspolrzedne tesktury wierzcholka (dwuwymiarowa tablica bo tekstury w modelu chyba moga sie na siebie nakladac)
		models[model_i].TexCoords.push_back(glm::vec2(texCoord.x, texCoord.y));// dodaje je do wektora wspolrzedne tekstury jako vec2
	}
	//Struktura aiFace jest częścią biblioteki Assimp i służy do przechowywania informacji o ścianach w siatce trójwymiarowej
	for (int i = 0; i < mesh->mNumFaces; i++) { //przechodzi przez kazda sciane
		aiFace& face = mesh->mFaces[i]; //referencja do obiektu aiFace o nazwie face  //face reprezentuje jedna sciane w siatce
		for (int j = 0; j < face.mNumIndices; j++) { //przechodzi przez indeksy
			models[model_i].Indices.push_back(face.mIndices[j]);//dodawanie do wektora indices
		}
	}
}

void readTexture(const char* filename, int tex_i) {
	texs.push_back(0); //nowe miejsce w wektorze do przechowywania tekstur
	GLuint tex; //zmienna gluint do trzymania jednej tekstury
	glActiveTexture(GL_TEXTURE0); //ustawia aktywna jednostke teksturujaca na jednostke o indeksie 0

	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image;   //alokuj wektor do wczytania obrazka
	unsigned width, height;   //zmienne do których wczytamy wymiary 
	unsigned error = lodepng::decode(image, width, height, filename);
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci GPU skojarzonej z uchwytem
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());//wczytanie tekstury z pamieci komputera do pamieci GPU

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//parametry filtrowania tekstur zapewnia plynne przejscie miedzy pikselami tekstury przy skalowaniu
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	texs[tex_i] = tex;//tex zapisany do wektora tekstur
}

void initOpenGLProgram(GLFWwindow* window) {
    initShaders();
	glClearColor(0.9, 0.9, 0.9, 0.5);//kolor tla 
	glEnable(GL_DEPTH_TEST);//ktory piksel jest blizej do kamery
	glEnable(GL_BLEND);//dobrze dzialaja elementy przezroczyste
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//wspiera dzialanie przezroczystosci

	glfwSetKeyCallback(window, key_callback);//wlaczenie dzialania klawiatury
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//kursor myszki zostaje ukryty i nie opuszcza okna, trzeba alttaabowac
	glfwSetCursorPosCallback(window, mouse_callback);//ustawia funkcje zwrotna mouse_callback, wywolana przy zmianie pozycji kursora


	readTexture("./img/fish.png", TEX_FISH);
	readTexture("./img/tank2.png", TEX_TANK);
	readTexture("./img/rock0.png", TEX_ROCK1);
	readTexture("./img/sand.png", TEX_ROCK2);
	readTexture("./img/sand.png", TEX_BOTTOM);
	readTexture("./img/fish2.png", TEX_FISH1);
	readTexture("./img/lisc.png", TEX_PLANT);
	readTexture("./img/Helena.png", TEX_FISH2);

	loadModel(std::string("models/fish.obj"), FISH);
	loadModel(std::string("models/tank.fbx"), TANK);
	loadModel(std::string("models/Rock0.obj"), ROCK1);
	loadModel(std::string("models/podloze.obj"), ROCK2);
	loadModel(std::string("models/akwarium.obj"), AKWAR);
	loadModel(std::string("models/fish_h.obj"), FISH1);
	loadModel(std::string("models/nemo.obj"), FISH2);
	loadModel(std::string("models/plants.obj"), PLANTS);

	waterShader = new ShaderProgram("v_water.glsl", NULL, "f_water.glsl");
	phongShader = new ShaderProgram("v_phong.glsl", NULL, "f_phong.glsl");
	glassShader = new ShaderProgram("v_glass.glsl", NULL, "f_glass.glsl");
}

void freeOpenGLProgram(GLFWwindow* window) {
    freeShaders();

	glDeleteTextures(1, &texs[TEX_TANK]);
	glDeleteTextures(1, &texs[TEX_ROCK1]);
	glDeleteTextures(1, &texs[TEX_ROCK2]);
	glDeleteTextures(1, &texs[TEX_BOTTOM]); //modeli nie trzeba usuwac bo nie sa zarzadzane przez OpenGL tylko przez assimp
}

void drawGlass(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	glassShader->use();

	glUniformMatrix4fv(glassShader->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(glassShader->u("V"), 1, false, glm::value_ptr(V)); 
	glUniformMatrix4fv(glassShader->u("M"), 1, false, glm::value_ptr(M)); 

	glEnableVertexAttribArray(glassShader->a("vertex"));
	glVertexAttribPointer(glassShader->a("vertex"), 4, GL_FLOAT, false, 0, myCubeVertices);

	glEnableVertexAttribArray(glassShader->a("normal"));
	glVertexAttribPointer(glassShader->a("normal"), 4, GL_FLOAT, false, 0, myCubeVertexNormals);

	glDrawArrays(GL_TRIANGLES, 0, myCubeVertexCount);

	glDisableVertexAttribArray(glassShader->a("vertex"));
	glDisableVertexAttribArray(glassShader->a("normal"));
}

void drawModel(glm::mat4 P, glm::mat4 V, glm::mat4 M, int model_i, int texture) {
	waterShader->use();

	glUniformMatrix4fv(waterShader->u("P"), 1, false, glm::value_ptr(P)); //macierz projekci, widoku, i modelu do shadera wodnego
	glUniformMatrix4fv(waterShader->u("V"), 1, false, glm::value_ptr(V)); //->u - uniform variable
	glUniformMatrix4fv(waterShader->u("M"), 1, false, glm::value_ptr(M));
	glUniform4fv(waterShader->u("light1"), 1, glm::value_ptr(light1));	//przeslanie wartosci swiatel do shadera wodnego
	glUniform4fv(waterShader->u("light2"), 1, glm::value_ptr(light2));

	glEnableVertexAttribArray(waterShader->a("vertex"));	//wlaczenie atrybutow wierzcholkow  skojarzenie danych wierzcholkow z odpowiednimi atrybutami shadera
	glVertexAttribPointer(waterShader->a("vertex"), 4, GL_FLOAT, false, 0, models[model_i].Vertices.data()); 

	glEnableVertexAttribArray(waterShader->a("texCoord"));  //to samo
	glVertexAttribPointer(waterShader->a("texCoord"), 2, GL_FLOAT, false, 0, models[model_i].TexCoords.data()); 
	 
	glActiveTexture(GL_TEXTURE0); //aktywuje teksture //gl_texture0	jest domyslna jednostka teksturujaca 
	glBindTexture(GL_TEXTURE_2D, texs[texture]); //powiazuje teksture z jednostka teskturujaca
	glUniform1i(waterShader->u("tex"), 0); //przeslanie wartosci uniform dla tekstury

	glDrawElements(GL_TRIANGLES, models[model_i].Indices.size(), GL_UNSIGNED_INT, models[model_i].Indices.data());//funkcja rysujaca model za pomoca indeksow
	//indeksy lacza ze soba wierzcholki prymitywnych ksztaltow, tutaj trojkatow

	glDisableVertexAttribArray(waterShader->a("vertex"));  //->a - attribute variable
	glDisableVertexAttribArray(waterShader->a("normal"));
	glDisableVertexAttribArray(waterShader->a("color"));
}

void drawRocks(glm::mat4 P, glm::mat4 V) {
	glm::mat4 rocks = glm::mat4(1.0f); //macierz jednostkowa 4x4 najlatwiejsza do transformacji
	rocks = glm::translate(rocks, glm::vec3(0.0f, -7.85f, 0.0f));
	rocks = glm::scale(rocks, glm::vec3(0.25f, 0.25f, 0.25f));
	glm::mat4 rocks1 = glm::mat4(1.0f);
	rocks1 = glm::scale(rocks1, glm::vec3(6.0f, 0.1f, 6.0f));

	glm::mat4 rock1 = glm::translate(rocks, glm::vec3(0.0f, -2.0f, 0.0f));
	drawModel(P, V, rock1, ROCK1, TEX_ROCK1);

	glm::mat4 rock2 = glm::translate(rocks1, glm::vec3(0.0f, -80.0f, 0.0f));
	drawModel(P, V, rock2, ROCK2, TEX_ROCK2);
}

void drawLight(glm::mat4 P, glm::mat4 V, glm::mat4 M) {
	Models::Sphere light(0.25, 36, 36);

	spConstant->use();

	glUniformMatrix4fv(spConstant->u("P"), 1, false, glm::value_ptr(P)); 
	glUniformMatrix4fv(spConstant->u("V"), 1, false, glm::value_ptr(V)); 
	glUniformMatrix4fv(spConstant->u("M"), 1, false, glm::value_ptr(M)); 

	glUniform4f(spConstant->u("color"), 1, 1, 0.8, 1);

	light.drawSolid();
}

void drawPlants(glm::mat4 P, glm::mat4 V) {
	glm::mat4 M = glm::mat4(1.0f);
	M = glm::scale(M, glm::vec3(0.25f, 0.25f, 0.25f));


	glm::mat4 plant1 = glm::scale(M, glm::vec3(1.5f ,1.5f, 1.5f));
	plant1 = glm::translate(plant1, glm::vec3(10.0f, -20.2f, 3.0f));
	drawModel(P, V, plant1, PLANTS, TEX_PLANT);
}

void drawScene(GLFWwindow* window,float angle) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	glm::mat4 M = glm::mat4(1.0f);//macierz modelu
	glm::mat4 V = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);//macierz widoku 
	glm::mat4 P = glm::perspective(glm::radians(50.0f), 1.0f, 1.0f, 50.0f);  //macierz perspektywy
	
	glm::mat4 Ml1 = glm::translate(M, glm::vec3(light1[0], light1[1], light1[2]));
	drawLight(P, V, Ml1);//swiatlo1
	glm::mat4 Ml2 = glm::translate(M, glm::vec3(light2[0], light2[1], light2[2]));
	drawLight(P, V, Ml2);//swiatlo2

	M = glm::rotate(M, -PI/2, glm::vec3(0.0f, 0.0f, 1.0f)); //macierz obrotu wokol osi Z

	float wsp_fish = 0.1f;

	glm::mat4 FishMatrix = glm::mat4(1.0f);
	glm::mat4 NemoMatrix = glm::scale(FishMatrix, glm::vec3(wsp_fish, wsp_fish, wsp_fish));
	glm::mat4 FishMatrix_h = glm::scale(FishMatrix, glm::vec3(wsp_fish, wsp_fish, wsp_fish));


	glm::mat4 Mf = glm::rotate(NemoMatrix, angle * 0.15f, glm::vec3(0.0f, -1.0f, 0.0f));
	Mf = glm::translate(Mf, glm::vec3(4.0f / wsp_fish, 0.0f, 0.0f));
	drawModel(P, V, Mf, FISH2, TEX_FISH1);
	
	glm::mat4 Mf1 = glm::rotate(FishMatrix_h, -angle * 0.5f, glm::vec3(0.0f, -1.0f, 0.0f));
	Mf1 = glm::translate(Mf1, glm::vec3(2.0f / wsp_fish, 2.0f / wsp_fish, 0.0f));
	drawModel(P, V, Mf1, FISH1, TEX_FISH2);

	glm::mat4 Mf2 = glm::rotate(NemoMatrix, angle * 0.19f, glm::vec3(0.0f, -1.0f, 0.0f));
	Mf2 = glm::translate(Mf2, glm::vec3(1.0f / wsp_fish, 1.0f / wsp_fish, 0.0f));
	drawModel(P, V, Mf2, FISH2, TEX_FISH1);

	glm::mat4 Mf3 = glm::rotate(FishMatrix_h, -angle * 0.01f, glm::vec3(0.0f, -1.0f, 0.0f));
	Mf3 = glm::translate(Mf3, glm::vec3(4.0f / wsp_fish, 3.0f / wsp_fish, 0.0f));
	drawModel(P, V, Mf3, FISH1, TEX_FISH2);

	glm::mat4 Mf4 = glm::rotate(NemoMatrix, angle * 0.16f, glm::vec3(0.0f, -1.0f, 0.0f));
	Mf4 = glm::translate(Mf4, glm::vec3(6.0f / wsp_fish, 2.0f / wsp_fish, 0.0f));
	drawModel(P, V, Mf4, FISH2, TEX_FISH1);

	glm::mat4 Mf5 = glm::rotate(NemoMatrix, angle * 0.01f, glm::vec3(0.0f, -1.0f, 0.0f)); 
	Mf5 = glm::translate(Mf5, glm::vec3(3.0f / wsp_fish, 1.0f / wsp_fish, 0.0f));
	drawModel(P, V, Mf5, FISH2, TEX_FISH1);

	glm::mat4 Mf7 = glm::rotate(M, angle * 0.01f, glm::vec3(-1.0f, 0.0f, 0.0f));  
	Mf7 = glm::translate(Mf7, glm::vec3(3.0f, 0.0f, 2.7f));
	drawModel(P, V, Mf7, FISH, TEX_FISH);

	glm::mat4 Mf8 = glm::rotate(M, -0.01f * angle, glm::vec3(-1.0f, 0.0f, 0.0f)); 
	Mf8 = glm::translate(Mf8, glm::vec3(1.0f, 0.0f, 2.5f));
	Mf8 = glm::rotate(Mf8, PI, glm::vec3(1.0f, 0.0f, 0.0f));
	drawModel(P, V, Mf8, FISH, TEX_FISH);

	glm::mat4 Mf9 = glm::rotate(M, -angle * 0.01f, glm::vec3(-1.0f, 0.0f, 0.0f)); 
	Mf9 = glm::translate(Mf9, glm::vec3(3.0f, 0.0f, -0.7f));
	drawModel(P, V, Mf9, FISH, TEX_FISH);

	glm::mat4 Mf10 = glm::rotate(M, -angle * 0.8f, glm::vec3(-0.1f, 0.0f, 0.0f));  
	Mf10 = glm::translate(Mf10, glm::vec3(-2.0f, 0.0f, -2.7f));
	drawModel(P, V, Mf10, FISH, TEX_FISH);

	drawRocks(P, V);
	drawPlants(P, V);


	glm::mat4 Mt = glm::rotate(M, PI / 2, glm::vec3(0.0f, 1.0f, 0.0f));
	Mt = glm::scale(Mt, glm::vec3(6.0f, 6.0f, 6.0f));

	glm::mat4 Mg = glm::scale(M, glm::vec3(8.0f, 6.0f, 6.0f));
	drawGlass(P, V, Mg);

	glfwSwapBuffers(window); //zamiana tylniego bufora na przedni (nastepna klatka)

}


int main(void)
{
	GLFWwindow* window; 

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) { 
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(900, 900, "OpenGL", NULL, NULL); 

	if (!window) 
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); 
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) { 
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	
	float angle = 0; 
	glfwSetTime(0); //ustawia czas na 0
	while (!glfwWindowShouldClose(window))//az do zamkniecia okna 
	{
		angle += speed * glfwGetTime(); //kat obrotu ryb to poprzedni kat plus predkosc ustalona wczesniej i czas od ostatniego wyowlania klatki
		glfwSetTime(0); //czas na 0 (zeby tempo obrotu bylo rowne w czasie niezaleznie od klatek)
		drawScene(window,angle);//rysuj scene
		glfwPollEvents(); //feedback od klawiatury lub myszku
	}

	freeOpenGLProgram(window); //zwolnij tekstury i shadery

	glfwDestroyWindow(window); //zamknij okno
	glfwTerminate(); //wylacz program
	exit(EXIT_SUCCESS);//wyjdz

}
