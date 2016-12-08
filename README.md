# PolarizedText: 

PolarizedText is a library based upon fastText for vector embedding of words. 
A polarization (label) term is introduced in addition to the skipgram model. Such an enbedding can enhance the performance of text classification models (such as the hierachecal softmax provided in fastText) for the problem of many classes. For more details, check out the github post: https://ddu1.github.io/Anonymization/  

## Building polarizedtext

To build

```
$ git clone https://github.com/ddu1/polarizedtext.git
$ cd polarizedtext
$ make
```

A example is provided for 20newsgroups dataset (18828). Simply 

```
$ cd polarizedtext
$ ./twenty_newsgroups.sh
```

