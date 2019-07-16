# Optimizing And-Inverter Graph under Maximum Fanout Constraint

## Platform

Please build and run the program on a 64-bit Unix system.

## Compile

```
make
```

The c++11 feature is required.
It will generate a program called contest.out.

## Run

```
usage: ./contest.out --input=string [options] ...
options:
  -i, --input     Input BLIF file (SOP style) (string)
  -o, --output    Output BLIF file (SOP style) (string [=])
  -l, --limit     Maximum fanout constraint (int [=2])
  -?, --help      print this message
```

For example,
the input file is contestA.blif,
the maximum fanout constraint is 6,
the output file name is contestA_constraint_6.blif,
then you should run:

```
./contest.out -i contestA.blif -l 6 -o contestA_constraint_6.blif
```
