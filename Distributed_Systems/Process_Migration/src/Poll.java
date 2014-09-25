import java.util.Enumeration;


public class Poll extends Thread{
	
	/**
	 * Polls if the threads in that node are alive.
	 */
	public void run() {
		while(true) {
			Enumeration<ProcessPacket> items = ProcessManager.runningProcesses.elements();
			while (items.hasMoreElements()) {
				ProcessPacket p = items.nextElement();
				if(!p.t.isAlive())
					ProcessManager.runningProcesses.remove(p.pid);
			}
			
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}		
	}
}
