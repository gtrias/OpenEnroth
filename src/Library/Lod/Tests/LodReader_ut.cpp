#include <string>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Library/FileSystem/Directory/DirectoryFileSystem.h"
#include "Library/Lod/LodReader.h"
#include "Library/Lod/LodWriter.h"

#include "Utility/Streams/BlobOutputStream.h"

const char brokenLod[] =
    "LOD\0"         "Game"          "MMVI"          "\0\0\0\0"      // signature, version
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "Maps"          " for"          " MMV"          // description
    "I\0\0\0"       "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\x64\0\0\0"    "\0\0\0\0"      "\1\0\0\0"      // size = 100, unk_0 = 0, numDirectories = 1
    "I\0\0\0"       "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      // unk_1
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"
    "maps"          "\0\0\0\0"      "\0\0\0\0"      "\0\0\0\0"      // name
    "\x20\x01\0\0"  "\x21\0\0\0"    "\0\0\0\0"      "\x01\0\0\0"    // dataOffset = 288, dataSize = 33, unk_0 = 0, numItems = 1, priority = 0
                                                                    //                              ^ actual data size is 48
    "lolk"          "ek\0\0"        "\0\0\0\0"      "\0\0\0\0"      // name
    "\x20\0\0\0"    "\x10\0\0\0"    "\0\0\0\0"      "\0\0\0\0"      // dataOffset = 32, dataSize = 16, unk_0 = 0, numItems = 0, priority = 0
    "data"          "data"          "data"          "data";

UNIT_TEST(LodReader, RussianLod) {
    // Opening a LOD with invalid directory dataSize should just work.
    LodReader reader(Blob::view(brokenLod, sizeof(brokenLod)).withDisplayPath("russian.lod"), LOD_ALLOW_DUPLICATES);
    EXPECT_TRUE(reader.isOpen());
    EXPECT_EQ(reader.ls(), std::vector<std::string>{"lolkek"});
    EXPECT_EQ(reader.info().rootName, "maps");
    EXPECT_EQ(reader.info().description, "Maps for MMVI");
    EXPECT_EQ(reader.info().version, LOD_VERSION_MM6_GAME);
    EXPECT_TRUE(reader.exists("lolkek"));
    EXPECT_FALSE(reader.exists("lolkek1"));
    EXPECT_FALSE(reader.exists("lolke"));
    EXPECT_EQ(reader.read("lolkek").str(), "datadatadatadata");

    // LODs are case-insensitive.
    EXPECT_TRUE(reader.exists("lolKEK"));
    EXPECT_EQ(reader.read("LOLkek").str(), "datadatadatadata");

    // Check that we throw when accessing non-existent files.
    EXPECT_THROW((void) reader.read("lolke"), std::exception);
}

UNIT_TEST(LodReader, ErrorMessage) {
    std::string_view name = "XXXXXXXXXXX";
    Blob blob = Blob().withDisplayPath(name);

    EXPECT_THROW_MESSAGE(LodReader(Blob::share(blob)), name);
}

UNIT_TEST(LodReader, DisplayPath) {
    LodReader reader(Blob::view(brokenLod, sizeof(brokenLod)).withDisplayPath("russian.lod"), LOD_ALLOW_DUPLICATES);
    EXPECT_EQ(reader.read("lolkek").displayPath(), "russian.lod/lolkek");
    EXPECT_EQ(reader.read("LOLKEK").displayPath(), "russian.lod/LOLKEK");
}

UNIT_TEST(LodReader, Detect) {
    EXPECT_TRUE(lod::detect(Blob::view(brokenLod, sizeof(brokenLod))));
}

UNIT_TEST(LodReader, StreamingBrokenLod) {
    // Same data as in RussianLod above, but read through the streaming path: only the index is read into memory, the
    // file entry is read on demand. Note that this LOD's root directory entry spans past its index, which is exactly
    // what the streaming path has to handle without reading the whole file.
    ScopedTestFile tmp("russian.lod", std::string_view(brokenLod, sizeof(brokenLod)));

    DirectoryFileSystem fs(NativePath(""));
    LodReader reader;
    reader.open(&fs, "russian.lod", LOD_ALLOW_DUPLICATES);

    EXPECT_TRUE(reader.isOpen());
    EXPECT_EQ(reader.ls(), std::vector<std::string>{"lolkek"});
    EXPECT_EQ(reader.info().rootName, "maps");
    EXPECT_EQ(reader.info().description, "Maps for MMVI");
    EXPECT_EQ(reader.info().version, LOD_VERSION_MM6_GAME);
    EXPECT_TRUE(reader.exists("lolkek"));
    EXPECT_FALSE(reader.exists("lolkek1"));
    EXPECT_EQ(reader.read("lolkek").str(), "datadatadatadata");

    // LODs are case-insensitive.
    EXPECT_TRUE(reader.exists("lolKEK"));
    EXPECT_EQ(reader.read("LOLkek").str(), "datadatadatadata");

    // Check that we throw when accessing non-existent files.
    EXPECT_THROW((void) reader.read("lolke"), std::exception);
}

UNIT_TEST(LodReader, StreamingMatchesInMemory) {
    LodInfo info;
    info.version = LOD_VERSION_MM7;
    info.description = "Some LOD";
    info.rootName = "data";

    std::string file1 = "123";
    std::string file2 = "a";
    std::string file3 = "";
    std::string file4 = std::string(1'000'000, '0');

    Blob lod;
    BlobOutputStream stream(&lod, "some.lod");

    LodWriter writer(&stream, info);
    writer.write("1", Blob::view(file1));
    writer.write("2", Blob::view(file2));
    writer.write("3", Blob::view(file3));
    writer.write("4", Blob::view(file4));
    writer.close();
    stream.close();

    ScopedTestFile tmp("some.lod", lod.str());
    DirectoryFileSystem fs(NativePath(""));
    LodReader streamed;
    streamed.open(&fs, "some.lod");

    LodReader inMemory(std::move(lod));

    // Streaming must be indistinguishable from keeping the whole LOD in memory - including empty entries, and entries
    // that sit past a megabyte of other data.
    EXPECT_EQ(streamed.ls(), inMemory.ls());
    EXPECT_EQ(streamed.info().rootName, inMemory.info().rootName);
    EXPECT_EQ(streamed.info().description, inMemory.info().description);
    EXPECT_EQ(streamed.info().version, inMemory.info().version);
    for (std::string_view name : {"1", "2", "3", "4"}) {
        EXPECT_EQ(streamed.read(name).str(), inMemory.read(name).str()) << "entry " << name;
    }
}
