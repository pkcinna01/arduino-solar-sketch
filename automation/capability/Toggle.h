//
// Created by pkcinna on 8/21/18.
//

#ifndef AUTOMATION_TOGGLE_H
#define AUTOMATION_TOGGLE_H


#include "Capability.h"

namespace automation {

    class Toggle : public Capability {

    public:

        Toggle(Device* pDevice) : Capability(pDevice) {

        }

        const string getType() const override {
          return "TOGGLE";
        }
    };

};

#endif //AUTOMATION_TOGGLE_H
