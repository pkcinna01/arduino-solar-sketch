#ifndef AUTOMATION_TOGGLE_H
#define AUTOMATION_TOGGLE_H


#include "Capability.h"

namespace automation {

    class Toggle : public Capability {

    public:

        Toggle(Device* pDevice) : Capability(pDevice) {

        }

        RTTI_GET_TYPE_IMPL(automation,Toggle)
 
    };

};

#endif //AUTOMATION_TOGGLE_H
