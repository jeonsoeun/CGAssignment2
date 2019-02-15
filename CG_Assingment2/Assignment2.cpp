#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<vector>
#include<string>

#include <GL/glew.h>
#include <GL/glut.h>
#include<glm/gtx/transform.hpp>
#include<glm/gtc/quaternion.hpp>
#include<glm/gtx/quaternion.hpp>

using namespace std;

class Shape{
private:
	static Shape*instance;
	Shape(){};

public:
	~Shape(){};
	static Shape* getInstance()//
	{
		if (instance == NULL)
		{
			instance = new Shape();
		}
		return instance;
	}
	GLuint* VBO; //buffer
	GLuint* VAO; //vertex Array
	GLuint* VEO; //element buffer

	vector<GLfloat> points; // 점의 위치와 색을 담은 배열.
	vector<GLuint> indices; //indice 배열
	vector<GLfloat> colors;//색을 정해주는 배열
	int pointNum; //점의 개수. (점의 번호) 0부터시작
	int polygonPointNum;//다각형의 점의 개수. 1부터시작.
	int mode; //현재 다각형을 그리는 모드인지 입체도형을 그리는 모드인지 정하는 변수.
	GLfloat length;//각 층의 높이.

	glm::mat4 rotMat;//회전 매트릭스.
	GLfloat camCenterZ; //z축으로 카메라의 위치.

	float transX, transY, transZ; //x축이동, y축이동, z축이동
	float x_angle, y_angle, z_angle; //x축 각, y축각도, z축각도.
	float scale; //크기조절.

	float x_turn_angle; //다음단계의 x축에 대한꺾임정도.
	float y_turn_angle; //다음단계의 y축에 대한 꺾임정도.
};

float g_screenWidth, g_screenHeight, g_programID;
GLint colID;
Shape* Shape::instance;
void extendShape();

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
	//create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	GLint Result = GL_FALSE;
	int InfoLogLength;

	//Read the vertex shader code from the file
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if (VertexShaderStream.is_open())
	{
		string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	//Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	//Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	//Read the fragment shader code from the file
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if (FragmentShaderStream.is_open())
	{
		string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	//Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	//Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	//Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	vector<char> ProgramErrorMessage(InfoLogLength);
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

void renderScene(void)
{
	//Clear all pixels
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//입체를 그릴때만 depth버퍼 사용.
	if (Shape::getInstance()->mode == 2)
	{
		glEnable(GL_DEPTH_TEST);
	}
	Shape* instance = Shape::getInstance();
	//월드 좌표계로 바꿔줄 행렬 초기화 및 in변수와 연결.
	glm::mat4 worldMat = glm::mat4(0.1f);
	GLuint worldLoc = glGetUniformLocation(g_programID, "worldMat");
	glUniformMatrix4fv(worldLoc, 1, GL_FALSE, &worldMat[0][0]);

	//투영 좌표계로 바꿔줄 행렬 초기화(직교투영으로 초기화.) 및 in변수와 연결. 
	glm::mat4 projMat = glm::mat4(0.1f);
	projMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 100.0f);
	GLuint projLoc = glGetUniformLocation(g_programID, "projMat");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMat[0][0]);

	//카메라시점 좌표계로 바꿔줄 행렬 초기화 및 in변수와 연결.
	glm::mat4 viewMat = glm::mat4(1.0f);
	GLuint viewLoc = glGetUniformLocation(g_programID, "viewMat");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMat[0][0]);

	glBindVertexArray(instance->VAO[0]);
	if (instance->mode == 0)	//처음 점찍어서 다각형 그릴때.
	{
		glDrawArrays(GL_LINE_STRIP, 0, instance->points.size() / 6);
	}
	else if (instance->mode == 1)//점찍어서 다각형 완성, 입체모양 보여주기전 회전.
	{
		glDrawElements(GL_TRIANGLES, instance->indices.size(), GL_UNSIGNED_INT, ((void*)(0)));
	}
	else if (instance->mode == 2)//입체모양을 보여줄때.
	{
		//회전, 확대/축소, 이동을 해준다.
		glm::mat4 tranMat = glm::translate(glm::vec3(instance->transX, instance->transY, instance->transZ));
		glm::mat4 scalMat = glm::scale(glm::mat4(),glm::vec3(instance->scale, instance->scale, instance->scale));
		worldMat = tranMat*scalMat*instance->rotMat;	
		glUniformMatrix4fv(worldLoc, 1, GL_FALSE, &worldMat[0][0]);

		//카메라 시점을 변경
		viewMat[3] = glm::vec4(0, 0, instance->camCenterZ, 1.0f);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMat[0][0]);

		//투영을 원근투영으로 변경
		projMat = glm::perspective(120.0f, g_screenWidth/g_screenHeight, 0.1f, 100.0f);
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMat[0][0]);
		
		glDrawElements(GL_TRIANGLES, instance->indices.size(), GL_UNSIGNED_INT, ((void*)(0)));
	}
	//더블 버퍼
	glutSwapBuffers();
}

void init()
{
	//initilize the glew and check the errors.
	GLenum res = glewInit();
	if (res != GLEW_OK)
	{
		fprintf(stderr, "Error: '%s' \n", glewGetErrorString(res));
	}
	//select the background color
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClearDepth(1.0);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glDepthFunc(GL_LESS);
	Shape* instance = Shape::getInstance();

	instance->mode = 0;
	instance->pointNum = 0;

	instance->transX = 0.0f;
	instance->transY = 0.0f;
	instance->transZ = 0.0f;

	instance->scale = 1;

	instance->x_angle = 0.1;
	instance->y_angle = 0.3;
	instance->z_angle = 0.0;

	glm::mat4 rot_y = glm::rotate(instance->y_angle, glm::vec3(0, 1, 0));
	glm::mat4 rot_x = glm::rotate(instance->x_angle, glm::vec3(1, 0, 0));
	instance->rotMat = rot_y*rot_x;

	instance->length = 0.2;

	instance->x_turn_angle = 0.0;
	instance->y_turn_angle = 0.0;

	instance->camCenterZ = -3;
}

void myMouse(int button, int state, int x, int y)
{
	Shape* instance = Shape::getInstance();
	//마우스 좌표를 nomalize 좌표로 바꾸기.
	float fx, fy;
	fx = x / (float)(g_screenWidth - 1)*2.0f - 1.0f;
	fy = -(y / (float)(g_screenHeight - 1)*2.0f - 1.0f);
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		//다각형을 그리는 모드.
		if (instance->mode == 0)
		{
			//처음에 찍은 점을 다시 클릭하면 그리기 종료. 적어도 점이 3개이상부터 가능.
			if (instance->points.size() / 6 >= 3 &&
				instance->points[0] - fx >= -0.03&&instance->points[0] - fx <= 0.03
				&&instance->points[1] - fy >= -0.03&&instance->points[1] - fy <= 0.03)
			{
				instance->mode = 1;

				instance->polygonPointNum = instance->pointNum;
			}
			//나머지 경우. 점을 찍는다.
			else
			{
				//색바꾸기. 각 점마다 다른색.
				instance->colors.push_back((fx + 1) / 2);
				instance->colors.push_back((fy + 1) / 4);
				instance->colors.push_back(instance->pointNum / 20);

				//정점 넣기.
				instance->points.push_back(fx);
				instance->points.push_back(fy);
				instance->points.push_back(0.0);
				instance->points.push_back(instance->colors[instance->pointNum*3]);
				instance->points.push_back(instance->colors[instance->pointNum*3 + 1]);
				instance->points.push_back(instance->colors[instance->pointNum*3 + 2]);
				glBindBuffer(GL_ARRAY_BUFFER, instance->VBO[0]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*instance->points.size(), instance->points.data(), GL_STATIC_DRAW);

				//index넣기. 
				instance->indices.push_back(instance->pointNum);
				if (instance->pointNum >= 3)
				{
					instance->indices.push_back(instance->pointNum - 1);
					instance->indices.push_back(0);
				}
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->VEO[0]);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLint)*instance->indices.size(), instance->indices.data(), GL_STATIC_DRAW);
				instance->pointNum++;
			}
		}
		//입체모형 그리는 모드.
		else if (instance->mode == 1)
		{
			instance->mode = 2;
		}
		else if (instance->mode ==2)
		{
			int level = instance->pointNum/instance->polygonPointNum;
			//윗면을 없앤다.
			if (level > 1)
			{
				for (int i = 0; i < (instance->polygonPointNum - 2) * 3; i++)
				{
					instance->indices.pop_back();
				}
			}

			extendShape();

			//새로생긴 층의 옆면만들기.
			for (int i = 0; i < instance->polygonPointNum; i++)
			{
				//새로생긴 층의 옆면.
				if (i < instance->polygonPointNum - 1)
				{
					instance->indices.push_back(instance->pointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + 1);

					instance->indices.push_back(instance->pointNum);
					instance->indices.push_back(instance->pointNum + 1);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + 1);
				}
				//마지막옆면.
				else
				{
					instance->indices.push_back(instance->pointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum*2 + 1);

					instance->indices.push_back(instance->pointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + 1);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum*2 + 1);
				}
				instance->pointNum++;
			}
			//새로생긴층의 윗면만들기.
			for (int i = 1; i < instance->polygonPointNum-1; i++)
			{
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + i);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + i + 1);
				
			}
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->VEO[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLint)*instance->indices.size(), instance->indices.data(), GL_STATIC_DRAW);
		}
	}
}

void myKeyboard(unsigned char key, int x, int y)
{
	Shape* instance = Shape::getInstance();
	//각도 조절
	if (key == 'a' || key == 'A')
	{	
		instance->x_angle = 0.1;
		glm::mat4 rot_x = glm::rotate(instance->x_angle, glm::vec3(1, 0, 0));
		instance->rotMat*=rot_x;
	}
	else if (key == 's' || key == 'S')
	{
		instance->y_angle = 0.1;
		glm::mat4 rot_y = glm::rotate(instance->y_angle, glm::vec3(0, 1, 0));
		instance->rotMat*=rot_y;
	}
	else if (key == 'd' || key == 'D')
	{
		instance->z_angle = 0.1;
		glm::mat4 rot_z = glm::rotate(instance->z_angle, glm::vec3(0, 0, 1));
		instance->rotMat*=rot_z;
	}

	//스케일 조정
	else if (key == 'q' || key == 'Q')
		instance->scale -= 0.1;
	else if (key == 'w' || key == 'W')
		instance->scale += 0.1;

	//층 길이조정.
	else if (key == 'k' || key == 'K')
		instance->length -= 0.1;
	else if (key == 'l'&& instance->length > 0.1f)
		instance->length += 0.1;

	//다음층 꺾임 정도 조정
	else if (key == 'u' || key == 'U')
		instance->x_turn_angle -= 0.1;
	else if (key == 'i' || key == 'I')
		instance->x_turn_angle += 0.1;
	else if (key == 'o' || key == 'O')
		instance->y_turn_angle -= 0.1;
	else if (key == 'p' || key == 'P')
		instance->y_turn_angle += 0.1;

	//카메라의 앞, 뒤 이동
	else if (key == '[')
		instance->camCenterZ -= 0.1;
	else if (key == ']')
		instance->camCenterZ += 0.1;

	glutPostRedisplay();
}
void mySpecialKey(int key, int x, int y)
{
	Shape* instance = Shape::getInstance();
	//translate해주는 변수. 각각 x축 y축 z축
	if (key == GLUT_KEY_LEFT)
		instance->transX -= 0.1;
	else if (key == GLUT_KEY_RIGHT)
		instance->transX += 0.1;

	else if (key == GLUT_KEY_DOWN)
		instance->transY -= 0.1;
	else if (key == GLUT_KEY_UP)
		instance->transY += 0.1;

	else if (key == GLUT_KEY_PAGE_UP)
		instance->transZ -= 0.1;
	else if (key == GLUT_KEY_PAGE_DOWN)
		instance->transZ += 0.1;
	
	glutPostRedisplay();
}

void extendShape()
{
	Shape* instance = Shape::getInstance();
	
	//GLfloat xyz[3] = {0,0,0};
	//for (int i = 0; i < 3; i++)
	//{
	//	for (int j = 0; j < instance->polygonPointNum; j++)
	//	{
	//		xyz[i] += instance->points[instance->points.size() - instance->polygonPointNum * 6+6*j+i];
	//	}
	//	xyz[i] = xyz[i] / instance->polygonPointNum;
	//}
	//glm::mat4 retransAxis = glm::translate(glm::vec3(xyz[0],xyz[1],xyz[2]));
	//glm::mat4 transAxis = glm::translate(glm::vec3(-xyz[0], -xyz[1], -xyz[2]));
	
	//x, y축기준으로 다음면 회전.
	glm::mat4 y_rotAxis = glm::rotate(instance->y_turn_angle,glm::vec3(0,1,0));
	y_rotAxis *= glm::rotate(instance->x_turn_angle, glm::vec3(1, 0, 0));

	glm::vec4* v = new glm::vec4 [instance->polygonPointNum];
	for (int i = 0; i < instance->polygonPointNum; i++)
	{
		int preStart = instance->points.size() - instance->polygonPointNum*6; //이전층의 시작점.
		//다음면의 모든 점이 z축기준으로 올라가게 만듬.
		v[i] = glm::vec4(instance->points[preStart + i * 6], instance->points[preStart + i * 6 + 1], instance->points[preStart + i * 6 + 2]+instance->length , 1.0f);
		v[i] = y_rotAxis * v[i];
	}

	//새로운 층의 점 입력.
	for (int i = 0; i < instance->polygonPointNum; i++)
	{
		instance->points.push_back(v[i][0]);
		instance->points.push_back(v[i][1]);
		instance->points.push_back(v[i][2]);
		instance->points.push_back(instance->points[instance->points.size() - instance->polygonPointNum * 6]);
		instance->points.push_back(instance->points[instance->points.size() - instance->polygonPointNum * 6]);
		instance->points.push_back(instance->points[instance->points.size() - instance->polygonPointNum * 6]+0.1);
	}
	glBindBuffer(GL_ARRAY_BUFFER, instance->VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*instance->points.size(), instance->points.data(), GL_STATIC_DRAW);
}
int main(int argc, char**argv){
	//init GLUT and create Window
	//initialize the GLUT
	glutInit(&argc, argv);
	//GLUT_DOUBLE enables double buffering (drawing to a background buffer while the other buffer is displayed)
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA|GLUT_DEPTH);
	//These two functions are used to define the position and size of the window. 
	glutInitWindowPosition(200, 200);

	g_screenWidth = 800;
	g_screenHeight = 500;
	glutInitWindowSize(g_screenWidth, g_screenHeight);

	//This is used to define the name of the window.
	glutCreateWindow("201411237 전소은- Assignment#1");

	//call initization function
	init();

	//instance변수 생성.
	Shape* instance = Shape::getInstance();

	//쉐이더의 ID받기
	GLuint programID = LoadShaders("VertexShader.txt", "FragmentShader.txt");
	glUseProgram(programID);
	g_programID = programID;
	GLint posID = glGetAttribLocation(g_programID, "pos"); //위치
	colID = glGetAttribLocation(g_programID, "col");

	instance->VBO = new GLuint[1];
	glGenBuffers(1, instance->VBO);
	instance->VAO = new GLuint[1];
	glGenVertexArrays(1, &instance->VAO[0]);
	glBindVertexArray(instance->VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, instance->VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*instance->points.size(), instance->points.data(), GL_STATIC_DRAW);

	instance->VEO = new GLuint[1];
	glGenBuffers(1, instance->VEO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->VEO[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*instance->indices.size() * 3, instance->indices.data(), GL_STATIC_DRAW);


	glVertexAttribPointer(posID, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, ((GLvoid*)(0)));
	glEnableVertexAttribArray(posID);

	glVertexAttribPointer(colID, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, ((GLvoid*)(sizeof(GLfloat) * 3)));
	glEnableVertexAttribArray(colID);

	glutMouseFunc(myMouse);
	glutKeyboardFunc(myKeyboard);
	glutSpecialFunc(mySpecialKey);

	glutDisplayFunc(renderScene);

	//enter GLUT event processing cycle
	glutMainLoop();

	glDeleteVertexArrays(1, instance->VAO);

	return 1;

}