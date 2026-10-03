#include "TestData.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

#include "TestFramework.hpp"
#include "data/DataFileReader.hpp"

namespace ugh::test {

const data::GameData& gameData() {
    static std::unique_ptr<const data::GameData> data = [] {
        std::string error;
        auto loaded = data::DataFileReader::read(dataPath(), error);
        if (!loaded) {
            std::cout << "cannot read the game data: " << error << "\n";
            std::exit(1);
        }
        return loaded;
    }();
    return *data;
}

}  // namespace ugh::test
