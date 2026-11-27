// SPDX-License-Identifier: MIT
#import <Metal/Metal.h>

#include "Rendering/MetalBridge.hpp"

namespace TwentyThreeSeventeen::Rendering
{
    auto
    HasMetal3Device() noexcept -> bool
    {
        @autoreleasepool
        {
            if (@available(macOS 13.0, *))
            {
                id<MTLDevice> device = MTLCreateSystemDefaultDevice();
                return device != nil &&
                       [device supportsFamily:MTLGPUFamilyMetal3];
            }
            return false;
        }
    }
} // namespace TwentyThreeSeventeen::Rendering
