#ifndef PROTOTYPE_IMETA_H
#define PROTOTYPE_IMETA_H

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

class IMeta : public IDispatcher {
public:
    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    size_t size() override;
    void clear() override;
    size_t real_size() override {
        return 0;
    }

private:
    size_t _size = 0;
};


#endif //PROTOTYPE_IMETA_H
