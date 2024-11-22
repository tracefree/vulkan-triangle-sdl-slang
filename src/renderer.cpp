#include "renderer.h"

#include "util.h"
#include "shaders/triangle.h"


bool Renderer::create_vulkan_instance(uint32_t p_extension_count, const char* const* p_extensions) {
    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.apiVersion = VK_API_VERSION_1_3;

    const char* enabled_layers[1] = {"VK_LAYER_KHRONOS_validation"};

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledExtensionCount = p_extension_count;
    create_info.ppEnabledExtensionNames = p_extensions;
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = enabled_layers;

    return vkCreateInstance(&create_info, nullptr, &instance) == VK_SUCCESS;
};

bool Renderer::create_physical_device() {
    uint32_t physical_device_count;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (physical_device_count == 0) {
        print("Could not find a physical rendering device!");
        return false;
    }

    VkPhysicalDevice* physical_devices = new VkPhysicalDevice[physical_device_count];
    VkResult result = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    // TODO: Choose the best graphics card by querying support for required properties.
    physical_device = physical_devices[0];

    return result == VK_SUCCESS;
}

bool Renderer::create_device() {
    uint32_t queue_family_count {0};
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    int queue_index {0};
    
    for (const VkQueueFamilyProperties &queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 presentation_support { false };
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, surface, &presentation_support);
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

    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(device, queue_index, 0, &queue);
    return true;
}

bool Renderer::create_swapchain() {
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = 3;
    create_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent = VkExtent2D {VIEWPORT_WIDTH, VIEWPORT_HEIGHT};
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.clipped = VK_TRUE;

    return vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) == VK_SUCCESS;
}

bool Renderer::create_image_views() {
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
    swapchain_images = new VkImage[swapchain_image_count];
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);
    swapchain_image_views.resize(swapchain_image_count);

    for (size_t i = i; i < swapchain_image_count; i++) {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images[i];
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

        if (vkCreateImageView(device, &create_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool Renderer::create_render_pass() {
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

    return vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) == VK_SUCCESS;
}


bool Renderer::create_shader_module(const uint32_t bytes[], const size_t length, VkShaderModule &r_shader_module) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = length;
    create_info.pCode = bytes;
    return vkCreateShaderModule(device, &create_info, nullptr, &r_shader_module) == VK_SUCCESS;
}


bool Renderer::create_pipeline() {
    VkShaderModule shader_module;

    if (!create_shader_module(triangle_spv, triangle_spv_sizeInBytes, shader_module)) {
        print("Could not create shader modules!");
        return false;
    }

    VkPipelineShaderStageCreateInfo vert_shader_stage_info {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = shader_module;
    vert_shader_stage_info.pName = "vertex";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = shader_module;
    frag_shader_stage_info.pName = "fragment";

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
    viewport.width  = (float) VIEWPORT_WIDTH;
    viewport.height = (float) VIEWPORT_HEIGHT;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width  = (float) VIEWPORT_WIDTH;
    scissor.extent.height = (float) VIEWPORT_HEIGHT;

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

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
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
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    if (!vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) == VK_SUCCESS) {
        print("Could not create graphics pipeline!");
        return false;
    }

    vkDestroyShaderModule(device, shader_module, nullptr);

    return true;
}


bool Renderer::create_framebuffers() {
    framebuffers.resize(swapchain_image_views.size());

    for (size_t i = 0; i < swapchain_image_views.size(); i++) {
        VkImageView attachments[] = {
            swapchain_image_views[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = VIEWPORT_WIDTH;
        framebuffer_info.height = VIEWPORT_HEIGHT;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}


bool Renderer::create_command_pool() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = 0;

    return vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) == VK_SUCCESS;
}


bool Renderer::create_command_buffer() {
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    return vkAllocateCommandBuffers(device, &alloc_info, &command_buffer) == VK_SUCCESS;
}


void Renderer::record_command_buffer(VkCommandBuffer p_command_buffer, uint32_t p_image_index) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(p_command_buffer, &begin_info) != VK_SUCCESS) {
        print("Could not begin command buffer!");
    }

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = framebuffers[p_image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent.width = VIEWPORT_WIDTH;
    render_pass_info.renderArea.extent.height = VIEWPORT_HEIGHT;

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(p_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(p_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(VIEWPORT_WIDTH);
    viewport.height = static_cast<float>(VIEWPORT_HEIGHT);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(p_command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {VIEWPORT_WIDTH, VIEWPORT_HEIGHT};
    vkCmdSetScissor(p_command_buffer, 0, 1, &scissor);

    vkCmdDraw(p_command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(p_command_buffer);
    
    if (vkEndCommandBuffer(p_command_buffer) != VK_SUCCESS) {
        print("Could not end command buffer!");
    }
}


bool Renderer::create_sync_objects() {
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    int result;
    result  = (int)vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphore);
    result += (int)vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphore);
    result += (int)vkCreateFence(device, &fence_info, nullptr, &gImageAvailableFence);

    return result == 0;
}


void Renderer::cleanup_swapchain() {
    vkDeviceWaitIdle(device);
    for (VkFramebuffer framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (VkImageView image_view : swapchain_image_views) {
        vkDestroyImageView(device, image_view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}


void Renderer::recreate_swapchain() {
    vkDeviceWaitIdle(device);

    cleanup_swapchain();

    create_swapchain();
    create_image_views();
    create_framebuffers();
}

bool Renderer::initialize(uint32_t p_extension_count, const char* const* p_extensions, SDL_Window* p_window) {
    window = p_window;
    
    if (!create_vulkan_instance(p_extension_count, p_extensions)) {
        print("Could not create Vulkan instance!");
        return false;
    }
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        print("Could not create Vulkan surface! SDL error: %s", SDL_GetError());
        return false;
    }
    if (!create_physical_device()) {
        print("Could not create physical device!");
        return false;
    }
    if (!create_device()) {
        print("Could not create logical device!");
        return false;
    }
    if(!create_swapchain()) {
        print("Could not create swapchain!");
        return false;
    }
    if (!create_image_views()) {
        print("Could not create image views!");
        return false;
    }
    if (!create_render_pass()) {
        print("Could not create render pass!");
        return false;
    }
    if (!create_pipeline()) {
        print("Could not create pipeline!");
        return false;
    }
    if (!create_framebuffers()) {
        print("Could not create framebuffers!");
        return false;
    }
    if (!create_command_pool()) {
        print("Could not create command pool!");
        return false;
    }
    if (!create_command_buffer()) {
        print("Could not create command buffer!");
        return false;
    }
    if (!create_sync_objects()) {
        print("Could not create fence and semaphores!");
        return false;
    }

    return true;
}

void Renderer::cleanup() {
    cleanup_swapchain();

    vkDestroySemaphore(device, image_available_semaphore, nullptr);
    vkDestroySemaphore(device, render_finished_semaphore, nullptr);
    vkDestroyFence(device, gImageAvailableFence, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    SDL_DestroyWindow(window);
    window = nullptr;

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    SDL_Vulkan_UnloadLibrary();
}

void Renderer::draw() {
    vkWaitForFences(device, 1, &gImageAvailableFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &gImageAvailableFence);

    uint32_t image_index;
    VkResult acquisition_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
    if (acquisition_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    }

    vkResetCommandBuffer(command_buffer, 0);
    record_command_buffer(command_buffer, image_index);     

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphore;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semaphore;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, gImageAvailableFence);

    VkSwapchainKHR swap_chains[] = {swapchain};
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    VkResult presentation_result = vkQueuePresentKHR(queue, &present_info);
    if (presentation_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
    }
}