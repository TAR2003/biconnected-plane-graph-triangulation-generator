#include "BenchCommon.hpp"

#include "GenerateOneconnectedTriangulations.hpp"

namespace tb
{
    namespace
    {
        class OneconnectedRun final : public GeneratorRun
        {
            GenerateOneconnectedTriangulations generator_;

        public:
            OneconnectedRun(long long vertexCount, const std::vector<std::vector<long long>> &adjacency, long long limit)
                : generator_(vertexCount, adjacency, limit) {}

            void generateAllTriangulations() override { generator_.generateAllTriangulations(); }

            GeneratorStats stats() const override
            {
                return {generator_.totalTriangulations, false, 0, 0};
            }
        };
    } // namespace

    std::unique_ptr<GeneratorRun> makeOneconnected(
        long long vertexCount, const std::vector<std::vector<long long>> &adjacency, long long limit)
    {
        return std::make_unique<OneconnectedRun>(vertexCount, adjacency, limit);
    }
} // namespace tb
