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

	vector<GLfloat> points; // ���� ��ġ�� ���� ���� �迭.
	vector<GLuint> indices; //indice �迭
	vector<GLfloat> colors;//���� �����ִ� �迭
	int pointNum; //���� ����. (���� ��ȣ) 0���ͽ���
	int polygonPointNum;//�ٰ����� ���� ����. 1���ͽ���.
	int mode; //���� �ٰ����� �׸��� ������� ��ü������ �׸��� ������� ���ϴ� ����.
	GLfloat length;//�� ���� ����.

	glm::mat4 rotMat;//ȸ�� ��Ʈ����.
	GLfloat camCenterZ; //z������ ī�޶��� ��ġ.

	float transX, transY, transZ; //x���̵�, y���̵�, z���̵�
	float x_angle, y_angle, z_angle; //x�� ��, y�ఢ��, z�ఢ��.
	float scale; //ũ������.

	float x_turn_angle; //�����ܰ��� x�࿡ ���Ѳ�������.
	float y_turn_angle; //�����ܰ��� y�࿡ ���� ��������.
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
	//��ü�� �׸����� depth���� ���.
	if (Shape::getInstance()->mode == 2)
	{
		glEnable(GL_DEPTH_TEST);
	}
	Shape* instance = Shape::getInstance();
	//���� ��ǥ��� �ٲ��� ��� �ʱ�ȭ �� in������ ����.
	glm::mat4 worldMat = glm::mat4(0.1f);
	GLuint worldLoc = glGetUniformLocation(g_programID, "worldMat");
	glUniformMatrix4fv(worldLoc, 1, GL_FALSE, &worldMat[0][0]);

	//���� ��ǥ��� �ٲ��� ��� �ʱ�ȭ(������������ �ʱ�ȭ.) �� in������ ����. 
	glm::mat4 projMat = glm::mat4(0.1f);
	projMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 100.0f);
	GLuint projLoc = glGetUniformLocation(g_programID, "projMat");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMat[0][0]);

	//ī�޶���� ��ǥ��� �ٲ��� ��� �ʱ�ȭ �� in������ ����.
	glm::mat4 viewMat = glm::mat4(1.0f);
	GLuint viewLoc = glGetUniformLocation(g_programID, "viewMat");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMat[0][0]);

	glBindVertexArray(instance->VAO[0]);
	if (instance->mode == 0)	//ó�� ���� �ٰ��� �׸���.
	{
		glDrawArrays(GL_LINE_STRIP, 0, instance->points.size() / 6);
	}
	else if (instance->mode == 1)//���� �ٰ��� �ϼ�, ��ü��� �����ֱ��� ȸ��.
	{
		glDrawElements(GL_TRIANGLES, instance->indices.size(), GL_UNSIGNED_INT, ((void*)(0)));
	}
	else if (instance->mode == 2)//��ü����� �����ٶ�.
	{
		//ȸ��, Ȯ��/���, �̵��� ���ش�.
		glm::mat4 tranMat = glm::translate(glm::vec3(instance->transX, instance->transY, instance->transZ));
		glm::mat4 scalMat = glm::scale(glm::mat4(),glm::vec3(instance->scale, instance->scale, instance->scale));
		worldMat = tranMat*scalMat*instance->rotMat;	
		glUniformMatrix4fv(worldLoc, 1, GL_FALSE, &worldMat[0][0]);

		//ī�޶� ������ ����
		viewMat[3] = glm::vec4(0, 0, instance->camCenterZ, 1.0f);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &viewMat[0][0]);

		//������ ������������ ����
		projMat = glm::perspective(120.0f, g_screenWidth/g_screenHeight, 0.1f, 100.0f);
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projMat[0][0]);
		
		glDrawElements(GL_TRIANGLES, instance->indices.size(), GL_UNSIGNED_INT, ((void*)(0)));
	}
	//���� ����
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
	//���콺 ��ǥ�� nomalize ��ǥ�� �ٲٱ�.
	float fx, fy;
	fx = x / (float)(g_screenWidth - 1)*2.0f - 1.0f;
	fy = -(y / (float)(g_screenHeight - 1)*2.0f - 1.0f);
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		//�ٰ����� �׸��� ���.
		if (instance->mode == 0)
		{
			//ó���� ���� ���� �ٽ� Ŭ���ϸ� �׸��� ����. ��� ���� 3���̻���� ����.
			if (instance->points.size() / 6 >= 3 &&
				instance->points[0] - fx >= -0.03&&instance->points[0] - fx <= 0.03
				&&instance->points[1] - fy >= -0.03&&instance->points[1] - fy <= 0.03)
			{
				instance->mode = 1;

				instance->polygonPointNum = instance->pointNum;
			}
			//������ ���. ���� ��´�.
			else
			{
				//���ٲٱ�. �� ������ �ٸ���.
				instance->colors.push_back((fx + 1) / 2);
				instance->colors.push_back((fy + 1) / 4);
				instance->colors.push_back(instance->pointNum / 20);

				//���� �ֱ�.
				instance->points.push_back(fx);
				instance->points.push_back(fy);
				instance->points.push_back(0.0);
				instance->points.push_back(instance->colors[instance->pointNum*3]);
				instance->points.push_back(instance->colors[instance->pointNum*3 + 1]);
				instance->points.push_back(instance->colors[instance->pointNum*3 + 2]);
				glBindBuffer(GL_ARRAY_BUFFER, instance->VBO[0]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*instance->points.size(), instance->points.data(), GL_STATIC_DRAW);

				//index�ֱ�. 
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
		//��ü���� �׸��� ���.
		else if (instance->mode == 1)
		{
			instance->mode = 2;
		}
		else if (instance->mode ==2)
		{
			int level = instance->pointNum/instance->polygonPointNum;
			//������ ���ش�.
			if (level > 1)
			{
				for (int i = 0; i < (instance->polygonPointNum - 2) * 3; i++)
				{
					instance->indices.pop_back();
				}
			}

			extendShape();

			//���λ��� ���� ���鸸���.
			for (int i = 0; i < instance->polygonPointNum; i++)
			{
				//���λ��� ���� ����.
				if (i < instance->polygonPointNum - 1)
				{
					instance->indices.push_back(instance->pointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + 1);

					instance->indices.push_back(instance->pointNum);
					instance->indices.push_back(instance->pointNum + 1);
					instance->indices.push_back(instance->pointNum - instance->polygonPointNum + 1);
				}
				//����������.
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
			//���λ������� ���鸸���.
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
	//���� ����
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

	//������ ����
	else if (key == 'q' || key == 'Q')
		instance->scale -= 0.1;
	else if (key == 'w' || key == 'W')
		instance->scale += 0.1;

	//�� ��������.
	else if (key == 'k' || key == 'K')
		instance->length -= 0.1;
	else if (key == 'l'&& instance->length > 0.1f)
		instance->length += 0.1;

	//������ ���� ���� ����
	else if (key == 'u' || key == 'U')
		instance->x_turn_angle -= 0.1;
	else if (key == 'i' || key == 'I')
		instance->x_turn_angle += 0.1;
	else if (key == 'o' || key == 'O')
		instance->y_turn_angle -= 0.1;
	else if (key == 'p' || key == 'P')
		instance->y_turn_angle += 0.1;

	//ī�޶��� ��, �� �̵�
	else if (key == '[')
		instance->camCenterZ -= 0.1;
	else if (key == ']')
		instance->camCenterZ += 0.1;

	glutPostRedisplay();
}
void mySpecialKey(int key, int x, int y)
{
	Shape* instance = Shape::getInstance();
	//translate���ִ� ����. ���� x�� y�� z��
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
	
	//x, y��������� ������ ȸ��.
	glm::mat4 y_rotAxis = glm::rotate(instance->y_turn_angle,glm::vec3(0,1,0));
	y_rotAxis *= glm::rotate(instance->x_turn_angle, glm::vec3(1, 0, 0));

	glm::vec4* v = new glm::vec4 [instance->polygonPointNum];
	for (int i = 0; i < instance->polygonPointNum; i++)
	{
		int preStart = instance->points.size() - instance->polygonPointNum*6; //�������� ������.
		//�������� ��� ���� z��������� �ö󰡰� ����.
		v[i] = glm::vec4(instance->points[preStart + i * 6], instance->points[preStart + i * 6 + 1], instance->points[preStart + i * 6 + 2]+instance->length , 1.0f);
		v[i] = y_rotAxis * v[i];
	}

	//���ο� ���� �� �Է�.
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
	glutCreateWindow("201411237 ������- Assignment#1");

	//call initization function
	init();

	//instance���� ����.
	Shape* instance = Shape::getInstance();

	//���̴��� ID�ޱ�
	GLuint programID = LoadShaders("VertexShader.txt", "FragmentShader.txt");
	glUseProgram(programID);
	g_programID = programID;
	GLint posID = glGetAttribLocation(g_programID, "pos"); //��ġ
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