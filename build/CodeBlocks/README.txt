Using codeblocks with MSBuild
=============================
One way to get msbuild working with CodeBlocks is to
1. Make sure the compiler is in the PATH (running codeblocks from a developer command prompt will do)
2. Add the 2015 compiler by ether copying, renaming the 2010 builder and change all 10.0 paths to 14.0 and all 7.0 references to 7.1 or add the folowing XML to the default.conf under the compiler/user_sets element

            <microsoft_visual_c_2015>
                <NAME>
                    <str>
                        <![CDATA[Microsoft Visual C++ 2015]]>
                    </str>
                </NAME>
                <PARENT>
                    <str>
                        <![CDATA[msvc10]]>
                    </str>
                </PARENT>
                <INCLUDE_DIRS>
                    <str>
                        <![CDATA[c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\include;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\include;]]>
                    </str>
                </INCLUDE_DIRS>
                <RES_INCLUDE_DIRS>
                    <str>
                        <![CDATA[c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\include;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\include;]]>
                    </str>
                </RES_INCLUDE_DIRS>
                <LIBRARY_DIRS>
                    <str>
                        <![CDATA[c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\lib;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\lib;]]>
                    </str>
                </LIBRARY_DIRS>
                <MASTER_PATH>
                    <str>
                        <![CDATA[c:\Program Files (x86)\Microsoft Visual Studio 14.0\VC]]>
                    </str>
                </MASTER_PATH>
                <EXTRA_PATHS>
                    <str>
                        <![CDATA[c:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\bin;C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1\bin;]]>
                    </str>
                </EXTRA_PATHS>
            </microsoft_visual_c_2015>
            