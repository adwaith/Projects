JFLAGS =
JC = javac
.SUFFIXES: .java .class
.java.class:
		$(JC) $(JFLAGS) $*.java

CLASSES = \
		Calc.java \
		CalcInterface.java \
		Case.java \
		CaseInterface.java \
		Client.java \
		Listen.java \
		ListenAction.java \
		RegPacket.java \
		RemoteHandler.java \
		RemoteObjectReference.java \
		RMIRegistry.java \
		Server.java \

default: classes

classes: $(CLASSES:.java=.class)

clean:
		$(RM) *.class
