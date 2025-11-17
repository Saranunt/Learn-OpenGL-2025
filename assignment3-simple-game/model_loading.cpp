#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <stb_image.h>

#include <iostream>
#include <cmath>
#include <vector>
#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// plane state
glm::vec3 planePosition(100.0f, 26.0f, 0.0f);
float planePitch = 0.0f;  // rotation around X axis (nose up/down)
float planeYaw = 0.0f;    // rotation around Y axis (left/right)
float planeRoll = 0.0f;   // rotation around Z axis (banking)
float planeSpeed = 5.0f;  // units per second

// islands
std::vector<glm::vec3> islandPositions;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

    // tell GLFW to not capture our mouse (third-person view doesn't need mouse control)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");
    

    // load models
    // -----------
    Model ourModel(FileSystem::getPath("resources/objects/plane/plane.dae"));
    //stbi_set_flip_vertically_on_load(true);
    Model islandModel(FileSystem::getPath("resources/objects/island4/Untitled.dae"));
    
    // Randomly place extra islands (2 or 3) in the world
    islandPositions.clear();
    islandPositions.push_back(glm::vec3(0.0f, 26.0f, 0.0f)); // original island
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> islandCountDist(2, 3);
    std::uniform_real_distribution<float> horizontalDist(-1500.0f, 1500.0f);
    std::uniform_real_distribution<float> heightDist(26.0f, 26.0f);
    
    int extraIslands = islandCountDist(gen);
    islandPositions.reserve(1 + extraIslands);
    for (int i = 0; i < extraIslands; ++i)
    {
        islandPositions.emplace_back(horizontalDist(gen), heightDist(gen), horizontalDist(gen));
    }
    
    // load and create texture for ground
    // ----------------------------------
    unsigned int groundTexture = loadTexture(FileSystem::getPath("resources/textures/wave.png").c_str());
    
    // Create static ground plane
    // --------------------------
    // Texture coordinates are set to repeat multiple times for tiling effect
    float tileRepeat = 50.0f; // Repeat texture 50 times across the ground
    float groundVertices[] = {
        // positions          // normals           // texture coords
         1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  tileRepeat,  tileRepeat,
        -1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  0.0f,        tileRepeat,
        -1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  0.0f,        0.0f,
        
         1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  tileRepeat,  tileRepeat,
        -1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  0.0f,        0.0f,
         1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  tileRepeat,  0.0f
    };
    
    unsigned int groundVAO, groundVBO;
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // texture coord attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        
        // Update yaw based on roll (banking causes turning)
        // -------------------------------------------------
        float turnRate = 0.5f; // degrees per second per degree of roll
        planeYaw -= planeRoll * turnRate * deltaTime;
        
        // Normalize yaw to 0-360 degrees
        while (planeYaw < 0.0f) planeYaw += 360.0f;
        while (planeYaw >= 360.0f) planeYaw -= 360.0f;
        
        // Update plane position based on orientation
        // ------------------------------------------
        float yawRad = glm::radians(planeYaw);
        float pitchRad = glm::radians(planePitch);
        
        // Calculate forward direction based on yaw and pitch
        // Forward direction in the plane's local space
        glm::vec3 forward(
            std::sin(yawRad) * std::cos(pitchRad),
            -std::sin(pitchRad),  // Negative because pitch up should move up
            std::cos(yawRad) * std::cos(pitchRad)
        );
        forward = glm::normalize(forward);
        
        // Move plane forward
        planePosition += forward * planeSpeed * deltaTime;

        // Update third-person camera
        // ---------------------------
        // Camera follows behind and above the plane
        float cameraDistance = 8.0f;
        float cameraHeight = 3.0f;
        
        // Calculate camera position behind the plane
        glm::vec3 cameraOffset(
            -std::sin(yawRad) * cameraDistance,
            cameraHeight,
            -std::cos(yawRad) * cameraDistance
        );
        
        glm::vec3 cameraPos = planePosition + cameraOffset;
        
        // Look at the plane
        glm::vec3 cameraTarget = planePosition;
        camera.Position = cameraPos;
        camera.Front = glm::normalize(cameraTarget - cameraPos);
        camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);

        // render
        // ------
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f); // Sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        // Render static ground plane
        // --------------------------
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        
        // Ground stays at fixed position (identity matrix)
        glm::mat4 groundModel = glm::mat4(1.0f);
        ourShader.setMat4("model", groundModel);
        
        // bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        ourShader.setInt("texture_diffuse1", 0);
        
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        // Unbind wave texture so island model doesn't use it
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Render island model
        // -------------------
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        
        // Island transforms: draw the main island and the randomly placed ones
        for (const auto& islandPos : islandPositions)
        {
            glm::mat4 islandModelMatrix = glm::mat4(1.0f);
            islandModelMatrix = glm::translate(islandModelMatrix, islandPos);
            islandModelMatrix = glm::rotate(islandModelMatrix, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
            islandModelMatrix = glm::scale(islandModelMatrix, glm::vec3(500.0f, 500.0f, 500.0f));
            ourShader.setMat4("model", islandModelMatrix);
            islandModel.Draw(ourShader);
        }

        // Render plane model
        // ------------------
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        
        // Build transformation matrix for plane
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, planePosition);
        
        // Apply rotations: yaw, pitch, roll (in that order)
        model = glm::rotate(model, glm::radians(planeYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(planePitch), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(planeRoll), glm::vec3(0.0f, 0.0f, 1.0f));
        
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteTextures(1, &groundTexture);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Plane rotation controls
    float rotationSpeed = 10.0f; // degrees per second
    float acceleration = 10.0f;  // units per second^2 for speed control
    
    // W/S: Pitch (nose up/down)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        planePitch += rotationSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        planePitch -= rotationSpeed * deltaTime;
    
    // A/D: Roll (banking left/right)
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        planeRoll += rotationSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        planeRoll -= rotationSpeed * deltaTime;
    
    // Clamp pitch to reasonable limits
    if (planePitch > 89.0f) planePitch = 89.0f;
    if (planePitch < -89.0f) planePitch = -89.0f;
    
    // Normalize yaw to 0-360 degrees
    while (planeYaw < 0.0f) planeYaw += 360.0f;
    while (planeYaw >= 360.0f) planeYaw -= 360.0f;
    
    // Clamp roll to reasonable limits
    if (planeRoll > 45.0f) planeRoll = 45.0f;
    if (planeRoll < -45.0f) planeRoll = -45.0f;

    // Z / X: adjust speed (forward velocity)
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        planeSpeed += acceleration * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        planeSpeed -= acceleration * deltaTime;
    if (planeSpeed < 1.0f) planeSpeed = 1.0f;
    if (planeSpeed > 50.0f) planeSpeed = 50.0f;
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
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
