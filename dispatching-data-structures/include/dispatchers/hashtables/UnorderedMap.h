#ifndef PROTOTYPE_UNORDEREDMAP_H
#define PROTOTYPE_UNORDEREDMAP_H

#include "Defines.h"
#include "dispatchers/IDispatcher.h"

#include <unordered_map>

#define REHASH_BUCKET_INCREASE 100

class UnorderedMap : public IDispatcher {
public:
    ~UnorderedMap() override;

    bool registerAnalyzer(identifier_t identifier, const analyzer_builder &make_analyzer) override;
    void registerAnalyzers(const std::map<identifier_t, analyzer_builder> &analyzer_builders) override;
    IAnalyzer *lookup(identifier_t identifier) override;
    size_t size() override;
    void clear() override;

    size_t real_size() override;

private:
    std::unordered_map<identifier_t, IAnalyzer*> table;
    void stringifyAnalyzersState(std::ostream &os) const override;

    void freeAnalyzers();

    inline bool containsBucketCollision() {
        for (size_t bucket = 0; bucket < table.bucket_count(); bucket++) {
            if (table.bucket_size(bucket) > 1) {
                return true;
            }
        }
        return false;
    }
};

#endif //PROTOTYPE_UNORDEREDMAP_H
