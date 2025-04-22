#include "VKSetup.hpp"
#include "VKRender.hpp"
#include "VKImage.hpp"
#include "VKUniform.hpp"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h> 
#include <assimp/postprocess.h> 
#include "MeshData.hpp"

#include "glm/gtc/matrix_transform.hpp" 
#define GLM_ENABLE_EXPERIMENTAL 
#include "glm/gtx/transform.hpp" 
#include "glm/gtx/string_cast.hpp" 
#include "glm/gtc/type_ptr.hpp" 
#include "VKUtility.hpp" 


struct UPushVertex {
    alignas(16) glm::mat4 modelMat;
};


struct Vertex { 
    glm::vec3 pos; 
    glm::vec4 color; 
    };

struct SceneData {     
    vector<VulkanMesh> allMeshes; 
    const aiScene *scene = nullptr;
    float rotAngle = 0.0f; // hold current local rotation angle in degrees

    // camera
    glm::vec3 eye = glm:: vec3(0.0f, 0.0f, 1.0f); // cam pos
    glm::vec3 lookAt = glm::vec3 (0.0f, 0.0f, 0.0f); // look-at point

    glm::vec2 mousePos; // to hold the last mouse position 

    glm::mat4 viewMat = glm::mat4(1.0f); 
    glm::mat4 projMat = glm::mat4(1.0f); 
    };

SceneData sceneData;

struct UBOVertex {
    alignas(16) glm::mat4 viewMat;
    alignas(16) glm::mat4 projMat;
};

void renderScene(vk::CommandBuffer &commandBuffer,
    SceneData *sceneData,
    aiNode *node,
    glm::mat4 parentMat,
    int level,
    vk::PipelineLayout pipelineLayout);


glm::mat4 makeRotateZ(float rotAngle, glm::vec3 offset) {
    // convert rotAngle to radians
    float radians = glm::radians(rotAngle);

    // translate by negative offset
    glm::mat4 translateNegOffset = glm::translate(glm::mat4(1.0f), -offset);

    // rotate rotAngle around the Z axis 
    glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), radians, glm::vec3(0.0f, 0.0f, 1.0f));

    // translate by offset
    glm::mat4 translateOffset = glm::translate(glm::mat4(1.0f), offset);

    // return the composite transformation
    return translateOffset * rotateZ * translateNegOffset;
}

glm::mat4 makeLocalRotate(glm::vec3 offset, glm::vec3 axis, float angle) {
    float radians = glm::radians(angle); // convert angle to radians

    glm::mat4 translateNegOffset = glm::translate(glm::mat4(1.0f), -offset);
    glm::mat4 rotateAroundAxis = glm::rotate(glm::mat4(1.0f), radians, axis);
    glm::mat4 translateOffset = glm::translate(glm::mat4(1.0f), offset);

    return translateOffset * rotateAroundAxis * translateNegOffset; // return composite transformation

}


class Assign04RenderEngine : public VulkanRenderEngine {
    public:
        // constructor
        Assign04RenderEngine(VulkanInitData &vkInitData) : VulkanRenderEngine(vkInitData) {}

        // destructor
        virtual ~Assign04RenderEngine() {}; 
    
        virtual bool initialize(VulkanInitRenderParams *params) override {
            if (!VulkanRenderEngine::initialize(params)) {
                return false;
            }

            // create a UBO for each frame-in-flight (similar to profexercises08 line 252)
            deviceUBOVert = createVulkanUniformBufferData(
                vkInitData.device,
                vkInitData.physicalDevice,
                sizeof(UBOVertex),
                MAX_FRAMES_IN_FLIGHT
            );

            // create the descriptor pool
            vector<vk::DescriptorPoolSize> poolSizes = {
                vk::DescriptorPoolSize(
                    vk::DescriptorType::eUniformBuffer,
                    1 * MAX_FRAMES_IN_FLIGHT
                )
            };

            vk::DescriptorPoolCreateInfo poolCreateInfo = vk::DescriptorPoolCreateInfo()
                .setPoolSizes(poolSizes)
                .setMaxSets(MAX_FRAMES_IN_FLIGHT);

            // Create the actual descriptorPool...
            descriptorPool = vkInitData.device.createDescriptorPool(poolCreateInfo);




            return true;
        }

        // pulled from VKRender.cpp (line 415 except for cast and mesh loop)
        virtual void recordCommandBuffer(void *userData,
            vk::CommandBuffer &commandBuffer,
            unsigned int frameIndex) override {

                // cast userData as a SceneData pointer
                SceneData *sceneData = static_cast<SceneData*>(userData);
                 
                // Begin commands
                commandBuffer.begin(vk::CommandBufferBeginInfo());

                // Get the extents of the buffers (since we'll use it a few times)
                vk::Extent2D extent = vkInitData.swapchain.extent;

                // Begin render pass
                array<vk::ClearValue, 2> clearValues {};
                clearValues[0].color = vk::ClearColorValue(0.0f, 2.2f, 0.8f, 1.0f);
                clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0.0f);

                commandBuffer.beginRenderPass(vk::RenderPassBeginInfo(
                this->renderPass, 
                this->framebuffers[frameIndex], 
                { {0,0}, extent },
                clearValues),
                vk::SubpassContents::eInline);

                // Bind pipeline
                commandBuffer.bindPipeline(
                vk::PipelineBindPoint::eGraphics, 
                this->pipelineData.graphicsPipeline);

                // Set up viewport and scissors
                vk::Viewport viewports[] = {{0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f}};
                commandBuffer.setViewport(0, viewports);

                vk::Rect2D scissors[] = {{{0,0}, extent}};
                commandBuffer.setScissor(0, scissors);

                // Record our buffer (ONLY drawing first mesh)
                // recordDrawVulkanMesh(commandBuffer, allMeshes->at(0));
                // Loop through and recordDrawVulkanMesh() on each VulkanMesh in sceneData->allMeshes.
                // for (VulkanMesh &mesh : sceneData->allMeshes) {
                //     recordDrawVulkanMesh(commandBuffer, mesh);
                // }
                renderScene(
                    commandBuffer,
                    sceneData,
                    sceneData->scene->mRootNode,
                    glm::mat4(1.0f), // identity matrix
                    0, // level
                    this->pipelineData.pipelineLayout
                );

                // Stop render pass
                commandBuffer.endRenderPass();

                // End command buffer
                commandBuffer.end();
                }
                                
        virtual vector <vk::PushConstantRange> getPushConstantRanges() override {
            // from ProfExercises07 line 314
            return {
                vk::PushConstantRange {
                    vk::ShaderStageFlagBits::eVertex, 0, sizeof(UPushVertex) 
                }
            };
        }
    protected:
    
        UBOVertex hostUBOVert; // HOST vertex shader UBO data
        UBOData deviceUBOVert; // DEVICE vertex shader UBO data
        vk::DescriptorPool descriptorPool; // Memory manager for descriptor sets
        vector<vk::DescriptorSet> descriptorSets; // List of descriptor sets
};


void renderScene(vk::CommandBuffer &commandBuffer,
    SceneData *sceneData,
    aiNode *node,
    glm::mat4 parentMat,
    int level,
    vk::PipelineLayout pipelineLayout) {
        // get the transformation for the current node
        aiMatrix4x4 aiT = node->mTransformation;

        // convert the transformation to a glm::mat4 nodeT
        glm::mat4 nodeT;
        aiMatToGLM4(aiT, nodeT);

        // compute current model matrix
        glm::mat4 modelMat = parentMat * nodeT;

        // get location of current node
        glm::vec4 pos4 = modelMat[3]; // last column of modelMat
        glm::vec3 pos = glm::vec3(pos4); // convert vec4 to a vec3 pos

        // get a proper local Z rotation: R
        glm::mat4 R = makeRotateZ(sceneData-> rotAngle, pos);
        glm::mat4 tmpModel = R * modelMat;
        
        // create an instance of UPushVertex and store tmpModel as the model matrix
        UPushVertex pushData;
        pushData.modelMat = tmpModel;

        // use commandBuffer.pushConstants() to push up the UPushVertex data
        commandBuffer.pushConstants(
            // this->pipelineData... doesn't work for me, used similar structure to ProfExercises07 line 459
            pipelineLayout,
            vk::ShaderStageFlagBits::eVertex,
            0, sizeof(UPushVertex), &pushData
        );

        // draw each mesh in the current node
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            int index = node->mMeshes[i];
            VulkanMesh &mesh = sceneData->allMeshes.at(index);
            recordDrawVulkanMesh(commandBuffer, mesh);
        }

        // call renderScene() on each child of the NODE
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            renderScene(
                commandBuffer,
                sceneData,
                node->mChildren[i],
                modelMat, // parent matrix
                level + 1,
                pipelineLayout
            );
        }
    }


void extractMeshData(aiMesh *mesh, Mesh<Vertex> &m) {
    // Clear out the Mesh's vertices and indices.
    m.vertices.clear();
    m.indices.clear();

    // Loop through all vertices in the aiMesh (mesh->mNumVertices)
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i){
        Vertex v;

        /* Grab the vertex position information from mesh->mVertices[i] 
        and store it in the Vertex's position. */
        aiVector3D aiPos = mesh->mVertices[i];
        // somehow convert to glm::vec3 structs
        v.pos = glm::vec3(aiPos.x, aiPos.y, aiPos.z);
        // Set the color of the Vertex (non-black and alpha = 1.0)
        v.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        
        // Add the Vertex to the Mesh's vertices list
        m.vertices.push_back(v);

    }
    


    // Loop through all faces in the aiMesh (mesh->mNumFaces)
    for (unsigned int i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];


        // Loop through the number of indices for this face (face.mNumIndices)
        for (unsigned int k = 0; k < face.mNumIndices; k++) {
            m.indices.push_back(face.mIndices[k]); 
        }
    }
        
} 

static void key_callback (GLFWwindow *window,
                            int key, int scancode,
                            int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window,true);
        } else if (key == GLFW_KEY_J) {
            sceneData.rotAngle += 1.0f;
        } else if (key == GLFW_KEY_K) {
            sceneData.rotAngle -= 1.0f;
        }
    }
}

static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
    glm::vec2 newMousePos = glm::vec2 (xpos, ypos);
    glm::vec2 relMouse = newMousePos - sceneData.mousePos;

    // Use glfwGetFramebufferSize() to acquire the current framebuffer size
    // (similar to ProfExercises08 mouse_motion_callback)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    // Divide relMouse.x by current framebuffer width and relMouse.y by current framebuffer...
    if (fbWidth > 0 && fbHeight > 0) {
        relMouse.x /= static_cast<float>(fbWidth);
        relMouse.y /= static_cast<float>(fbHeight);
    }

    // relative x motion
    float angleX = 30.0f * relMouse.x;
    glm::mat4 rotateYMat = makeLocalRotate(
        sceneData.eye,
        glm::vec3(0, 1, 0),
        angleX
    );

    glm::vec4 lookAtV = glm::vec4(sceneData.lookAt, 1.0f);
    lookAtV = rotateYMat * lookAtV;
    sceneData.lookAt = glm::vec3(lookAtV);

    // relative y
    glm::vec3 camDir = sceneData.lookAt - sceneData.eye;
    glm::vec3 localX = glm::normalize(glm::cross(camDir, glm::vec3(0, 1, 0)));

    float angleY = 30.0f * relMouse.y;
    glm::mat4 rotateXMat = makeLocalRotate(
        sceneData.eye,
        localX,
        angleY
    );

    lookAtV = glm::vec4(sceneData.lookAt, 1.0f);
    lookAtV = rotateXMat * lookAtV;
    sceneData.lookAt = glm::vec3 (lookAtV);


    // stored mouse position
    sceneData.mousePos = newMousePos;
}


    int main(int argc, char **argv) {
    cout << "BEGIN FORGING!!!" << endl;

    // default model path
    string modelPath = "sampleModels/sphere.obj";

    // If argc >= 2, grab argv[1] as the model path to load and convert to a string.
    if (argc >= 2) {
        modelPath = string(argv[1]);
    }

    // needed to load models, works with ReadFile
    Assimp::Importer importer;

    // load options
    sceneData.scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs | 
        aiProcess_GenNormals |          
        aiProcess_JoinIdenticalVertices 
    );

    // Check to make sure the model loaded correctly
    if (!sceneData.scene || 
        (sceneData.scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || 
        !sceneData.scene->mRootNode) 
    {
        return -1;  // Exit with error
    }
    // maybe cerr here if things go wrong?

    


    // Set name
    string appName = "Assign04";
    string windowTitle = "Assign04: dematta";
    int windowWidth = 800;
    int windowHeight = 600;

    // Create GLFW window
    GLFWwindow* window = createGLFWWindow(windowTitle, windowWidth, windowHeight);
    glfwSetKeyCallback(window, key_callback);

    // mouse
    glfwSetCursorPosCallback(window, mouse_position_callback);


    // Setup up Vulkan via vk-bootstrap
    VulkanInitData vkInitData;
    initVulkanBootstrap(appName, window, vkInitData);

    

    // Setup basic forward rendering process
    string vertSPVFilename = "build/compiledshaders/" + appName + "/shader.vert.spv";                                                    
    string fragSPVFilename = "build/compiledshaders/" + appName + "/shader.frag.spv";
    
    // Create render engine
    VulkanInitRenderParams params = {
        vertSPVFilename, fragSPVFilename
    };
    // Make sure your VulkanRenderEngine is an instance of Assign02RenderEngine
    VulkanRenderEngine *renderEngine = new Assign04RenderEngine(vkInitData);
    // VulkanRenderEngine *renderEngine = new VulkanRenderEngine(  vkInitData);
    renderEngine->initialize(&params);
    
    // Create very simple quad on host
    // Mesh<SimpleVertex> hostMesh = {
    //     {
    //         {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    //         {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    //         {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    //         {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}
    //     },
    //     { 0, 2, 1, 2, 0, 3 }
    // };
    
    for (unsigned int i = 0; i < sceneData.scene->mNumMeshes; ++i) {
        Mesh<Vertex> meshData; // Create a Mesh<Vertex> object inside the loop 
        
        // Call extractMeshData to get a Mesh from each sceneData.scene->mMeshes[i]
        extractMeshData(sceneData.scene->mMeshes[i], meshData);

        // convert to VulkanMesh
        VulkanMesh vulkanMesh = createVulkanMesh(vkInitData, renderEngine->getCommandPool(), meshData);

        // Add the VulkanMesh to your vector of VulkanMesh's in your sceneData
        sceneData.allMeshes.push_back(vulkanMesh);

    }

    // Create Vulkan mesh
    // VulkanMesh mesh = createVulkanMesh(vkInitData, renderEngine->getCommandPool(), hostMesh); 
    // vector<VulkanMesh> allMeshes {
    //     { mesh }
    // };

    float timeElapsed = 1.0f;
    int framesRendered = 0;
    auto startCountTime = getTime();
    float fpsCalcWindow = 5.0f;
                                       
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Get start time        
        auto startTime = getTime();

        // Poll events for window
        glfwPollEvents();  

        // Draw frame
        renderEngine->drawFrame(&sceneData);   

        // Increment frame count
        framesRendered++;

        // Get end and elapsed
        auto endTime = getTime();
        float frameTime = getElapsedSeconds(startTime, endTime);
        
        float timeSoFar = getElapsedSeconds(startCountTime, getTime());

        if(timeSoFar >= fpsCalcWindow) {
            float fps = framesRendered / timeSoFar;
            cout << "FPS: " << fps << endl;

            startCountTime = getTime();
            framesRendered = 0;
        }        
    }
        
    // Make sure all queues on GPU are done
    vkInitData.device.waitIdle();
    
    // Cleanup  
    // cleanupVulkanMesh(vkInitData, mesh);
    for (VulkanMesh &mesh : sceneData.allMeshes) {
        cleanupVulkanMesh(vkInitData, mesh);
    }
    // Clear out your list of VulkanMesh objects in your sceneData
    sceneData.allMeshes.clear();
    delete renderEngine;
    cleanupVulkanBootstrap(vkInitData);
    cleanupGLFWWindow(window);
    
    cout << "FORGING DONE!!!" << endl;
    return 0;
}