#include "LOD.h"

#include <memory>

#include "EngineFileSystem.h"

std::unique_ptr<LodReader> pGames_LOD;

bool Initialize_GamesLOD_NewLOD() {
    pGames_LOD = std::make_unique<LodReader>();
#ifdef __vita__
    // See LodReader::open(FileSystem *): the Vita can't hold the game's LODs in RAM at once.
    pGames_LOD->open(dfs, "data/games.lod");
#else
    pGames_LOD->open(dfs->read("data/games.lod"));
#endif
    return true;
}
