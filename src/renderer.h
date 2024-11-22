#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>

constexpr int VIEWPORT_WIDTH{ 800 };
constexpr int VIEWPORT_HEIGHT{ 800 };

class Renderer {

private:
    SDL_Window* window{ nullptr };
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue queue;
    VkSwapchainKHR swapchain;
    uint32_t swapchain_image_count;
    VkImage* swapchain_images;
    uint32_t current_image_index;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> framebuffers;
    VkPipeline pipeline;
    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence gImageAvailableFence;
    
    bool create_vulkan_instance(uint32_t p_extension_count, const char* const* p_extensions);
    bool create_physical_device();
    bool create_device();
    bool create_swapchain();
    bool create_image_views();
    bool create_render_pass();
    bool create_shader_module(const uint32_t bytes[], const size_t length, VkShaderModule &r_shader_module);
    bool create_pipeline();
    bool create_framebuffers();
    bool create_command_pool();
    bool create_command_buffer();
    void record_command_buffer(VkCommandBuffer p_command_buffer, uint32_t p_image_index);
    bool create_sync_objects();
    void cleanup_swapchain();
    void recreate_swapchain();
    
public:
    bool initialize(uint32_t p_extension_count, const char* const* p_extensions, SDL_Window* p_window);
    void cleanup();
    void draw();

    Renderer() {};
    ~Renderer() {};
};