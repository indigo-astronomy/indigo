Atik Cameras 2021

We recommend placing the dll's provided into C:\Windows\System32 & C:\Windows\SysWOW64 respectively so that they are globally accessible 
(64 -> System32 & 32 -> SysWOW64 on a 64 bit system or 32 -> System32 on a 32 bit system)

You can also place the dll's alongside your produced .exe file if you do not want the dll's to be globally accessible on the machine. 

You will need the platform toolset v141 (Visual Studio 2017) c++ redistributable at least to use these dll's which can be found here: 
	https://support.microsoft.com/en-gb/help/2977003/the-latest-supported-visual-c-downloads

Since the 2015 toolset Microsoft have kept ABI compatability and as our dll is built with the 2017 toolset it is compatible with the 2019 toolset, read more here:
	https://docs.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017?view=msvc-160