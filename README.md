# EPOS

The [Embedded Parallel Operating System (EPOS)](https://epos.lisha.ufsc.br) project aims at automating the development of embedded systems so that developers can concentrate on what really matters: their applications. EPOS relies on the Application-Driven Embedded System Design (ADESD) method to guide the development of both software and hardware components that can be automatically adapted to fulfill the requirements of particular applications. EPOS features a set of tools to support developers in selecting, configuring, and plugging components into its application-specific framework. The combination of methodology, components, frameworks, and tools enable the automatic generation of an application-specific embedded system instances. 

## Getting Started

Check the **Setting up EPOS** section of the [manual](https://epos.lisha.ufsc.br/EPOS+2+User+Guide#Setting_up_EPOS).

### Prerequisites

* **Cross-compilers** for the target architecture you intend to use.

    Fedora packs compilers for x86 that can be installed with ```dnf install binutils-x86_64-linux-gnu gcc-c++-x86_64-linux-gnu``` and ARM compilers that can be installed with: ```dnf install arm-none-eabi-binutils-cs arm-none-eabi-gcc-cs-c++ arm-none-eabi-newlib```.

    On ubuntu 18.04 the x86 packs can be installed with ```apt install binutils-x86-64-linux-gnu```, and ARM compilers can be installed with ```apt install binutils-arm-none-eabi gcc-arm-none-eabi```. Make sure your ubuntu has the ```make``` package already installed.

* **32-bit development libs** (if your development platform is 64-bit)

    For fedora: ```dnf install libc-devel.i686 libstdc++.i686 libstdc++-devel zlib.i686```

    For ubuntu 18.04: ```apt install lib32stdc++6 libc6-i386 libc6-dev-i386 lib32z1 lib32ncurses5 libbz2-1.0:i386 gcc-multilib g++-multilib```

* **Intel 8086** tools (to compile the bootstrap, only if you intend to use x86)

    For fedora: ```dnf install dev86```

    For ubuntu: ```apt install bin86```

### Installing

Simply extract the tarball or clone the repository in a convenient location for you. EPOS is fully self-contained. 

### Building

Go into the directory where you extracted EPOS and issue a ```make all``` to have instances of EPOS built for each of the applications in the ```app``` directory. 

You can also built for specific applications using ```make APPLICATION=<app>```, where \<app\> is a subdir of ```app```.

### Running

After building an application-oriented instance of EPOS, you can run the application with the tailored EPOS on QEMU using: ```make APPLICATION=<app> run```

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://gitlab.lisha.ufsc.br/osdi/teaching2/tags). 

## Authors

* **Antônio Augusto Fröhlich** - *Initial work* - [Guto](https://lisha.ufsc.br/Guto)

See also the list of [contributors](https://epos.lisha.ufsc.br/EPOS+Developers) who participated in this project.

## License

This project is licensed under the GPL 2.0 License - see the [LICENSE](LICENSE) file for details
