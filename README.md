# PolarizedText: 

PolarizedText is a library based upon fastText for vector embedding of words. 
In addition to the skipgram model with negative sampling, a polarization term is introduced. This is in particular helpful for word similarity analysis like anonymization.

## Building polarizedtext

To build

```
$ git clone https://github.com/ddu1/polarizedtext.git
$ cd polarizedtext
$ make
```

A example is provided to embed the 20newsgroups dataset. Simply 

```
$ cd polarizedtext
$ ./example.sh
```


## License

fastText is BSD-licensed. We also provide an additional patent grant.
