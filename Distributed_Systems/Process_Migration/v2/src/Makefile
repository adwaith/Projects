JFLAGS =
JC = javac
.SUFFIXES: .java .class
.java.class:
		$(JC) $(JFLAGS) $*.java

CLASSES = \
		Listen.java \
		MigratableProcess.java \
		Parser.java \
		Poll.java \
		ProcessManager.java \
		ProcessPacket.java \
		test1.java \
		test2.java \
		TransactionalFileInputStream.java \
		TransactionalFileOutputStream.java

default: classes

classes: $(CLASSES:.java=.class)

clean:
		$(RM) *.class