
#include "android/base/system/System.h"

#include "Standalone.h"

#include <functional>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using android::base::System;

namespace emugl {

class HelloTriangle : public SampleApplication {
protected:
    struct VertexAttributes {
        float position[2];
        float color[3];
    };
    void initialize() override;
    void draw() override;

private:
    GLint mTransformLoc;
    GLuint mBuffer;
    float mTime = 0.0f;
};

}  // namespace emugl
