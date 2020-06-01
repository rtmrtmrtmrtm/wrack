# wrack

A toy that compiles a vote-count schema into C++, then executes a 100%
write vote benchmark.

make
./votebm

The schema is in votebm-schema.txt. The C++ code it compiles to
appears in votebm-schema.cc. The benchmark driver is votebm.cc.

On my laptop this yields about 1.5 million writes/second. Maybe the
closest comparison is the paper's DBToaster measurement, which reports
half a million writes per second. But comparison would be growwly
unfair: virtually every feature of Noria is missing from this toy,
including

  * RPC interface (the client code is directly linked)

  * persistence

  * negative records

  * schema change

  * partial

  * multi-core
