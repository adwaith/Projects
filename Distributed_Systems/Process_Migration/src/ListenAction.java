import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.Socket;
import java.util.Arrays;


public class ListenAction extends Thread {
	/**
	 * The Socket with which communication is done.
	 */
	private Socket s;
	
	/**
	 * Deserializes the ProcessPacket object and extracts necessary fields
	 * from it. Starts a new thread of the process.
	 * 
	 * @param msg The file name of the serialized object.
	 */
	private void deserialize(String msg) {
		ProcessPacket o;
		FileInputStream file;
		try {
			file = new FileInputStream("migrate/process_" + msg + ".ser");
			ObjectInputStream in;
			in = new ObjectInputStream(file);
			o = (ProcessPacket)in.readObject();
			in.close();
			file.close();
			Thread t = new Thread(o.mp);
			Long pid = ProcessManager.getPid();
			o.pid = pid;
			o.t = t;
			ProcessManager.runningProcesses.put(pid, o);
			t.start();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}  catch (IOException e) {
			e.printStackTrace();
		} catch (ClassNotFoundException e) {
			
		}
	}
	
	/**
	 * Constantly listen for a message to migrate, remote migrate,
	 * getting pid, adding new nodes, retrevies the node list, launch new
	 * process.
	 */
	public void run() {
		try {
		DataInputStream in =
                  new DataInputStream(s.getInputStream());
        String msg = in.readUTF();
        String[] broken = msg.split(":");
        String ret;
        if (broken[0].equalsIgnoreCase("migrate")) {
        	ret = "1";
        	DataOutputStream out;
			try {
				out = new DataOutputStream(s.getOutputStream());
				out.writeUTF(ret);
			} catch (IOException e) {
				e.printStackTrace();
			}
	        
        }
        
        else if (broken[0].equalsIgnoreCase("getpid")) {
        	DataOutputStream out;
			try {
				out = new DataOutputStream(s.getOutputStream());
				out.writeLong(ProcessManager.getPid());
			} catch (IOException e) {
				e.printStackTrace();
			}
        }
        
        else if (broken[0].equalsIgnoreCase("remotemigrate")) {
        	ProcessPacket temp = ProcessManager.runningProcesses.
        								get(Long.parseLong(broken[1]));
        	if(temp == null) {
        		ret = "no";
            	DataOutputStream out;
    			try {
    				out = new DataOutputStream(s.getOutputStream());
    				out.writeUTF(ret);
    			} catch (IOException e) {
    				e.printStackTrace();
    			}
        	}
        	else {
	        	ProcessManager.getNodeListFromMaster();
	        	ProcessManager.nextStep(Long.parseLong(broken[1]), 
	        			Long.parseLong(broken[2]));
	        	ret = "yes";
            	DataOutputStream out;
    			try {
    				out = new DataOutputStream(s.getOutputStream());
    				out.writeUTF(ret);
    			} catch (IOException e) {
    				e.printStackTrace();
    			}
        	}
        }
        
        else if (broken[0].equalsIgnoreCase("pid")) {
        	deserialize(broken[1]);     
        } 
        
        else if (broken[0].equalsIgnoreCase("die")) {
        	ret = "ok";
        	DataOutputStream out;
			try {
				out = new DataOutputStream(s.getOutputStream());
				out.writeUTF(ret);
			} catch (IOException e) {
				e.printStackTrace();
			}     
			System.exit(10);
        } 
        
        else if (broken[0].equalsIgnoreCase("addme")) {
        	long nid = ProcessManager.addToNodeList(broken[1].substring(1), 
        			Integer.parseInt(broken[2]), broken[3]);
        	DataOutputStream out;
			try {
				out = new DataOutputStream(s.getOutputStream());
				out.writeLong(nid);
			} catch (IOException e) {
				e.printStackTrace();
			}
        }
        
        else if (broken[0].equalsIgnoreCase("alive")) {
			ret = "yes";
        	DataOutputStream out;
			try {
				out = new DataOutputStream(s.getOutputStream());
				out.writeUTF(ret);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
        
        else if (broken[0].equalsIgnoreCase("ns")) {
        	ObjectOutputStream out;
			try {
				out = new ObjectOutputStream(s.getOutputStream());
				out.writeObject(ProcessManager.nodeList);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
        
        else if (broken[0].equalsIgnoreCase("nodelist")) {
        	ObjectOutputStream out;
			try {
				out = new ObjectOutputStream(s.getOutputStream());
				out.writeObject(ProcessManager.nodeList);
			} catch (IOException e) {
				e.printStackTrace();
			}
        }
        
        else if (broken[0].equalsIgnoreCase("ps")) {
        	ObjectOutputStream out;
			try {
				out = new ObjectOutputStream(s.getOutputStream());
				out.writeObject(ProcessManager.runningProcesses);
			} catch (IOException e) {
				e.printStackTrace();
			}
        }
        
        else if(broken[0].equalsIgnoreCase("launch")) {
        	String retl = ProcessManager.launchInvoke(Arrays.copyOfRange(broken, 
        			1, broken.length));
        	DataOutputStream out;
			try {
				out = new DataOutputStream(s.getOutputStream());
				out.writeUTF(retl);
			} catch (IOException e) {
				e.printStackTrace();
			}
        }
        
        s.close();
		}
        catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Constructor for the class. Assigns the socket with which 
	 * communication is done.
	 * 
	 * @param s The Socket.
	 */
	public ListenAction(Socket s) {
		this.s = s;
	}
}
