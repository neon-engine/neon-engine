#include "extension-running.hpp"

namespace neon
{
  ExtensionRunning::ExtensionRunning(ExtensionHost *extensions, ComponentFormats *formats)
  {
    _extensions = extensions;
    _formats = formats;
  }

  void ExtensionRunning::Register(EntityStore &store)
  {
    _extensions->RegisterComponents(store, *_formats);
  }

  void ExtensionRunning::Initialize(EntityStore &store)
  {
    _extensions->Start(store);
  }

  void ExtensionRunning::Update(EntityStore &store, const double delta_time)
  {
    _extensions->Update(delta_time);
  }

  void ExtensionRunning::FixedUpdate(EntityStore &store, const double fixed_delta_time)
  {
    _extensions->FixedUpdate(fixed_delta_time);
  }

  void ExtensionRunning::Interpolate(EntityStore &store, const double blend)
  {
    _extensions->Interpolate(blend);
  }
} // neon
