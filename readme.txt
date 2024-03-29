----------------------------------------------------------------------
Compiling the Guidolib project
----------------------------------------------------------------------

======================================================================
1) Compiling the GUIDOEngine
----------------------------------------------------------------------
The GUIDOEngine relies on CMake, a cross-platform, open-source build 
system ( see http://www.cmake.org/).
The cmake folder contains the project description and is used to generate 
native projects. 
	
Note about the project organization:
-----------------------------------
On MacOS and Windows, in order to support different architectures (win32, win64 
on windows, MacOS, iOS on mac), the library is expected to be compiled in a 'build'
folder located at the root of the project that should contain 4 sub-folders: 
MacOS, iOS, win32 and win64.

To compile for a given platform:
	change to build/your_target_arch directory
	type:  cmake ../../cmake -G "your target generator" OPTIONS
	run your project/makefile and compile

OPTIONS indicates optional components and is between:
	-DMIDIEXPORT='yes | no' to support MIDI export
	
Note about MIDI export:
--------------------------
    MIDI export requires libmidisharelight. For MacOS and Windows, the library is embedded 
    in binary form in the src/midisharelight folder. Thus there is no additional step.
    On linux, you must get the library source code, compile and install.

Note for javascript:
--------------------
See readme.txt in javascript folder.

Note for Android:
-------------------------
    Download the Android SDK and NDK.

    From the build/tools directory of the NDK, invoke make-standalone-toolchain.sh
    to make a standalone toolchain in the directory of your choice (see NDK)
    documentation for how.  More precisely, read section 4 in the document
    STANDALONE-TOOLCHAIN.html that ships with the NDK. Make sure to use a recent
    platform (at least android-18). I will call the path to your standalone toolchain
    $PATH_TO_STANDALONE_TOOLCHAIN, the path to the NDK $PATH_TO_NDK and the path to
    the SDK $PATH_TO_SDK.

    Modify your path to contain the following.  Otherwise, the compiler
    won't find things like the standard library.

      export PATH=$PATH:$PATH_TO_NDK
      export PATH=$PATH:$PATH_TO_SDK/tools
      export PATH=$PATH:$PATH_TO_SDK/platform-tools
      export PATH=$PATH:$PATH_TO_STANDALONE_TOOLCHAIN/bin

    The GUIDO android applications use:
		- the guidoengine java package called guidoengine.jar
		- incidentally, the drawcommand java package called drawcommand.jar

    To compile this, go to the java directory and do:

    make headers
    make class
    make jar

    The jar needs to be simlinked/copied into all the android sample projects that need it
    under the libs/ directory.

    From the directory android-apps/guido-engine-android, run

      ndk-build

    And this will build the library for android.
    To test with a sample project, read the README file in android-apps/simple-guido-editor directory.
      

Note for Linux platforms:
--------------------------
	You need to have libcairo2-dev installed to compile the GUIDOEngine.
	The procedure to compile is close to the usual 'configure' 'make' 'make install'
	steps. Actually, you can simply do the following:
	> cd /your_path_to_the_project/cmake
	> cmake -G "Unix Makefiles"
	> make
	> sudo make install

   Supporting MIDI export on linux:
   -------------------------------
   you must get the midishare source code that includes the midisharelight library:
	   git://midishare.git.sourceforge.net/gitroot/midishare/midishare 
   You don't need to compile midishare but only the midisharelight library.
   midisharelight is a recent addition to the project and for the moment, it is only
   available from the 'dev' branch. It is located at the project root folder.
   midisharelight is cmake based:
	> cd midisharelight/cmake
	> cmake -G "Unix Makefiles"
	> make
	> sudo make install


Note for Windows platforms:
--------------------------
	The CMake project description is "Visual Studio" oriented. 
	Using MingW may require some adaptation.
	/!\ Having .NET Framework v4.5 installed can be problematic during cmake generation
			AND during Visual Studio compilation. A downgrade to v4.0 will fix this problems.


======================================================================
2) Compiling the GUIDO Qt applications
----------------------------------------------------------------------
You need to have Qt SDK version 5.0 or later installed to compile the 
GUIDO Qt applications.
(see at http://qt-project.org//)
You can use QTCreator on all platforms to compile the applications.
See below if you want to use another development environment.

>>>>>> Mac OS
----------------------------
To compile the GUIDO Qt applications do the following:
  > cd /your_path_to_the_guido_project/Qt
  > make projects
Next you should find Xcode projects into every application folder.
Just open the projects and compile.

>>>>>> Windows
----------------------------
To compile the GUIDO Qt applications, the easiest way is probably to use QTCreator.
However, if you want to use Visual Studio you should:
- set the QMAKESPEC variable to the corresponding output specification or use
  the -spec option with qmake (see Qt documentation)
- generate each project in each application folder using 'qmake'
Note that a solution for Visual Studio 2010 is provided at the root of the Qt folder.
Warning, the solution works only after the individual projects have been generated.

>>>>>> Linux
----------------------------
Qt can be installed from synaptic (on Ubuntu)
To compile the GUIDO Qt applications do the following:
  > cd /your_path_to_the_guido_project/Qt
  > make unix

======================================================================
In case of trouble, contact me: <fober@grame.fr>
======================================================================
