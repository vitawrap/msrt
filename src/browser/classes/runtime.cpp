#include "runtime.hpp"

namespace ms {
namespace browser {

    void Runtime::startReady() {

        startVM.invoke();
    }

}
}
