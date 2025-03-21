#include "VKSetup.hpp"
#include "VKRender.hpp"
#include "VKImage.hpp"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h> 
#include <assimp/postprocess.h> 
#include "MeshData.hpp"


struct Vertex { 
    glm::vec3 pos; 
    glm::vec4 color; 
    };

struct SceneData {     
    vector<VulkanMesh> allMeshes; 
    const aiScene *scene = nullptr; 
    };

SceneData sceneData;


class Assign02RenderEngine : public VulkanRenderEngine {
    public:
        // constructor
        Assign02RenderEngine(VulkanInitData &vkInitData) : VulkanRenderEngine(vkInitData) {}

        // destructor
        virtual ~Assign02RenderEngine() {}; 
    
        virtual bool initialize(VulkanInitRenderParams *params) override {
            if (!VulkanRenderEngine::initialize(params)) {
                return false;
            }
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
                for (VulkanMesh &mesh : sceneData->allMeshes) {
                    recordDrawVulkanMesh(commandBuffer, mesh);
                }

                // Stop render pass
                commandBuffer.endRenderPass();

                // End command buffer
                commandBuffer.end();
                }
                                

                        
};

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






    







int main(int argc, char **argv) {
    cout << "BEGIN FORGING!!!" << endl;

    // Set name
    string appName = "Assign02";
    string windowTitle = "Assign02: dematta";
    int windowWidth = 800;
    int windowHeight = 600;

    // Create GLFW window
    GLFWwindow* window = createGLFWWindow(windowTitle, windowWidth, windowHeight);

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
    VulkanRenderEngine *renderEngine = new VulkanRenderEngine(  vkInitData);
    renderEngine->initialize(&params);
    
    // Create very simple quad on host
    Mesh<SimpleVertex> hostMesh = {
        {
            {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
            {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}
        },
        { 0, 2, 1, 2, 0, 3 }
    };
    
    // Create Vulkan mesh
    VulkanMesh mesh = createVulkanMesh(vkInitData, renderEngine->getCommandPool(), hostMesh); 
    vector<VulkanMesh> allMeshes {
        { mesh }
    };

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
        renderEngine->drawFrame(&allMeshes);  

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
    cleanupVulkanMesh(vkInitData, mesh);
    delete renderEngine;
    cleanupVulkanBootstrap(vkInitData);
    cleanupGLFWWindow(window);
    
    cout << "FORGING DONE!!!" << endl;
    return 0;
}