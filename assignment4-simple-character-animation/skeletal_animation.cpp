#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_map>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void updateThirdPersonCamera();
glm::vec3 SampleRootTranslation(Animation* animation, const std::string& rootBoneName, float animationTime);
glm::vec3 EstimateRootLoopDisplacement(Animation* animation, const std::string& rootBoneName);
glm::vec3 RotateDeltaByYaw(const glm::vec3& delta, float yawDegrees);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
const float orbitYaw = -90.0f; // Static camera YAW
float orbitPitch = -15.0f;
float cameraDistance = 6.0f;
glm::vec3 cameraTargetOffset = glm::vec3(0.0f, 1.2f, 0.0f);

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 characterPosition = glm::vec3(0.0f); // Character stays at origin
const float characterYaw = 0.0f; // Static character rotation
const float walkSpeed = 1.25f;
const float runSpeed = 4.0f;
const float characterHeightOffset = -0.0f;
const std::string ROOT_BONE_NAME = "mixamorig:Hips";
Animation* activeAnimation = nullptr;
std::unordered_map<const Animation*, glm::vec3> rootLoopDisplacements;
glm::vec3 previousRootSample(0.0f);
float previousRootTime = 0.0f;
bool rootMotionInitialized = false;
AnimBlendState animBlendState = AnimBlendState::IDLE;
float blendAmount = 0.1f;
const float blendRate = 0.00001f; 

// ground plane move instead of character
glm::vec3 groundPosition = glm::vec3(0.0f); 
float groundYaw = 0.0f; 

enum class MovementState {
	IDLE,
	WALK,
	RUN
};

enum class AnimBlendState {
	IDLE,
	IDLE_TO_WALK,
	WALK,
	WALK_TO_IDLE,
	WALK_TO_RUN,
	RUN,
	RUN_TO_WALK
};

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// mouse capture
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// flip texture
	stbi_set_flip_vertically_on_load(true);

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader ourShader("anim_model.vs", "anim_model.fs");
	Shader groundShader("ground.vs", "ground.fs");

	
	// load models
	// -----------
	Model ourModel(FileSystem::getPath("resources/objects/mixamo/warrock.dae"));
	Animation idleAnimation(FileSystem::getPath("resources/objects/mixamo/idle.dae"),&ourModel);
	Animation walkAnimation(FileSystem::getPath("resources/objects/mixamo/walk.dae"), &ourModel);
	Animation runAnimation(FileSystem::getPath("resources/objects/mixamo/run.dae"), &ourModel);
	Animator animator(&idleAnimation);
	MovementState movementState = MovementState::IDLE;
	activeAnimation = &idleAnimation;
	rootLoopDisplacements[&idleAnimation] = glm::vec3(0.0f);
	rootLoopDisplacements[&walkAnimation] = EstimateRootLoopDisplacement(&walkAnimation, ROOT_BONE_NAME);
	rootLoopDisplacements[&runAnimation] = EstimateRootLoopDisplacement(&runAnimation, ROOT_BONE_NAME);
	rootMotionInitialized = false;

	const float groundHalfSize = 5.0f; 
	const float groundHeight = characterHeightOffset - 0.1f;
	float groundVertices[] = {
		// positions                         // normals        // tex coords 
		 groundHalfSize, groundHeight,  groundHalfSize,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
		-groundHalfSize, groundHeight,  groundHalfSize,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
		-groundHalfSize, groundHeight, -groundHalfSize,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,

		 groundHalfSize, groundHeight,  groundHalfSize,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
		-groundHalfSize, groundHeight, -groundHalfSize,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
		 groundHalfSize, groundHeight, -groundHalfSize,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
	};

	unsigned int groundVAO, groundVBO;
	glGenVertexArrays(1, &groundVAO);
	glGenBuffers(1, &groundVBO);
	glBindVertexArray(groundVAO);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glBindVertexArray(0);

	unsigned int groundTexture = loadTexture(FileSystem::getPath("resources/textures/checkerboard.png").c_str());
	groundShader.use();
	groundShader.setInt("groundTexture", 0);

	updateThirdPersonCamera();

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		bool forwardPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
		bool runPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

		// Static facing direction (no camera-based rotation)
		glm::vec3 facingDir = glm::vec3(0.0f, 0.0f, -1.0f); // Fixed forward direction

		switch (animBlendState)
		{
		case AnimBlendState::IDLE:
			movementState = MovementState::IDLE;
			activeAnimation = &idleAnimation;
			animator.PlayAnimation(&idleAnimation, NULL, animator.m_CurrentTime, 0.0f, 0.0f);
			if (forwardPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				animBlendState = AnimBlendState::IDLE_TO_WALK;
				activeAnimation = &walkAnimation;
			}
			break;

		case AnimBlendState::IDLE_TO_WALK:
			movementState = MovementState::WALK;
			activeAnimation = &walkAnimation;
			blendAmount += blendRate;
			blendAmount = std::min(blendAmount, 1.0f);
			animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (blendAmount >= 1.0f)
			{
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, 0.0f);
				animBlendState = AnimBlendState::WALK;
			}
			break;

		case AnimBlendState::WALK:
			movementState = MovementState::WALK;
			activeAnimation = &walkAnimation;
			animator.PlayAnimation(&walkAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, 0.0f);
			if (!forwardPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				animBlendState = AnimBlendState::WALK_TO_IDLE;
				activeAnimation = &idleAnimation;
				movementState = MovementState::IDLE;
			}
			else if (runPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&walkAnimation, &runAnimation, animator.m_CurrentTime, 0.0f, blendAmount);
				animBlendState = AnimBlendState::WALK_TO_RUN;
				activeAnimation = &runAnimation;
				movementState = MovementState::RUN;
			}
			break;

		case AnimBlendState::WALK_TO_IDLE:
			movementState = MovementState::IDLE;
			activeAnimation = &idleAnimation;
			blendAmount += blendRate;
			blendAmount = std::min(blendAmount, 1.0f);
			animator.PlayAnimation(&walkAnimation, &idleAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (forwardPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&idleAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				animBlendState = AnimBlendState::IDLE_TO_WALK;
				activeAnimation = &walkAnimation;
				movementState = MovementState::WALK;
			}
			else if (blendAmount >= 1.0f)
			{
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&idleAnimation, NULL, startTime, 0.0f, 0.0f);
				animBlendState = AnimBlendState::IDLE;
			}
			break;

		case AnimBlendState::WALK_TO_RUN:
			movementState = MovementState::RUN;
			activeAnimation = &runAnimation;
			blendAmount += blendRate;
			blendAmount = std::min(blendAmount, 1.0f);
			animator.PlayAnimation(&walkAnimation, &runAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (!forwardPressed || !runPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&runAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				animBlendState = AnimBlendState::RUN_TO_WALK;
				activeAnimation = &walkAnimation;
				movementState = MovementState::WALK;
			}
			else if (blendAmount >= 1.0f)
			{
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				animator.PlayAnimation(&runAnimation, NULL, startTime, 0.0f, 0.0f);
				animBlendState = AnimBlendState::RUN;
			}
			break;

		case AnimBlendState::RUN:
			movementState = MovementState::RUN;
			activeAnimation = &runAnimation;
			animator.PlayAnimation(&runAnimation, NULL, animator.m_CurrentTime, animator.m_CurrentTime2, 0.0f);
			if (!forwardPressed || !runPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&runAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				animBlendState = AnimBlendState::RUN_TO_WALK;
				activeAnimation = &walkAnimation;
				movementState = forwardPressed ? MovementState::WALK : MovementState::IDLE;
			}
			break;

		case AnimBlendState::RUN_TO_WALK:
			movementState = forwardPressed ? MovementState::WALK : MovementState::IDLE;
			activeAnimation = forwardPressed ? &walkAnimation : &idleAnimation;
			blendAmount += blendRate;
			blendAmount = std::min(blendAmount, 1.0f);
			animator.PlayAnimation(&runAnimation, &walkAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
			if (forwardPressed && runPressed)
			{
				blendAmount = 0.0f;
				animator.PlayAnimation(&walkAnimation, &runAnimation, animator.m_CurrentTime, animator.m_CurrentTime2, blendAmount);
				animBlendState = AnimBlendState::WALK_TO_RUN;
				activeAnimation = &runAnimation;
				movementState = MovementState::RUN;
			}
			else if (blendAmount >= 1.0f)
			{
				blendAmount = 0.0f;
				float startTime = animator.m_CurrentTime2;
				if (forwardPressed)
				{
					animator.PlayAnimation(&walkAnimation, NULL, startTime, 0.0f, 0.0f);
					animBlendState = AnimBlendState::WALK;
					activeAnimation = &walkAnimation;
					movementState = MovementState::WALK;
				}
				else
				{
					animator.PlayAnimation(&idleAnimation, NULL, 0.0f, 0.0f, 0.0f);
					animBlendState = AnimBlendState::IDLE;
					activeAnimation = &idleAnimation;
					movementState = MovementState::IDLE;
				}
			}
			break;
		}

		animator.UpdateAnimation(deltaTime);

		Animation* motionAnimation = activeAnimation != nullptr ? activeAnimation : animator.GetCurrentAnimation();
		if (motionAnimation != nullptr)
		{
			float animationTime = animator.GetCurrentTime();
			glm::vec3 rootSample = SampleRootTranslation(motionAnimation, ROOT_BONE_NAME, animationTime);

			if (!rootMotionInitialized)
			{
				previousRootSample = rootSample;
				previousRootTime = animationTime;
				rootMotionInitialized = true;
			}
			else
			{
				glm::vec3 localDelta = rootSample - previousRootSample;

				if (animationTime < previousRootTime)
				{
					glm::vec3 loopDisplacement = glm::vec3(0.0f);
					auto it = rootLoopDisplacements.find(motionAnimation);
					if (it != rootLoopDisplacements.end())
						loopDisplacement = it->second;
					localDelta = (loopDisplacement - previousRootSample) + rootSample;
				}

				previousRootSample = rootSample;
				previousRootTime = animationTime;

				if (movementState != MovementState::IDLE)
				{
					glm::vec3 worldDelta(0.0f);
					if (glm::length(localDelta) < 0.0001f)
					{
						float fallbackSpeed = (movementState == MovementState::RUN) ? runSpeed : walkSpeed;
						worldDelta = facingDir * fallbackSpeed * deltaTime;
					}
					else
					{
						localDelta.y = 0.0f;
						worldDelta = RotateDeltaByYaw(localDelta, characterYaw);
					}
					// Move ground plane in opposite direction instead of character
					groundPosition -= worldDelta;
					// Update ground rotation based on movement direction
					if (glm::length(worldDelta) > 0.0001f)
					{
						groundYaw = glm::degrees(atan2(worldDelta.x, -worldDelta.z));
					}
				}
			}
		}

		updateThirdPersonCamera();
		
		// render
		// ------
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// view/projection transformations
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		groundShader.use();
		groundShader.setMat4("projection", projection);
		groundShader.setMat4("view", view);
		// Apply ground plane transformation (translation and rotation)
		glm::mat4 groundModel = glm::mat4(1.0f);
		groundModel = glm::translate(groundModel, groundPosition);
		groundModel = glm::rotate(groundModel, glm::radians(groundYaw), glm::vec3(0.0f, 1.0f, 0.0f));
		groundShader.setMat4("model", groundModel);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, groundTexture);
		glBindVertexArray(groundVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// don't forget to enable shader before setting uniforms
		ourShader.use();
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
		glm::mat4 skeletonTransform = glm::mat4(1.0f);
		// Character stays at origin with static rotation
		skeletonTransform = glm::translate(skeletonTransform, characterPosition + glm::vec3(0.0f, characterHeightOffset, 0.0f));
		skeletonTransform = glm::rotate(skeletonTransform, glm::radians(-characterYaw - 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		skeletonTransform = glm::scale(skeletonTransform, glm::vec3(0.5f));

		for (int i = 0; i < transforms.size(); ++i)
		{
			glm::mat4 skinnedMatrix = skeletonTransform * transforms[i];
			ourShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", skinnedMatrix);
		}


		// render the loaded model
		glm::mat4 model = glm::mat4(1.0f);
		ourShader.setMat4("model", model);
		ourModel.Draw(ourShader);


		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float yoffset = lastY - ypos; 

	lastX = xpos;
	lastY = ypos;

	// only allow pitch changes
	orbitPitch += yoffset * camera.MouseSensitivity;
	orbitPitch = std::clamp(orbitPitch, -30.0f, 75.0f);

	updateThirdPersonCamera();
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	cameraDistance -= (float)yoffset;
	cameraDistance = std::clamp(cameraDistance, 3.0f, 12.0f);
	updateThirdPersonCamera();
}

unsigned int loadTexture(const char* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RGB;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

void updateThirdPersonCamera()
{
	float clampedPitch = std::clamp(orbitPitch, -30.0f, 75.0f);
	orbitPitch = clampedPitch;

	// Camera target is always at character position (which is static at origin)
	glm::vec3 target = characterPosition + cameraTargetOffset;
	float yawRad = glm::radians(orbitYaw); // Static YAW
	float pitchRad = glm::radians(clampedPitch);

	glm::vec3 offset;
	offset.x = cameraDistance * cos(pitchRad) * cos(yawRad);
	offset.y = cameraDistance * sin(pitchRad);
	offset.z = cameraDistance * cos(pitchRad) * sin(yawRad);

	camera.Position = target + offset;
	camera.Front = glm::normalize(target - camera.Position);
	camera.Right = glm::normalize(glm::cross(camera.Front, camera.WorldUp));
	camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
}

glm::vec3 SampleRootTranslation(Animation* animation, const std::string& rootBoneName, float animationTime)
{
	glm::vec3 translation(0.0f);
	if (!animation)
		return translation;

	Bone* rootBone = animation->FindBone(rootBoneName);
	if (!rootBone)
		return translation;

	float duration = animation->GetDuration();
	if (duration <= 0.0f)
		return translation;

	float wrappedTime = fmod(animationTime, duration);
	if (wrappedTime < 0.0f)
		wrappedTime += duration;

	rootBone->InterpolatePosition(wrappedTime, translation);
	return translation;
}

glm::vec3 EstimateRootLoopDisplacement(Animation* animation, const std::string& rootBoneName)
{
	if (!animation)
		return glm::vec3(0.0f);

	float duration = animation->GetDuration();
	if (duration <= 0.0f)
		return glm::vec3(0.0f);

	glm::vec3 start = SampleRootTranslation(animation, rootBoneName, 0.0f);
	glm::vec3 end = SampleRootTranslation(animation, rootBoneName, duration - 0.0001f);
	return end - start;
}

glm::vec3 RotateDeltaByYaw(const glm::vec3& delta, float yawDegrees)
{
	float yawRad = glm::radians(yawDegrees);
	float cosYaw = cos(yawRad);
	float sinYaw = sin(yawRad);

	glm::vec3 result;
	result.x = delta.x * cosYaw - delta.z * sinYaw;
	result.z = delta.x * sinYaw + delta.z * cosYaw;
	result.y = delta.y;
	return result;
}
