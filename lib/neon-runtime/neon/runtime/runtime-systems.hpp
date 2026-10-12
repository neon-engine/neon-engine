#ifndef RUNTIME_SYSTEMS_HPP
#define RUNTIME_SYSTEMS_HPP

#include <memory>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-system.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/logging/logger.hpp>
#include <neon/logging/logging-system.hpp>
#include <neon/render/render-pipeline.hpp>
#include <neon/render/render-system.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/runtime/settings-config.hpp>
#include <neon/window/sdl2-window-system.hpp>
#include <neon/window/window-system.hpp>
#include <neon/world-system/ecs/entity-world.hpp>

namespace neon
{
  /// What the shared start-up has made by the time the application's own
  /// objects are built: the systems a Runtime is given, and the world they
  /// serve. Handed to the hooks of CompositionHooks, see
  /// RuntimeComposition. Everything is owned by the composition and lives
  /// until it is cleaned up.
  struct RuntimeSystems
  {
    const SettingsConfig *settings;
    WindowSystem *window;
    InputSystem *input;
    RenderSystem *render;
    RenderPipeline *render_pipeline;
    LoggingSystem *logging;
    EntityWorld *world;

    /// The logger named after the application, which its Runtime logs with.
    std::shared_ptr<Logger> logger;

    /// The file system and the format of the documents, for an application
    /// that reads a file of its own at the start, as the editor reads a
    /// script of its tools.
    FileSystemContext *file_system = nullptr;
    DocumentFormat *document_format = nullptr;

    /// The backends behind the window, the input, and the renderer, for an
    /// application that draws with them as the editor's tools do. Null
    /// without a window, where the headless systems stand in.
    SDL2_WindowSystem *sdl2_window = nullptr;
    SDL2_InputSystem *sdl2_input = nullptr;
    VK_RenderSystem *vk_render = nullptr;
  };
} // neon

#endif //RUNTIME_SYSTEMS_HPP
