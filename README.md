Cascade: Reproducible Segmentation of White Matter Changes using Strict Statistical Definition
=======

__This is the Cascade, academic version. Cascade-academic depends on **FSL** so you need acknowledge FSL terms of use before using this software.__


Content
-------
* [Introduction](#introduction)
* [Installation](#install)
* [Citation](#citation)
* [Copyright](#copyright)
* [License](#license)


Introduction
-------
Cascade, Classification of White Matter Changes, is a fully automated tool for quantification of White Matter Changes. Cascade is designed to be as flexible as possible. and can work with all sort of input sequences. It can work without any manually delineated samples as reference.

Please [report any issue](https://github.com/Damangir/Cascade/issues) at https://github.com/Damangir/Cascade/issues.

Install
-------
In order to install the software you need to have the following application installed on your computer.

 * Modern C++ compiler (gcc is recommended)
 * make
 * cmake 2.8+ (cmake.org)
 * Insight Toolkit 4.0+ (itk.org)

For the script generator and premade scripts you may use:
 * Bash 3+

You can check availability of these packages on your computer (on Unix-like computers e.g. Mac and Ubuntu). Please note the last command might take a few minutes to run.
```bash
~$ echo -ne "C++ compiler: "; command -v cc||  command -v gcc||  command -v clang||  command -v c++||  echo "No C++ compiler found"
~$ make --version
~$ cmake --version
~$ bash --version
~$ find / -name "ITKConfigVersion.cmake" -print -exec head -n1 {} \; 2>/dev/null
```

Once you obtain the Cascade, you can install it via:
```bash
~$ cd Cascade
Cascade$ mkdir build
Cascade$ cd build
build$ cmake ../src
build$ make
```

Before using the scripts you should export some envirnment variables:
```bash
export CASCADE_BIN=Cascade/build
export CASCADE_DATA=Cascade/data
```

You can use script generators located at src/Shell to generate scripts for your studies. There are some ready to use scripts which can import the results of [freesurfer](http://surfer.nmr.mgh.harvard.edu/) or [volbrain](http://volbrain.upv.es/) and runs the Cascade based on their results. It is recommended to import the results of your GM analysis if you have any in your studies. Otherwise use the script generator to generate a script which handles GM analysis internally.

An example usage of the softwares are:

```bash
$ RADIUS=1.5 FREESURFER_DIR=/path/to/FS_Subject FLAIR=/path/to/FLAIR OUTPUT_DIR=/path/to/cascade/output  modelfree.freesurfer.sh
```

or

```bash
$ RADIUS=1.5 VOLBRAIN_DIR=/path/to/volbrain_Subject FLAIR=/path/to/FLAIR OUTPUT_DIR=/path/to/cascade/output  modelfree.volbrain.sh
```

These scripts will generate a p-value mape ( 1-pvalue ) for being WML. You can use report script should you need to convert it to binary mask.

Citation
-------
We are publishing a paper detailing Cascade. Until its publication, if you are using this software in your research, you may use information at:

> Damangir S, Manzouri A, Oppedal K, Carlsson S, Firbank MJ, Sonnesyn H, Tysnes OB, O'Brien JT, Beyer MK, Westman E, Aarsland D, Wahlund LO, Spulber G. Multispectral MRI segmentation of age related white matter changes using a cascade of support vector machines. J Neurol Sci. 2012 Nov 15;322(1-2):211-6. doi: 10.1016/j.jns.2012.07.064. Epub 2012 Aug 24. PubMed PMID: 22921728.

Any scientific work derived from the result of this software or its modifications should refer to these papers:
* Single node reduced SVM for purging evident non-WMC: http://www.ncbi.nlm.nih.gov/pubmed/22921728

Copyright
-------
Copyright (C) 2013-2015 [Soheil Damangir](http://www.linkedin.com/in/soheildamangir) - All Rights Reserved

License
-------
[![Creative Commons License](https://raw.github.com/Damangir/Cascade/master/license.png "Creative Commons License")](http://creativecommons.org/licenses/by-nc-nd/3.0/)

Cascade by [Soheil Damangir](http://www.linkedin.com/in/soheildamangir) is licensed under a [Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License](http://creativecommons.org/licenses/by-nc-nd/3.0/).
To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/3.0/.