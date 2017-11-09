
#include "ChannelStream.h"
#include "RenderThreadInfo.h"
#include "../../../shared/OpenglCodecCommon/ChecksumCalculatorThreadInfo.h"

namespace emugl {

class GlobalRenderThread {
public:
    GlobalRenderThread();

    size_t decode(GLESv1Decoder* gl1,
                  GLESv2Decoder* gl2,
                  renderControl_decoder_context_t* rc,
                  unsigned char* buf, size_t datalen,
                  ChannelStream* stream,
                  ChecksumCalculator* checksumCalc);

};

} // namespace emugl
