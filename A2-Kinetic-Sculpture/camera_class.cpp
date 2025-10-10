#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <vector>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

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

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("7.4.camera.vs", "7.4.camera.fs");
    ////////////////////////////
    // wave model (plane with subdivided grid)
    ////////////////////////////

    // Wave animation parameters
    const int GRID_SIZE = 64;
    const int VERTEX_COUNT = GRID_SIZE * GRID_SIZE;
    const int TRIANGLE_COUNT = (GRID_SIZE - 1) * (GRID_SIZE - 1) * 6;
    
    // Generate subdivided plane vertices
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Generate vertices
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = ((float)j / (GRID_SIZE - 1)) * 2.0f - 1.0f; // -1 to 1
            float z = ((float)i / (GRID_SIZE - 1)) * 2.0f - 1.0f;
            float y = 0.0f;
            
            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Texture coordinates
            vertices.push_back((float)j / (GRID_SIZE - 1)); // 0 to 1
            vertices.push_back((float)i / (GRID_SIZE - 1));
        }
    }
    
    // Generate edge for triangles
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        for (int j = 0; j < GRID_SIZE - 1; j++) {
            int topLeft = i * GRID_SIZE + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * GRID_SIZE + j;
            int bottomRight = bottomLeft + 1;
            
            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // load and create water texture 
    // -----------------------------
    unsigned int waterTexture;
    glGenTextures(1, &waterTexture);
    glBindTexture(GL_TEXTURE_2D, waterTexture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load(FileSystem::getPath("resources/textures/wave.jpg").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load water texture" << std::endl;
    }
    stbi_image_free(data);

    ////////////////////////////
    // Boat Model
    ////////////////////////////
    
    // Boat model (hull with sharp ends and a sail)
    // Vertices: positions (x,y,z), texture coordinates (u,v)
    float boxVertices[] = {
        // Hull vertices (sharp at front Z+, sharp at back Z-)
        // Bottom hull
         0.0f, -0.5f,  1.0f,   0.5f, 0.0f, // 0 - front tip (bottom)
         0.0f, -0.5f,  1.0f,   0.0f, 0.2f, // 1 - front-left bottom
         0.0f, -0.5f,  1.0f,   1.0f, 0.2f, // 2 - front-right bottom
         0.0f, -0.5f, -1.0f,   0.0f, 0.8f, // 3 - back-left bottom
         0.0f, -0.5f, -1.0f,   1.0f, 0.8f, // 4 - back-right bottom
         0.0f, -0.5f, -1.0f,   0.5f, 1.0f, // 5 - back tip (bottom)

        // Upper hull rim
         0.0f,  0.0f,  1.5f,   0.5f, 0.0f, // 6 - front tip (top rim)
        -0.5f,  0.0f,  0.5f,   0.0f, 0.2f, // 7 - front-left top rim
         0.5f,  0.0f,  0.5f,   1.0f, 0.2f, // 8 - front-right top rim
        -0.5f,  0.0f, -0.5f,   0.0f, 0.8f, // 9 - back-left top rim
         0.5f,  0.0f, -0.5f,   1.0f, 0.8f, // 10 - back-right top rim
         0.0f,  0.0f, -1.5f,   0.5f, 1.0f, // 11 - back tip (top rim)

        // Sail vertices (simple vertical triangle above hull)
         0.0f,  0.0f,  -0.6f,   0.5f, 0.5f, // 12 - sail base
         0.0f,  0.9f,  0.1f,   0.5f, 0.1f, // 13 - sail top
         0.0f,  0.0f,  0.6f,   0.6f, 0.25f // 14 - sail front
    };

    // Boat indices - properly ordered triangles for complete hull
    unsigned int boxIndices[] = {
        // Bottom of hull (complete bottom surface)
        0, 1, 2,    // Front triangle
        1, 3, 2,    // Left side bottom
        2, 3, 4,    // Right side bottom  
        3, 5, 4,    // Back triangle

        // Left side of hull (bottom to top)
        0, 6, 1,    // Front-left
        1, 6, 7,    // Front-left
        1, 7, 3,    // Left side
        3, 7, 9,    // Left side
        3, 9, 5,    // Back-left
        5, 9, 11,   // Back-left

        // Right side of hull (bottom to top)
        0, 2, 6,    // Front-right
        2, 8, 6,    // Front-right
        2, 4, 8,    // Right side
        4, 10, 8,   // Right side
        4, 5, 10,   // Back-right
        5, 11, 10,  // Back-right

        // Top rim (deck surface)
        6, 7, 8,    // Front deck
        7, 9, 8,    // Left deck
        8, 9, 10,   // Center deck
        9, 11, 10,  // Right deck

        // Sail (simple triangle)
        12, 13, 14
    };
    
    // Box VAO, VBO, EBO
    unsigned int boxVAO, boxVBO, boxEBO;
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxEBO);
    
    glBindVertexArray(boxVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boxIndices), boxIndices, GL_STATIC_DRAW);
    
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Load box texture
    unsigned int boxTexture;
    glGenTextures(1, &boxTexture);
    glBindTexture(GL_TEXTURE_2D, boxTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Load block.png texture for the box
    int boxWidth, boxHeight, boxNrChannels;
    unsigned char *boxData = stbi_load(FileSystem::getPath("resources/textures/container2.png").c_str(), &boxWidth, &boxHeight, &boxNrChannels, 0);
    if (boxData)
    {
        GLenum format;
        if (boxNrChannels == 1)
            format = GL_RED;
        else if (boxNrChannels == 3)
            format = GL_RGB;
        else if (boxNrChannels == 4)
            format = GL_RGBA;
            
        glTexImage2D(GL_TEXTURE_2D, 0, format, boxWidth, boxHeight, 0, format, GL_UNSIGNED_BYTE, boxData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load block texture" << std::endl;
        // Fallback to brown solid color if texture fails to load
        unsigned char brownData[] = {
            230, 230, 230, 255,   
            230, 230, 230, 255,   
            230, 230, 230, 255, 
            230, 230, 230, 255,
        };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, brownData);
    }
    stbi_image_free(boxData);
 

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use();
    ourShader.setInt("waterTexture", 0);
    ourShader.setInt("boxTexture", 0);


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

        // render
        glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        // get time parameter for animate
        float time = static_cast<float>(glfwGetTime());
        
        ////////////////////////////
        // wave animation
        ////////////////////////////
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                int vertexIndex = (i * GRID_SIZE + j) * 5; // 5 floats per vertex (x,y,z,u,v)
                
                float x = vertices[vertexIndex];
                float z = vertices[vertexIndex + 2];
                
                // Water Movement!!!!
                float wave1 = 0.1f * sin(x * 2.0f + time * 1.5f) * cos(z * 1.5f + time * 1.2f);
                float wave2 = 0.2f * cos(x * 3.0f + time * 2.0f) * sin(z * 2.0f + time * 1.8f);
                float wave3 = 0.15f * sin(x * 4.0f + time * 2.5f) * cos(z * 3.0f + time * 2.2f);
                float wave4 = 0.1f * sin(x * 5.0f + time * 3.0f) * sin(z * 4.0f + time * 2.8f);
                
                // Combine waves
                float y = wave1 + wave2 + wave3 + wave4;
                vertices[vertexIndex + 1] = y; // Update Y position
            }
        }
        
        // Update vertex buffer with new positions
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        // bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waterTexture);

        // activate shader
        ourShader.use();

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        // pass time uniform for additional shader effects
        ourShader.setFloat("time", time);
        ourShader.setVec3("viewPos", camera.Position);

        // render the animated plane
        glBindVertexArray(VAO);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(5.0f, 1.0f, 5.0f)); // Scale up the plane
        ourShader.setMat4("model", model);
        ourShader.setBool("isBox", false); // This is water, not box

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        ////////////////////////////
        // Box rendering with floating animation
        ////////////////////////////
        
        // Bind box texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, boxTexture);
        
        // Bind box VAO
        glBindVertexArray(boxVAO);
        
        // Calculate floating height at center of grid by matching the water wave equations at (x=0, z=0)
        float x = 0.0f;
        float z = 0.0f;
        float wave1 = 0.1f * sin(x * 2.0f + time * 1.5f) * cos(z * 1.5f + time * 1.2f);
        float wave2 = 0.2f * cos(x * 3.0f + time * 2.0f) * sin(z * 2.0f + time * 1.8f);
        float wave3 = 0.15f * sin(x * 4.0f + time * 2.5f) * cos(z * 3.0f + time * 2.2f);
        float wave4 = 0.1f * sin(x * 5.0f + time * 3.0f) * sin(z * 4.0f + time * 2.8f);
        float floatHeight = 0.175 + (wave1 + wave2 + wave3 + wave4)*0.7f;
        
        // Position box at center of wave plane with floating animation
        glm::mat4 boxModel = glm::mat4(1.0f);
        boxModel = glm::translate(boxModel, glm::vec3(0.0f, floatHeight, 0.0f)); 
        boxModel = glm::rotate(boxModel, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        boxModel = glm::scale(boxModel, glm::vec3(0.5f, 0.5f, 0.5f)); 
        
        ourShader.setMat4("model", boxModel);
        ourShader.setBool("isBox", true); // bool that check if object was a box (for shader jing)
        
        // Draw the boat (now has 25 triangles = 75 indices)
        glDrawElements(GL_TRIANGLES, 75, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxEBO);

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

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    //if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    //    camera.ProcessKeyboard(UP, deltaTime);
    //if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    //    camera.ProcessKeyboard(DOWN, deltaTime);
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
