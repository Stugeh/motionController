# invoke SourceDir generated makefile for labra.pem3
labra.pem3: .libraries,labra.pem3
.libraries,labra.pem3: package/cfg/labra_pem3.xdl
	$(MAKE) -f D:\Koulu\JTKJ-LLO\ccs_wrkspc\motionController/src/makefile.libs

clean::
	$(MAKE) -f D:\Koulu\JTKJ-LLO\ccs_wrkspc\motionController/src/makefile.libs clean

