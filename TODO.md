### Chunk file format should use integer DocId and StreamId values.

* Update file format design. Will likely need to reserve sentinel values indicating
no more documents and no more streams, or will need to introduce document and stream
counters.
* Update file format documentation on blog.
* Update WorkBench.
* Update ChunkIngester.
* Add FileHeader and Version to Chunk file.



### Factor chunk ingestion into its own library.

* Document will stay in Index and be part of BitFunnel public API.
* Figure out how to minimize dependencies between chunk library and implementation
of Document.
* IngestChunks
* ChunkEnumerator
* ChunkTaskProcessor
* ChunkIngestor
* ChunkReader



### Decide on Term class fields

* Does Term need to track idfSum and idfMax?
* Does Term need to track ngramSize?



### Decide whether Term needs to support IObjectFormatter and IObjectParser.

* Seems to be used by TermVectorUnitTest.cpp.
* Do we need this for printing out the term frequency tables?



### Integrate document length histogram into Shard.[mihaela]



### Integrate ShardDefinition into Ingestor

* Create one shard per entry in ShardDefinition.
* Ingestor::Add() uses ShardDefinition to choose shard.
* Initially use hand-authored shard definition.



### Port optimal shard.

* ShardDefinitionBuilder
* SingleSourceShortestPath
* ShardCostFunction



### Port FileManager. [mhop]

* Create new Common/Configuration project.
* FileManager goes in this project.



### Add arena allocation to chunk ingestion pipeline.

* One arena or buffer per thread to hold entire chunk file. Reset per file.
* One arena per thread to hold document data. Reset per document.
* Implement or port arena that can expand its pool of memory.
* Implement or port std wrapper to use arenas with std::unordered_set.
* Plumb through chunk ingestion pipeline.



### Class comments for Chunk ingestion pipeline. [mhop]

* Overview
* IngestChunks
* ChunkEnumerator
* ChunkTaskProcessor
* ChunkIngestor
* ChunkReader
* Document
* Ingestor



### Chunk ingestion pipeline unit tests. [aclemmer]

* IngestChunks
* ChunkEnumerator
* ChunkTaskProcessor
* ChunkIngestor
* ChunkReader
* Document
* Ingestor
* Shard
* Slice
* Term
* DocumentHandleInternal



### DocumentFrequencyTable [mhop]

* Design file format. CSV vs binary.
* Sorted?
* Version header?
* How do we incorporate FileHeader into CSV?
* Initial version uses std::unordered_map.
* Decide whether new version needs to be lock free.
* Decide whether new version needs to be POD for fast load.
* Do we need Builder/Table separation?
  * Question has to do with sorting.
  * Can we afford to use an std::map or must we use a lock free hash table?
  * Do we need to enumerate terms in descending frequency for term table builder?
* Scenarios
  * Gathering term frequency data.
  * Term table?



### Port Version and FileHeader [mhop]

* Reimplement version parser.
* Potentially redesign to work with plain text files.



### GTest submodule.

* GTest brought into src/gtest via git submodule.
* Update README.md instructions on cloning/fetching.
* GTest integrated in CMakeLists.txt via add_subdirectory.



### Port AllocatorInterfaces and Arena allocators.

* IAllocator
* General allocator with initial buffer size and ability to grow.
* std::allocator wrapper.



### Fix Appveyor CI build.


### Investigate use of Boost Serialization

* Boost Serialization is not a header-only library and would need to be built.
* How would we integrate Boost into our build?
* How is the performance for std::unordered_map vs writing our own serialization?
* Can we interoperate with file headers and versions?
* Do we even want a binary format or will we be all CSV?
* License compatability





### Fix Travis CI build.


### Initial corpus statistics analysis. [aclemmer, mihaela]

* Generate document length histogram and term frequency tables for English Wikipedia.
* Generate optimal shard definition or some proxy definition.
* Generate term frequency tables per shard for English Wikipedia.
* Form an opinion/recommendation on whether each shard needs its own term table.
* Model costs (space, speed, false positives) of various row configuration strategies.
  * Would like to do away with BandTable and trainer.
  * Need to select adhoc term cutoff idf.
  * Need to know what term characteristics (e.g. ngram size, idf sum, idf max, etc.)
    are needed for adhoc row configuration.


### Ensure index branch builds and passes tests on Linux and OSX. [aclemmer]


