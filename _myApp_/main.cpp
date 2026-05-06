#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

// sb7.h 헤더 파일을 포함시킨다.
#include <sb7.h>
#include <vmath.h>
#include <shader.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"
#include "include/Model.h"
#include "include/GunModel.h"

class my_application : public sb7::application
{
public:
	GLuint compile_shader(const char* vs_file, const char* fs_file)
	{
		GLuint vertex_shader = sb7::shader::load(vs_file, GL_VERTEX_SHADER);
		GLuint fragment_shader = sb7::shader::load(fs_file, GL_FRAGMENT_SHADER);
		GLuint program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);
		glLinkProgram(program);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
		return program;
	}

	bool initAudio()
	{
		if (ma_engine_init(NULL, &engine) != MA_SUCCESS)
		{
			printf("Failed to initialize audio engine.\n");
			return false;
		}
		return true;
	}

	void shutdownAudio() {
		ma_engine_uninit(&engine);
	}

	void m4ShootSound() {
		ma_engine_play_sound(&engine, "assets/sounds/m4a1-1.wav", NULL);
	}
	void m4ReloadSound()
	{
		ma_engine_play_sound(&engine, "assets/sounds/m4Reload.wav", NULL);
	}

	void berettaShootSound() {
		ma_engine_play_sound(&engine, "assets/sounds/beretta.wav", NULL);
	}
	void berettaReloadSound()
	{
		ma_engine_play_sound(&engine, "assets/sounds/berettaReload.wav", NULL);
	}
	
	void switchSound()
	{
		ma_engine_play_sound(&engine, "assets/sounds/switch.wav", NULL);
	}

	Ray createRayFromCamera()
	{
		Ray ray;
		ray.origin = cameraPos;
		ray.direction = vmath::normalize(cameraFront);
		return ray;
	}

	bool intersectRayPlane(const Ray& ray, const vmath::vec3& planePoint, const vmath::vec3& planeNormal, float& hitDist)
	{
		float denom = vmath::dot(ray.direction, planeNormal);
		if (fabs(denom) < 1e-6) return false;

		vmath::vec3 diff = planePoint - ray.origin;
		hitDist = vmath::dot(diff, planeNormal) / denom;
		return hitDist >= 0;
	}

	void pointLightsUniform(GLuint& shader_program, int index, vmath::vec3 lPos, vmath::vec3 lAmbient, vmath::vec3 lDiff, vmath::vec3 lSpec, float c1, float c2)
	{
		std::string str = "pointLights[" + std::to_string(index) + "].";
		glUniform3fv(glGetUniformLocation(shader_program, (str + "position").c_str()), 1, lPos);
		glUniform3fv(glGetUniformLocation(shader_program, (str + "ambient").c_str()), 1, lAmbient);
		glUniform3fv(glGetUniformLocation(shader_program, (str + "diffuse").c_str()), 1, lDiff);
		glUniform3fv(glGetUniformLocation(shader_program, (str + "specular").c_str()), 1, lSpec);
		glUniform1f(glGetUniformLocation(shader_program, (str + "c1").c_str()), c1);
		glUniform1f(glGetUniformLocation(shader_program, (str + "c2").c_str()), c2);
	}

	virtual void startup()
	{
		initAudio();

		// 클래스 내부 변수 초기화
		initValues();

		// 쉐이더 프로그램 컴파일 및 연결
		shader_program = compile_shader("assets/shaders/phong_vs.glsl", "assets/shaders/phong_fs.glsl");

		stbi_set_flip_vertically_on_load(true);

		//총기 
		m4Model.init();
		m4Model.loadOBJ("assets/models/m4a1_s.obj");
		m4Model.loadDiffuseMap("assets/textures/m4a1_s.png");

		berettaModel.init();
		berettaModel.loadOBJ("assets/models/Beretta.obj");
		berettaModel.loadDiffuseMap("assets/textures/Berreta M9_Material_BaseColor.png");
		berettaModel.loadSpecularMap("assets/textures/Berreta M9_Material_Roughness.png");

		GLfloat vertice_target[] = {
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,

			 1.0f,  1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f
		};

		GLfloat target_texcoords[] = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,

			1.0f, 1.0f,
			0.0f, 1.0f,
			0.0f, 0.0f
		};

		GLfloat target_normals[] = {
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,

			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f
		};

		//타겟
		targetModel.init();
		targetModel.setupMesh(6, vertice_target, target_texcoords, target_normals);
		targetModel.loadDiffuseMap("assets/textures/target.png");

		//총알 자국
		GLfloat bulletMark_vertices[] =
		{
			-0.05f, -0.05f, 0.0f,
			 0.05f, -0.05f, 0.0f,
			 0.05f,  0.05f, 0.0f,
			-0.05f,  0.05f, 0.0f
		};

		GLfloat bulletMark_texcoords[] = {
			0.0f, 0.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
		};

		GLuint bulletMark_indices[] = {
			0, 1, 2,
			2, 3, 0
		};

		bulletMark.init();
		bulletMark.setupMesh(4, bulletMark_vertices, bulletMark_texcoords, target_normals);
		bulletMark.setupIndices(6, bulletMark_indices);
		bulletMark.loadDiffuseMap("assets/textures/bulletMark.png");


		GLfloat pyramid_vertices[] =
		{
			1.0f, 0.0f, -1.0f,
			-1.0f, 0.0f, -1.0f,
			-1.0f, 0.0f, 1.0f,
			1.0f, 0.0f, 1.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, -1.0f, 0.0f,
		};

		GLuint pyramid_indices[] =
		{
			4, 0, 1,
			4, 1, 2,
			4, 2, 3,
			4, 3, 0,

			5, 1, 0,
			5, 2, 1,
			5, 3, 2,
			5, 0, 3,
		};

		pyramidModel.init();
		pyramidModel.setupMesh(6, pyramid_vertices);
		pyramidModel.setupIndices(24, pyramid_indices);


		//바닥 
		GLfloat vertices_ground[] = {
			-10.0f, 0.0f, -10.0f,
			-10.0f, 0.0f, 10.0f,
			10.0f, 0.0f, -10.0f,
			10.0f, 0.0f, 10.0f,
		};

		GLfloat ground_texcoords[] = {
			0.0f, 5.0f,
			0.0f, 0.0f,
			5.0f, 5.0f,
			5.0f, 0.0f
		};

		GLfloat ground_normals[] = {
			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
			0.0f, 1.0f, 0.0f,
		};

		GLuint ground_indices[] = {
			0, 1, 2,
			2, 1, 3,
		};

		floorModel.init();
		floorModel.setupMesh(4, vertices_ground, ground_texcoords, ground_normals);
		floorModel.setupIndices(6, ground_indices);
		floorModel.loadDiffuseMap("assets/textures/wall.jpg");


		//천장
		ceilingModel.init();
		ceilingModel.setupMesh(4, vertices_ground, ground_texcoords, ground_normals);
		ceilingModel.setupIndices(6, ground_indices);
		ceilingModel.loadDiffuseMap("assets/textures/wall.jpg");

		//벽
		GLfloat vertice_wall[] = {
			-20.0f, -3.0f, 0.0f,
			 20.0f, -3.0f, 0.0f,
			 20.0f,  3.0f, 0.0f,

			 20.0f,  3.0f, 0.0f,
			-20.0f,  3.0f, 0.0f,
			-20.0f, -3.0f, 0.0f
		};

		GLfloat wall_texcoords[] = {
			0.0f, 0.0f,
			6.0f, 0.0f,
			6.0f, 1.0f,

			6.0f, 1.0f,
			0.0f, 1.0f,
			0.0f, 0.0f
		};

		GLfloat wall_normals[] = {
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,

			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f
		};

		GLfloat vertice_wall2[] = {
			-5.0f, -3.0f, 0.0f,
			 5.0f, -3.0f, 0.0f,
			 5.0f,  3.0f, 0.0f,

			 5.0f,  3.0f, 0.0f,
			-5.0f,  3.0f, 0.0f,
			-5.0f, -3.0f, 0.0f
		};

		GLfloat wall_texcoords2[] = {
			0.0f, 0.0f,
			1.5f, 0.0f,
			1.5f, 1.0f,

			1.5f, 1.0f,
			0.0f, 1.0f,
			0.0f, 0.0f
		};

		//벽
		wallSide.init();
		wallSide.setupMesh(6, vertice_wall, wall_texcoords, wall_normals);
		wallSide.loadDiffuseMap("assets/textures/brick.jpg");

		wallSide2.init();
		wallSide2.setupMesh(6, vertice_wall2, wall_texcoords2, wall_normals);
		wallSide2.loadDiffuseMap("assets/textures/brick.jpg");

		//테이블
		tableModel.init();
		tableModel.loadOBJ("assets/models/mytable.obj");
		tableModel.loadDiffuseMap("assets/textures/mytable.jpg");



		glEnable(GL_MULTISAMPLE);
	}

	virtual void shutdown()
	{
		shutdownAudio();
		glDeleteProgram(shader_program);
	}

	virtual void render(double currentTime)
	{
		animationTime += currentTime - previousTime;

		const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glClearBufferfv(GL_COLOR, 0, black);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);


		//카메라 인터랙션 위해서 클래스화 시킴
		// 카메라 매트릭스 계산
		//float distance = 5.f;
		//vmath::vec3 eye((float)cos(animationTime*0.3f)*distance, 1.0, (float)sin(animationTime*0.3f)*distance);
		/*vmath::vec3 eye(0.0f, 1.0f, distance);
		vmath::vec3 center(0.0, 0.0, 0.0);
		vmath::vec3 up(0.0, 1.0, 0.0);*/
		vmath::mat4 lookAt = vmath::lookat(cameraPos, cameraPos + cameraFront, cameraUp);
		vmath::mat4 projM = vmath::perspective(fov, (float)info.windowWidth / (float)info.windowHeight, 0.1f, 1000.0f);

		//for gun
		vmath::mat4 gunView = lookAt;//vmath::mat4::identity();
		vmath::mat4 gunProj = vmath::perspective(fov, (float)info.windowWidth / (float)info.windowHeight, 0.3f, 1000.0f); // near를 작게

		vmath::mat4 lookAtInverse = vmath::mat4(
			vmath::vec4(cameraRight, 0.0f),
			vmath::vec4(cameraUp, 0.0f),
			vmath::vec4(-cameraFront, 0.0f),
			vmath::vec4(cameraPos, 1.0f)
		);


		float deltaTime = 0.0f;
		deltaTime = currentTime - previousTime;

		cameraSpeed = 3.0f * deltaTime;

		vmath::vec3 attemptedMove(0.0f);

		//이동
		if (keys[0])
			attemptedMove += cameraSpeed * vmath::normalize(vmath::vec3(cameraFront[0], 0.0f, cameraFront[2]));
		if (keys[1])
			attemptedMove -= cameraSpeed * vmath::normalize(vmath::vec3(cameraFront[0], 0.0f, cameraFront[2]));
		if (keys[2])
			attemptedMove -= vmath::normalize(vmath::cross(cameraFront, cameraUp)) * cameraSpeed;
		if (keys[3])
			attemptedMove += vmath::normalize(vmath::cross(cameraFront, cameraUp)) * cameraSpeed;

		vmath::vec3 newPos = cameraPos + attemptedMove;
		float nx = newPos[0], nz = newPos[2];

		//공간 충돌 판정 좌표 검증
		bool inA = (nx >= -9.0f && nx <= 49.0f && nz >= -9.0f && nz <= 29.0f);
		bool notInB = (nx >= -6.0f && nx <= 29.0f && nz >= 9.0f && nz <= 21.0f);
		bool notInC = (nx >= 29.0f && nx <= 49.0f && nz >= -9.0f && nz <= 9.0f);
		bool notInD = (nx >= -1.4f && nx <= 1.4f && nz >= -9.0f && nz <= 5.0f);

		if (inA && !notInB && !notInC && !notInD)
			cameraPos = newPos;

		//점프
		if (keys[4])
		{
			float jumpTime = currentTime - previousTime; //1 frame
			cameraPos[1] += jumpVelocity * jumpTime;
			jumpVelocity -= 15.0f * jumpTime;

			if (cameraPos[1] <= 1.0f)
			{
				cameraPos[1] = 1.0f;
				jumpVelocity = 5.0f;
				keys[4] = false;
			}

		}

		//총기 선택
		if (guns[0])
			gunModel = &m4Model;
		else if (guns[1])
			gunModel = &berettaModel;

		
		if (gunModel->isRecoiling)
		{
			//총기 반동
			float recoilTime = currentTime - previousTime;
			gunModel->recoilOffset -= gunModel->recoilRecoverySpeed * recoilTime;

			if (gunModel->recoilOffset <= 0.0f)
			{
				gunModel->recoilOffset = 0.0f;
				gunModel->isRecoiling = false;
			}

			//카메라 반동
			float CameraRecoilTime = currentTime - previousTime;
			gunModel->cameraRecoilOffset -= gunModel->cameraRecoilRecoverySpeed * CameraRecoilTime;

			if (gunModel->cameraRecoilOffset <= 0.0f)
			{
				gunModel->cameraRecoilOffset = 0.0f;
			}
			if (gunModel->cameraRecoilOffset > 2.0f)
			{
				gunModel->cameraRecoilOffset--;
			}

			float recoilPitch = pitch + gunModel->cameraRecoilOffset;

			vmath::vec3 direction;
			direction[0] = cos(vmath::radians(yaw)) * cos(vmath::radians(recoilPitch));
			direction[1] = sin(vmath::radians(recoilPitch));
			direction[2] = sin(vmath::radians(yaw)) * cos(vmath::radians(recoilPitch));
			cameraFront = vmath::normalize(direction);

			//cameraFront 활용하여 제대로된 업벡터 구함
			Up = vmath::vec3(0.0f, 1.0f, 0.0f);
			cameraRight = vmath::normalize(vmath::cross(cameraFront, Up));
			cameraUp = vmath::normalize(vmath::cross(cameraRight, cameraFront));

		}
		//printf("%f\n", gunModel->cameraRecoilOffset);

		if (keys[5]) //사격 처리
		{
			if (guns[0])
			{
				if (gunModel->fullAuto)
				{
					if (gunModel->shoot(currentTime))
					{
						m4ShootSound();
						Ray bulletRay = createRayFromCamera();
						//printf("Ray Origin: (%.2f, %.2f, %.2f)\n", bulletRay.origin[0], bulletRay.origin[1], bulletRay.origin[2]);
						//printf("Ray Dir: (%.2f, %.2f, %.2f)\n", bulletRay.direction[0], bulletRay.direction[1], bulletRay.direction[2]);
						for (int i = 0; i < 4; i++)
						{
							if (intersectRayPlane(bulletRay, targetCenter[i], targetNormal, hitDist))
							{
								vmath::vec3 hitPoint = bulletRay.origin + hitDist * bulletRay.direction;

								// target의 로컬 좌표계 기준으로 hitPoint 위치를 확인
								vmath::vec3 localHit = hitPoint - targetCenter[i];

								// 회전 고려: 타겟이 YZ 평면이므로 localHit.y와 localHit.z 기준
								float halfSize = 1.0f; // 한 변이 2.0f니까 반은 1.0f
								if (fabs(localHit[1]) <= halfSize && fabs(localHit[2]) <= halfSize)
								{
									printf("Hit at (%.2f, %.2f, %.2f)\n", hitPoint[0], hitPoint[1], hitPoint[2]);
									vmath::vec3 hitPos = bulletRay.origin + hitDist * bulletRay.direction;
									bulletHoles.push_back(hitPos);
								}
							}
						}

					}
				}
				else
				{
					if (gunModel->shoot(currentTime))
					{
						m4ShootSound();
						Ray bulletRay = createRayFromCamera();
						//printf("Ray Origin: (%.2f, %.2f, %.2f)\n", bulletRay.origin[0], bulletRay.origin[1], bulletRay.origin[2]);
						//printf("Ray Dir: (%.2f, %.2f, %.2f)\n", bulletRay.direction[0], bulletRay.direction[1], bulletRay.direction[2]);

						for (int i = 0; i < 4; i++)
						{
							if (intersectRayPlane(bulletRay, targetCenter[i], targetNormal, hitDist))
							{
								vmath::vec3 hitPoint = bulletRay.origin + hitDist * bulletRay.direction;

								// target의 로컬 좌표계 기준으로 hitPoint 위치를 확인
								vmath::vec3 localHit = hitPoint - targetCenter[i];

								// 회전 고려: 타겟이 YZ 평면이므로 localHit.y와 localHit.z 기준
								float halfSize = 1.0f; 
								if (fabs(localHit[1]) <= halfSize && fabs(localHit[2]) <= halfSize)
								{
									printf("Hit at (%.2f, %.2f, %.2f)\n", hitPoint[0], hitPoint[1], hitPoint[2]);
									vmath::vec3 hitPos = bulletRay.origin + hitDist * bulletRay.direction;
									bulletHoles.push_back(hitPos);
								}
							}
						}
					}
					keys[5] = false; //반자동
				}
			}

			if (guns[1])
			{
				if (gunModel->shoot(currentTime))
				{
					berettaShootSound();
					Ray bulletRay = createRayFromCamera();
					//printf("Ray Origin: (%.2f, %.2f, %.2f)\n", bulletRay.origin[0], bulletRay.origin[1], bulletRay.origin[2]);
					//printf("Ray Dir: (%.2f, %.2f, %.2f)\n", bulletRay.direction[0], bulletRay.direction[1], bulletRay.direction[2]);

					for (int i = 0; i < 4; i++)
					{
						if (intersectRayPlane(bulletRay, targetCenter[i], targetNormal, hitDist))
						{
							vmath::vec3 hitPoint = bulletRay.origin + hitDist * bulletRay.direction;

							// target의 로컬 좌표계 기준으로 hitPoint 위치를 확인
							vmath::vec3 localHit = hitPoint - targetCenter[i];

							// 회전 고려: 타겟이 YZ 평면이므로 localHit.y와 localHit.z 기준
							float halfSize = 1.0f; 
							if (fabs(localHit[1]) <= halfSize && fabs(localHit[2]) <= halfSize)
							{
								printf("Hit at (%.2f, %.2f, %.2f)\n", hitPoint[0], hitPoint[1], hitPoint[2]);
								vmath::vec3 hitPos = bulletRay.origin + hitDist * bulletRay.direction;
								bulletHoles.push_back(hitPos);
							}
						}
					}
				}
					
				keys[5] = false;
			}

		}

		// 라이팅 설정 ---------------------------------------
		float r = 2.f;
		//vmath::vec3 pointLightPos = vmath::vec3(0.f, 5.0f, 0.0f);//::vec3((float)sin(animationTime * 0.5f) * r, 0.f, (float)cos(animationTime * 0.5f) * r);
		vmath::vec3 pointLightPos[] = 
		{
				vmath::vec3(0.f, 5.0f, 0.0f),
				vmath::vec3(10.0f, 5.0f, 0.0f),
				vmath::vec3(20.0f, 5.0f, 0.0f),
				vmath::vec3(-7.5f, 5.0f, 15.0f),
				vmath::vec3(-7.5f, 5.0f, 25.0f),
				vmath::vec3(10.0f, 5.0f, 25.0f),
				vmath::vec3(30.0f, 5.0f, 25.0f),
				vmath::vec3(28.0f, 5.0f, -6.0f),
				vmath::vec3(28.0f, 5.0f, 0.0f),
				vmath::vec3(28.0f, 5.0f, 6.0f),
				vmath::vec3(48.0f, 5.0f, 25.0f),
		};
		
		vmath::vec3 viewPos = cameraPos;

		vmath::vec3 pointLightAmbient(0.15f, 0.15f, 0.15f);
		vmath::vec3 pointLightDiffuse(0.6f, 0.6f, 0.6f);
		vmath::vec3 pointLightSpecular(0.5f, 0.5f, 0.5f);

		// 모델 그리기 ---------------------------------------
		glUseProgram(shader_program);

		glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, projM);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, lookAt);
		glUniform3fv(glGetUniformLocation(shader_program, "viewPos"), 1, viewPos);

		//라이팅 유니폼 전달
		glUniform3f(glGetUniformLocation(shader_program, "dirLight.direction"), 0.0f, -1.0f, 0.0f);
		glUniform3f(glGetUniformLocation(shader_program, "dirLight.ambient"), 0.1f, 0.1f, 0.1f);
		glUniform3f(glGetUniformLocation(shader_program, "dirLight.diffuse"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(shader_program, "dirLight.specular"), 0.0f, 0.0f, 0.0f);


		//glUniform3fv(glGetUniformLocation(shader_program, "pointLights[0].position"), 1, pointLightPos[0]);
		//glUniform3fv(glGetUniformLocation(shader_program, "pointLights[0].ambient"), 1, pointLightAmbient);
		//glUniform3fv(glGetUniformLocation(shader_program, "pointLights[0].diffuse"), 1, pointLightDiffuse);
		//glUniform3fv(glGetUniformLocation(shader_program, "pointLights[0].specular"), 1, pointLightSpecular);
		//glUniform1f(glGetUniformLocation(shader_program, "pointLights[0].c1"), 0.09f);
		//glUniform1f(glGetUniformLocation(shader_program, "pointLights[0].c2"), 0.032f);
		
		for (int i = 0; i < 7; i++)
		{
			pointLightsUniform(shader_program, i, pointLightPos[i], pointLightAmbient, pointLightDiffuse, pointLightSpecular, 0.09f, 0.032f);
		}
		
		pointLightsUniform(shader_program, 7, pointLightPos[7], vmath::vec3(0.15f, 0.0f, 0.0f), vmath::vec3(0.6f, 0.0f, 0.0f), vmath::vec3(0.5f, 0.0f, 0.0f), 0.09f, 0.032f);
		pointLightsUniform(shader_program, 8, pointLightPos[8], vmath::vec3(0.0f, 0.15f, 0.0f), vmath::vec3(0.0f, 0.6f, 0.0f), vmath::vec3(0.0f, 0.5f, 0.0f), 0.09f, 0.032f);
		pointLightsUniform(shader_program, 9, pointLightPos[9], vmath::vec3(0.0f, 0.0f, 0.15f), vmath::vec3(0.0f, 0.0f, 0.6f), vmath::vec3(0.0f, 0.0f, 0.5f), 0.09f, 0.032f);
		pointLightsUniform(shader_program, 10, pointLightPos[10], pointLightAmbient, pointLightDiffuse, pointLightSpecular, 0.09f, 0.032f);

		//spotLight
		glUniform1i(glGetUniformLocation(shader_program, "flashlightOn"), flashlightOn ? 1 : 0);
		glUniform3fv(glGetUniformLocation(shader_program, "spotLight.position"), 1, cameraPos);
		glUniform3fv(glGetUniformLocation(shader_program, "spotLight.direction"), 1, cameraFront);
		glUniform1f(glGetUniformLocation(shader_program, "spotLight.cutOff"), (float)cos(vmath::radians(12.5)));
		glUniform1f(glGetUniformLocation(shader_program, "spotLight.outerCutOff"), (float)cos(vmath::radians(15.5)));
		glUniform1f(glGetUniformLocation(shader_program, "spotLight.c1"), 0.09f);
		glUniform1f(glGetUniformLocation(shader_program, "spotLight.c2"), 0.032f);
		glUniform3f(glGetUniformLocation(shader_program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(shader_program, "spotLight.diffuse"), 1.0f, 1.0f, 1.0f);
		glUniform3f(glGetUniformLocation(shader_program, "spotLight.specular"), 1.0f, 1.0f, 1.0f);

		if (lineMode)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//전등
		if (drawLight) 
		{	
			for (int i = 0; i < 11; i++)
			{
				vmath::mat4 transform = vmath::translate(pointLightPos[i]) * vmath::scale(0.05f);
				glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, transform);
				pyramidModel.draw(shader_program);

			}	
		}

		//타겟
		for (int i = 0; i < 4; i++)
		{
			vmath::mat4 model = vmath::translate(targetCenter[i]) *
				vmath::rotate(-90.0f, 0.0f, 1.0f, 0.0f);
			glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, model);
			targetModel.draw(shader_program);
		}

		//총알 자국
		for (int i = 0; i < bulletHoles.size(); i++)
		{
			vmath::mat4 bulletModel = vmath::translate(vmath::vec3(bulletHoles[i][0] - 0.1f, bulletHoles[i][1], bulletHoles[i][2])) *
				vmath::rotate(-90.0f, 0.0f, 1.0f, 0.0f) *
				vmath::scale(0.5f);
			glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, bulletModel);
			bulletMark.draw(shader_program);
		}


		//테이블
		vmath::mat4 model = vmath::translate(vmath::vec3(0.0f, 0.0f, -8.0f)) * vmath::scale(0.4f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, model);
		tableModel.draw(shader_program);

		model = vmath::translate(vmath::vec3(0.0f, 0.0f, -4.5f)) * vmath::scale(0.4f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, model);
		tableModel.draw(shader_program);

		model = vmath::translate(vmath::vec3(0.0f, 0.0f, -1.0f)) * vmath::scale(0.4f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, model);
		tableModel.draw(shader_program);

		model = vmath::translate(vmath::vec3(0.0f, 0.0f, 2.5f)) * vmath::scale(0.4f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, model);
		tableModel.draw(shader_program);

		//바닥
		vmath::mat4 modelWall = vmath::translate(objPosition);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelWall);
		floorModel.draw(shader_program);

		modelWall = vmath::translate(vmath::vec3(0.0f, 0.0f, 20.0f)) * vmath::translate(objPosition);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelWall);
		floorModel.draw(shader_program);

		modelWall = vmath::translate(vmath::vec3(20.0f, 0.0f, 0.0f)) * vmath::translate(objPosition);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelWall);
		floorModel.draw(shader_program);

		modelWall = vmath::translate(vmath::vec3(20.0f, 0.0f, 20.0f)) * vmath::translate(objPosition);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelWall);
		floorModel.draw(shader_program);

		modelWall = vmath::translate(vmath::vec3(40.0f, 0.0f, 20.0f)) * vmath::translate(objPosition);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelWall);
		floorModel.draw(shader_program);

		//천장
		vmath::mat4 modelCeiling = vmath::translate(vmath::vec3(0.0f, 5.0f, 0.0f)) * vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelCeiling);
		ceilingModel.draw(shader_program);

		modelCeiling = vmath::translate(vmath::vec3(0.0f, 5.0f, 20.0f)) * vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelCeiling);
		ceilingModel.draw(shader_program);

		modelCeiling = vmath::translate(vmath::vec3(20.0f, 5.0f, 0.0f)) * vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelCeiling);
		ceilingModel.draw(shader_program);

		modelCeiling = vmath::translate(vmath::vec3(20.0f, 5.0f, 20.0f)) * vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelCeiling);
		ceilingModel.draw(shader_program);

		modelCeiling = vmath::translate(vmath::vec3(40.0f, 5.0f, 20.0f)) * vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelCeiling);
		ceilingModel.draw(shader_program);

		//벽
		vmath::mat4 modelbrick = vmath::translate(vmath::vec3(10.0f, 2.0f, -10.0f));
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(15.0f, 2.0f, 10.0f)) *
			vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(15.0f, 2.0f, 20.0f)) *
			vmath::rotate(0.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(10.0f, 2.0f, 30.0f)) *
			vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(-10.0f, 2.0f, 10.0f)) *
			vmath::rotate(90.0f, 0.0f, 1.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(30.0f, 2.0f, -10.0f)) *
			vmath::rotate(270.0f, 0.0f, 1.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(-5.0f, 2.0f, 15.0f)) *
			vmath::rotate(270.0f, 0.0f, 1.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide2.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(50.0f, 2.0f, 30.0f)) *
			vmath::rotate(180.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(55.0f, 2.0f, 20.0f)) *
			vmath::rotate(0.0f, 1.0f, 0.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		modelbrick = vmath::translate(vmath::vec3(50.0f, 2.0f, 20.0f)) *
			vmath::rotate(270.0f, 0.0f, 1.0f, 0.0f);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelbrick);
		wallSide.draw(shader_program);

		// 총기 렌더링 전에 바인딩
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, gunProj);
		glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, gunView);

		//총기
		if (guns[0])
		{
			//printf("%f %f\n", riflePos[3][1], pistolPos[3][1]);
			float changeTime = currentTime - previousTime;

			//총기 변경
			if (gunChanging)
			{
				gunChanged = false;
				pistolPos[3][1] -= changeVelocity * changeTime;
				changeVelocity -= 4.0f * changeTime;

				vmath::mat4 modelGun = lookAtInverse *
					pistolPos *
					vmath::rotate(180.0f, 0.0f, 1.0f, 0.0f) *
					vmath::scale(0.05f);

				glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelGun);
				berettaModel.draw(shader_program);
				//gunModel->draw(shader_program);
				if (pistolPos[3][1] <= -0.75f)
					gunChanging = false;
			}

			else
			{
				if (pistolPos[3][1] <= -0.25f)
				{
					riflePos[3][1] = pistolPos[3][1];
					pistolPos[3][1] -= changeVelocity * changeTime;
					changeVelocity -= 4.0 * changeTime;
				}
				else
				{
					riflePos[3][1] = -0.3f;
					changeVelocity = 2.0f;
					gunChanged = true;
				}

				//재장전 메커니즘
				vmath::mat4 reloadTranslate;// = vmath::translate(vmath::vec3(0.0f, 0.0f, 0.0f));
				if (gunModel->isReloading)
				{
					//printf("realoding!\n");
					reloadTranslate = vmath::translate(vmath::vec3(0.0f, 0.0f, 0.0f)) * vmath::rotate(-10.0f, 1.0f, 0.0f, 0.0f);

					float reloading = currentTime - previousTime; //0.01sec?
					gunModel->reloadTime -= reloading;

					if (gunModel->reloadTime <= 0.0f)
					{
						gunModel->reloadTime = 3.0f;
						gunModel->isReloading = false;
						gunModel->currentAmmo = gunModel->maxAmmo;
					}
				}
				else
				{
					reloadTranslate = vmath::translate(vmath::vec3(0.0f, 0.0f, 0.0f));
				}



				// 총기 반동
				vmath::mat4 recoilTranslate = vmath::translate(vmath::vec3(0.0f, gunModel->recoilOffset / 4.0f, gunModel->recoilOffset / 1.5f));

				vmath::mat4 modelGun = lookAtInverse *
					reloadTranslate *
					recoilTranslate *
					riflePos *
					vmath::rotate(180.0f, 0.0f, 1.0f, 0.0f) *
					vmath::scale(0.1f);

				glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelGun);
				//m4Model.draw(shader_program);
				gunModel->draw(shader_program);
			}
		}

		if (guns[1])
		{
			//printf("%f %f\n", riflePos[3][1], pistolPos[3][1]);
			float changeTime = currentTime - previousTime;

			if (gunChanging)
			{
				gunChanged = false;
				riflePos[3][1] -= changeVelocity * changeTime;
				changeVelocity -= 4.0f * changeTime;

				vmath::mat4 modelGun = lookAtInverse *
					riflePos *
					vmath::rotate(180.0f, 0.0f, 1.0f, 0.0f) *
					vmath::scale(0.1f);

				glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelGun);
				m4Model.draw(shader_program);
				//gunModel->draw(shader_program);
				if (riflePos[3][1] <= -0.7f)
					gunChanging = false;

			}

			else
			{
				if (riflePos[3][1] <= -0.25f)
				{
					pistolPos[3][1] = riflePos[3][1];
					riflePos[3][1] -= changeVelocity * changeTime;
					changeVelocity -= 4.0f * changeTime;
				}

				else
				{
					pistolPos[3][1] = -0.25f;
					changeVelocity = 2.0f;
					gunChanged = true;
				}

				//재장전 메커니즘
				vmath::mat4 reloadTranslate;// = vmath::translate(vmath::vec3(0.0f, 0.0f, 0.0f));
				if (gunModel->isReloading)
				{
					//printf("realoding!\n");
					reloadTranslate = vmath::translate(vmath::vec3(0.0f, 0.15f, 0.0f)) * vmath::rotate(-15.0f, 1.0f, 0.0f, 0.0f);

					float reloading = currentTime - previousTime; //0.01sec?
					gunModel->reloadTime -= reloading;

					if (gunModel->reloadTime <= 0.0f)
					{
						gunModel->reloadTime = 3.0f;
						gunModel->isReloading = false;
						gunModel->currentAmmo = gunModel->maxAmmo;
					}
				}
				else
				{
					reloadTranslate = vmath::translate(vmath::vec3(0.0f, 0.0f, 0.0f));
				}


				//반동
				vmath::mat4 recoilTranslate = vmath::translate(vmath::vec3(0.0f, gunModel->recoilOffset / 2.5f, gunModel->recoilOffset) * 1.2f);

				vmath::mat4 modelGun = lookAtInverse *
					reloadTranslate *
					recoilTranslate *
					pistolPos *
					vmath::rotate(180.0f, 0.0f, 1.0f, 0.0f) *
					vmath::scale(0.05f);

				glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, modelGun);
				//berettaModel.draw(shader_program);
				gunModel->draw(shader_program);
			}
		}

		

		previousTime = currentTime;
	}

	virtual void onResize(int w, int h)
	{
		sb7::application::onResize(w, h);

		if (glViewport != NULL)
			glViewport(0, 0, info.windowWidth, info.windowHeight);
	}

	virtual void init()
	{
		sb7::application::init();

		info.samples = 8;
		info.flags.debug = 1;
	}

	virtual void onKey(int key, int action)
	{
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case 'P':
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				break;
			case '1': //라이플
				if (guns[0] != true && gunChanged)
				{
					for (int i = 0; i < sizeof(guns) / sizeof(bool); i++)
					{
						guns[i] = false;
					}
					gunChanging = true;
					guns[0] = true;
				}

				break;
			case '2': //베레타
				//drawLight = !drawLight;
				if (guns[1] != true && gunChanged)
				{
					for (int i = 0; i < sizeof(guns) / sizeof(bool); i++)
					{
						guns[i] = false;
					}
					gunChanging = true;
					guns[1] = true;
				}
				break;
			case 'M':
				lineMode = !lineMode;
				break;
			case 'R':
				//initValues();
				if (!gunModel->isReloading && gunChanged)
				{
					gunModel->isReloading = true;
					if (guns[0])
						m4ReloadSound();
					if (guns[1])
						berettaReloadSound();
				}
				break;

			case 'W':
				keys[0] = true;
				break;
			case 'S':
				keys[1] = true;
				break;
			case 'A':
				keys[2] = true;
				break;
			case 'D':
				keys[3] = true;
				break;
			case GLFW_KEY_SPACE:
				if (keys[4] != true)
					keys[4] = true;
				break;
			case 'B':
				if (guns[0])
				{
					switchSound();
					gunModel->fullAuto = gunModel->fullAuto ? false : true;
					printf("%d\n", gunModel->fullAuto);
				}
				break;
			case 'F':
				//printf("%f %f %f\n", cameraPos[0], cameraPos[1], cameraPos[2]);
				flashlightOn = flashlightOn ? false : true;
				break;

			default:
				break;
			}
		}

		/*if (action == GLFW_REPEAT)
		{

			switch (key)
			{
			case 'W':
				cameraPos += cameraSpeed * vmath::normalize(vmath::vec3(cameraFront[0], 0.0f, cameraFront[2]));
				break;
			case 'S':
				cameraPos -= cameraSpeed * vmath::normalize(vmath::vec3(cameraFront[0], 0.0f, cameraFront[2]));
				break;
			case 'A':
				cameraPos -= vmath::normalize(vmath::cross(cameraFront, cameraUp)) * cameraSpeed;
				break;
			case 'D':
				cameraPos += vmath::normalize(vmath::cross(cameraFront, cameraUp)) * cameraSpeed;
				break;

			default:
				break;
			}
		}*/

		if (action == GLFW_RELEASE)
		{
			switch (key)
			{
			case 'W':
				keys[0] = false;
				break;
			case 'S':
				keys[1] = false;
				break;
			case 'A':
				keys[2] = false;
				break;
			case 'D':
				keys[3] = false;
				break;
				/*case GLFW_KEY_SPACE:
					keys[4] = false;
					break;*/

			default:
				break;
			}
		}
	}

	virtual void onMouseButton(int button, int action)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && gunChanged)
		{
			if (action == GLFW_PRESS)
			{
				keys[5] = true;  // 마우스 누름: 사격 시작
			}
			else if (action == GLFW_RELEASE)
			{
				keys[5] = false; // 마우스 뗌: 사격 멈춤
			}
		}
	}

	virtual void onMouseMove(int x, int y)
	{
		if (firstMouse)
		{
			lastX = x;
			lastY = y;
			firstMouse = false;
		}

		float xoffset = x - lastX;
		float yoffset = lastY - y;

		lastX = x;
		lastY = y;

		float sensitivity = 0.1f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		//cameraFront: 카메라가 바라보는 방향 구함
		vmath::vec3 direction;
		direction[0] = cos(vmath::radians(yaw)) * cos(vmath::radians(pitch));
		direction[1] = sin(vmath::radians(pitch));
		direction[2] = sin(vmath::radians(yaw)) * cos(vmath::radians(pitch));
		cameraFront = vmath::normalize(direction);

		//cameraFront 활용하여 제대로된 업벡터 구함
		Up = vmath::vec3(0.0f, 1.0f, 0.0f);
		cameraRight = vmath::normalize(vmath::cross(cameraFront, Up));
		cameraUp = vmath::normalize(vmath::cross(cameraRight, cameraFront));
	}

#define MAX_FOV 120.f
#define MIN_FOV 10.f
	virtual void onMouseWheel(int pos)
	{
		if (pos > 0)
			fov = vmath::min(MAX_FOV, fov + 1.0f);
		else
			fov = vmath::max(MIN_FOV, fov - 1.0f);
	}

	void initValues()
	{
		drawLight = true;
		animationTime = 0;
		previousTime = 0;
		lineMode = false;

		fov = 50.f;

		objPosition = vmath::vec3(0.0f, -1.0f, 0.0f);
	}


private:
	GLuint shader_program;

	Model pyramidModel;
	vmath::vec3 objPosition;

	//총기
	GunModel m4Model{ false, 20, 20 }, berettaModel{ false, 12, 12, false, 1.6f };
	GunModel* gunModel = &m4Model;
	bool guns[2] = { true, false };
	bool gunChanging = false;
	bool gunChanged = false;
	float changeVelocity = 2.0;

	//총기 위치
	vmath::mat4 riflePos = vmath::translate(vmath::vec3(0.2f, -0.3f, -0.5f));
	vmath::mat4 pistolPos = vmath::translate(vmath::vec3(0.15f, -0.25f, -0.7f));

	//일단 실험
	Model floorModel, ceilingModel;
	Model wallSide, wallSide2;
	Model tableModel;
	Model targetModel;

	//타겟 충돌 판정
	vmath::vec3 targetCenter[4] = { vmath::vec3(29.9f, 1.8f, -6.0f), vmath::vec3(29.9f, 1.8f, 0.0f), vmath::vec3(29.9f, 1.8f, 6.0f), vmath::vec3(49.9f, 1.8f, 25.0f) };
	vmath::vec3 targetNormal = vmath::vec3(-1.0f, 0.0f, 0.0f);
	float hitDist;
	std::vector<vmath::vec3> bulletHoles;
	Model bulletMark;

	//카메라 관련 요소
	vmath::vec3 cameraPos = vmath::vec3(-5.0f, 1.0f, 3.0f);
	vmath::vec3 cameraFront = vmath::vec3(0.0f, 0.0f, -1.0f);
	vmath::vec3 cameraUp = vmath::vec3(0.0f, 1.0f, 0.0f);
	float cameraSpeed;// = 0.01f;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float lastX = info.windowWidth / 2.0f;
	float lastY = info.windowHeight / 2.0f;
	bool firstMouse = true;
	vmath::vec3 Up;
	vmath::vec3 cameraRight;

	bool flashlightOn = false;

	bool keys[6]; //키보드 조작 WSAD, 스페이스바, 마우스 좌클릭(사격)
	float jumpVelocity = 5.0f;

	bool drawLight;
	bool lineMode;
	double previousTime;
	double animationTime;

	float fov;

	//사운드
	ma_engine engine;
};

// DECLARE_MAIN의 하나뿐인 인스턴스
DECLARE_MAIN(my_application)
