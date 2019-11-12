#ifndef _AUTOMATION_CACHEBLE_
#define _AUTOMATION_CACHEBLE_

#include "Automation.h"

namespace automation {

template <typename ValueT>
class Cacheable {

    public:
    mutable unsigned long lastLoadTimeMs;
    mutable ValueT value;

    virtual ValueT getValueNow() const = 0;

    virtual unsigned long getMaxCacheAgeMs() const = 0;

    virtual bool isExpired() const {
        return automation::millisecs() - lastLoadTimeMs > getMaxCacheAgeMs();
    }  

    virtual ValueT getCachedValue() const {
        if ( isExpired() ) {
            value = getValueNow();
            lastLoadTimeMs = automation::millisecs();
        }
        return value;
    }
};

}

#endif