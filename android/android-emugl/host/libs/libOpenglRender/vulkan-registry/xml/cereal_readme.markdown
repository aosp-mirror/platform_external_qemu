# "Cereal" generator for Vulkan driver pass through - Overview

Cereal consists of all necessary components for doing Vulkan driver development
for the emulator. It generates code that lives on both the guest and host.

Guest code:

1. Frontend (`guest/goldfish_vk_frontend`): The functions that are directly referenced
by the Vulkan HAL in the guest. Includes validation.
2. Encoder (`guest/goldfish_vk_encoder`): The functions that communicate API calls to the host
using the stream abstraction.

Host code:

1. Decoder (`host/goldfish_vk_decoder`) Functions using stream abstraction to decode Vulkan API calls
and to execute them on host, with some translation if necessary.

Common code:

1. Marshaling (`guest/goldfish_vk_marshaling`): Functions specifically for transforming Vulkan
structs to and from a serialized format.

