#include "surrender/srGERD.h"

// FUNCTION: SURRENDER 0x10024DB0
int srGERD::Renderer::isBatchFull() const
{
    if (sorted_d8_ == 0 && batch_limit_dc_ < batch_count_b8_) {
        return 1;
    }
    return 0;
}
