#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <string>
#include <fstream>

#include <vulkan/vulkan.hpp>

constexpr int kScreenWidth{ 800 };
constexpr int kScreenHeight{ 800 };

SDL_Window* gWindow{ nullptr };

VkInstance gVkInstance;
VkPhysicalDevice gVkPhysicalDevice;
VkDevice gVkDevice;
VkSurfaceKHR gSurface;
VkQueue gQueue;
VkSwapchainKHR gSwapchain;
uint32_t gSwapchainImageCount;
VkImage* gSwapchainImages;
uint32_t gCurrentImageIndex;
std::vector<VkImageView> gSwapchainImageViews;
std::vector<VkFramebuffer> gSwapchainFramebuffers;
VkPipeline gPipeline;
VkRenderPass gRenderPass;
VkPipelineLayout gPipelineLayout;
VkCommandPool gCommandPool;
VkCommandBuffer gCommandBuffer;
VkSemaphore gImageAvailableSemaphore;
VkSemaphore gRenderFinishedSemaphore;
VkFence gImageAvailableFence;

Uint64 previous_time {0};
double delta {0.0};


#define print SDL_Log


static std::vector<char> read_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        print("Could not open file '%s'!", file_path);
    }

    size_t file_size = (size_t) file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}

bool create_vulkan_instance(VkInstance &r_instance) {
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion = VK_API_VERSION_1_3;

    uint32_t sdl_extension_count;
    const char* const* sdl_extension_names = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    const char* enabled_layers[1] = {"VK_LAYER_KHRONOS_validation"};

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledExtensionCount = sdl_extension_count;
    create_info.ppEnabledExtensionNames = sdl_extension_names;
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = enabled_layers;

    return vkCreateInstance(&create_info, nullptr, &r_instance) == VK_SUCCESS;
};

bool create_physical_device(VkInstance p_instance, VkPhysicalDevice &r_physical_device) {
    uint32_t physical_device_count;
    vkEnumeratePhysicalDevices(p_instance, &physical_device_count, nullptr);
    if (physical_device_count == 0) {
        print("Could not find a physical rendering device!");
        return false;
    }

    VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
    VkResult result = vkEnumeratePhysicalDevices(p_instance, &physical_device_count, physical_devices);

    // TODO: Choose the best graphics card by querying support for required properties.
    r_physical_device = physical_devices[0];

    return result == VK_SUCCESS;
}

bool create_device(VkPhysicalDevice p_physical_device, VkSurfaceKHR p_surface, VkDevice &r_device, VkQueue &r_queue) {
    uint32_t queue_family_count {0};
    vkGetPhysicalDeviceQueueFamilyProperties(p_physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(p_physical_device, &queue_family_count, queue_families.data());
    int queue_index {0};
    
    for (const VkQueueFamilyProperties &queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 presentation_support { false };
            vkGetPhysicalDeviceSurfaceSupportKHR(p_physical_device, queue_index, p_surface, &presentation_support);
            if (presentation_support) {
                break;
            }
        }
        queue_index++;
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &priority;

    const char* enabled_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = enabled_extensions;

    if (vkCreateDevice(p_physical_device, &create_info, nullptr, &r_device) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(r_device, queue_index, 0, &r_queue);
    return true;
}

bool create_swapchain(VkPhysicalDevice p_physical_device, VkDevice p_device, VkSurfaceKHR p_surface, VkSwapchainKHR &r_swapchain) {
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = p_surface;
    create_info.minImageCount = 3;
    create_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent = VkExtent2D {kScreenWidth, kScreenHeight};
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.clipped = VK_TRUE;

    return vkCreateSwapchainKHR(p_device, &create_info, nullptr, &r_swapchain) == VK_SUCCESS;
}

bool create_image_views() {
    vkGetSwapchainImagesKHR(gVkDevice, gSwapchain, &gSwapchainImageCount, nullptr);
    gSwapchainImages = new VkImage[gSwapchainImageCount];
    vkGetSwapchainImagesKHR(gVkDevice, gSwapchain, &gSwapchainImageCount, gSwapchainImages);
    gSwapchainImageViews.resize(gSwapchainImageCount);

    for (size_t i = i; i < gSwapchainImageCount; i++) {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = gSwapchainImages[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = VK_FORMAT_B8G8R8A8_SRGB;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(gVkDevice, &create_info, nullptr, &gSwapchainImageViews[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool create_render_pass() {
    VkAttachmentDescription color_attachement = {};
    color_attachement.format = VK_FORMAT_B8G8R8A8_SRGB;
    color_attachement.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachement.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachement.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachement_reference = {};
    color_attachement_reference.attachment = 0;
    color_attachement_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachement_reference;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachement;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    return vkCreateRenderPass(gVkDevice, &render_pass_info, nullptr, &gRenderPass) == VK_SUCCESS;
}

bool create_shader_module(const std::vector<char>& code, VkShaderModule &r_shader_module) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    return vkCreateShaderModule(gVkDevice, &create_info, nullptr, &r_shader_module) == VK_SUCCESS;
}

bool create_pipeline() {
    std::vector<char> vert_shader_code = read_file("../../shaders/bin/triangle_vert.spv");
    std::vector<char> frag_shader_code = read_file("../../shaders/bin/triangle_frag.spv");

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    if (!create_shader_module(vert_shader_code, vert_shader_module) || 
        !create_shader_module(frag_shader_code, frag_shader_module)) {
        print("Could not create shader modules!");
        return false;
    }

    VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;
    vertex_input_info.pVertexBindingDescriptions = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float) kScreenWidth;
    viewport.height = (float) kScreenHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width  = (float) kScreenWidth;
    scissor.extent.height = (float) kScreenHeight;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;

    if (vkCreatePipelineLayout(gVkDevice, &pipeline_layout_info, nullptr, &gPipelineLayout) != VK_SUCCESS) {
        print("Could not create pipeline layout!");
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = gPipelineLayout;
    pipeline_info.renderPass = gRenderPass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    if (!vkCreateGraphicsPipelines(gVkDevice, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &gPipeline) == VK_SUCCESS) {
        print("Could not create graphics pipeline!");
        return false;
    }

    vkDestroyShaderModule(gVkDevice, vert_shader_module, nullptr);
    vkDestroyShaderModule(gVkDevice, frag_shader_module, nullptr);

    return true;
}

bool create_framebuffers() {
    gSwapchainFramebuffers.resize(gSwapchainImageViews.size());

    for (size_t i = 0; i < gSwapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
            gSwapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = gRenderPass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = kScreenWidth;
        framebuffer_info.height = kScreenHeight;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(gVkDevice, &framebuffer_info, nullptr, &gSwapchainFramebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool create_command_pool() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = 0;

    return vkCreateCommandPool(gVkDevice, &pool_info, nullptr, &gCommandPool) == VK_SUCCESS;
}

bool create_command_buffer() {
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = gCommandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    return vkAllocateCommandBuffers(gVkDevice, &alloc_info, &gCommandBuffer) == VK_SUCCESS;
}

void record_command_buffer(VkCommandBuffer p_command_buffer, uint32_t p_image_index) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(p_command_buffer, &begin_info) != VK_SUCCESS) {
        print("Could not begin command buffer!");
    }

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = gRenderPass;
    render_pass_info.framebuffer = gSwapchainFramebuffers[p_image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent.width = kScreenWidth;
    render_pass_info.renderArea.extent.height = kScreenHeight;

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(p_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(p_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gPipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(kScreenWidth);
    viewport.height = static_cast<float>(kScreenHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(p_command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {kScreenWidth, kScreenHeight};
    vkCmdSetScissor(p_command_buffer, 0, 1, &scissor);

    vkCmdDraw(p_command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(p_command_buffer);
    
    if (vkEndCommandBuffer(p_command_buffer) != VK_SUCCESS) {
        print("Could not end command buffer!");
    }
}

bool create_sync_objects() {
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    int result;
    result  = (int)vkCreateSemaphore(gVkDevice, &semaphore_info, nullptr, &gImageAvailableSemaphore);
    result += (int)vkCreateSemaphore(gVkDevice, &semaphore_info, nullptr, &gRenderFinishedSemaphore);
    result += (int)vkCreateFence(gVkDevice, &fence_info, nullptr, &gImageAvailableFence);

    return result == 0;
}

void cleanup_swapchain() {
    vkDeviceWaitIdle(gVkDevice);
    for (VkFramebuffer framebuffer : gSwapchainFramebuffers) {
        vkDestroyFramebuffer(gVkDevice, framebuffer, nullptr);
    }
    for (VkImageView image_view : gSwapchainImageViews) {
        vkDestroyImageView(gVkDevice, image_view, nullptr);
    }
    vkDestroySwapchainKHR(gVkDevice, gSwapchain, nullptr);
}

void recreate_swapchain() {
    vkDeviceWaitIdle(gVkDevice);

    cleanup_swapchain();

    create_swapchain(gVkPhysicalDevice, gVkDevice, gSurface, gSwapchain);
    create_image_views();
    create_framebuffers();
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    // Initialize app
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Load Vulkan driver
    if(!SDL_Vulkan_LoadLibrary(nullptr)) {
        SDL_Log("Could not load Vulkan library! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create window
    SDL_PropertiesID window_props {SDL_CreateProperties()};
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, kScreenWidth);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, kScreenHeight);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Prosper");
    gWindow = SDL_CreateWindowWithProperties(window_props);
    if (gWindow == nullptr) {
        SDL_Log("Window could not be created! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Initialize Vulkan
    if (!create_vulkan_instance(gVkInstance)) {
        print("Could not create Vulkan instance!");
        return SDL_APP_FAILURE;
    }
    if (!SDL_Vulkan_CreateSurface(gWindow, gVkInstance, nullptr, &gSurface)) {
        print("Could not create Vulkan surface! SDL error: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!create_physical_device(gVkInstance, gVkPhysicalDevice)) {
        print("Could not create physical device!");
        return SDL_APP_FAILURE;
    }
    if (!create_device(gVkPhysicalDevice, gSurface, gVkDevice, gQueue)) {
        print("Could not create logical device!");
        return SDL_APP_FAILURE;
    }
    if(!create_swapchain(gVkPhysicalDevice, gVkDevice, gSurface, gSwapchain)) {
        print("Could not create swapchain!");
        return SDL_APP_FAILURE;
    }
    if (!create_image_views()) {
        print("Could not create image views!");
        return SDL_APP_FAILURE;
    }
    if (!create_render_pass()) {
        print("Could not create render pass!");
        return SDL_APP_FAILURE;
    }
    if (!create_pipeline()) {
        print("Could not create pipeline!");
        return SDL_APP_FAILURE;
    }
    if (!create_framebuffers()) {
        print("Could not create framebuffers!");
        return SDL_APP_FAILURE;
    }
    if (!create_command_pool()) {
        print("Could not create command pool!");
        return SDL_APP_FAILURE;
    }
    if (!create_command_buffer()) {
        print("Could not create command buffer!");
        return SDL_APP_FAILURE;
    }
    if (!create_sync_objects()) {
        print("Could not create fence and semaphores!");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void draw() {
    vkWaitForFences(gVkDevice, 1, &gImageAvailableFence, VK_TRUE, UINT64_MAX);
    vkResetFences(gVkDevice, 1, &gImageAvailableFence);

    uint32_t image_index;
    VkResult acquisition_result = vkAcquireNextImageKHR(gVkDevice, gSwapchain, UINT64_MAX, gImageAvailableSemaphore, VK_NULL_HANDLE, &image_index);
    if (acquisition_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    }

    vkResetCommandBuffer(gCommandBuffer, 0);
    record_command_buffer(gCommandBuffer, image_index);     

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &gImageAvailableSemaphore;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &gRenderFinishedSemaphore;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &gCommandBuffer;

    vkQueueSubmit(gQueue, 1, &submit_info, gImageAvailableFence);

    VkSwapchainKHR swap_chains[] = {gSwapchain};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &gRenderFinishedSemaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    VkResult presentation_result = vkQueuePresentKHR(gQueue, &present_info);
    if (presentation_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
    }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    // Delta
    Uint64 current_time = SDL_GetTicksNS();
    delta = double(current_time - previous_time) * 0.000000001;
    previous_time = current_time;

    draw();
    
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appsatte, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    cleanup_swapchain();

    vkDestroySemaphore(gVkDevice, gImageAvailableSemaphore, nullptr);
    vkDestroySemaphore(gVkDevice, gRenderFinishedSemaphore, nullptr);
    vkDestroyFence(gVkDevice, gImageAvailableFence, nullptr);
    vkDestroyCommandPool(gVkDevice, gCommandPool, nullptr);
    vkDestroyPipeline(gVkDevice, gPipeline, nullptr);
    vkDestroyPipelineLayout(gVkDevice, gPipelineLayout, nullptr);
    vkDestroyRenderPass(gVkDevice, gRenderPass, nullptr);
    
    SDL_Vulkan_DestroySurface(gVkInstance, gSurface, nullptr);
    SDL_DestroyWindow(gWindow);
    gWindow = nullptr;

    vkDestroyDevice(gVkDevice, nullptr);
    vkDestroyInstance(gVkInstance, nullptr);

    SDL_Vulkan_UnloadLibrary();
}
