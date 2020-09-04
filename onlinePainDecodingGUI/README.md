This client decodes pain onset online.

The acquision of neural spike data and online spike sorting is based on the OmniPlex system hardware and software from Plexon Inc.
Plexon C/C++ SDK is used for interfacing acquistion and decoding:
http://www.plexon.com/sites/default/files/CandC++ClientDevelopmentKit_0.zip

online encoding & decoding algorithm is implemented based on the following paper:

Deciphering neuronal population codes for acute thermal pain
http://iopscience.iop.org/article/10.1088/1741-2552/aa644d/meta;jsessionid=0C9EB705201DCC31053DBB52DE72A02B.c3.iopscience.cld.iop.org

the C++ code uses liblbfgs and eigen3
https://github.com/chokkan/liblbfgs
http://eigen.tuxfamily.org/index.php?title=Main_Page
