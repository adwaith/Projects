import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.net.ConnectException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.concurrent.ConcurrentHashMap;


public class ProcessManager {
	
	/**
	 * Stores a list of all the processes running in the 
	 * current node. The Key is the process ID and the
	 * value is the ProcessPacket, which contains all
	 * necessary details about every thread.
	 */
	static ConcurrentHashMap<Long, ProcessPacket> runningProcesses = 
			new ConcurrentHashMap<Long, ProcessPacket>();
	
	static ConcurrentHashMap<Integer, NodeInfo> nodeList =
			new ConcurrentHashMap<Integer, NodeInfo>();
	
	/**
	 *  The starting process ID number.
	 */
	private static long start_pid = 0;
	
	/**
	 * Lists all the processes running on the current node. 
	 * Prints in the following format: PID - PROCESSNAME(ARGUMENTS)
	 */
	public static void psInvoke() {
		if(!runningProcesses.isEmpty())
			System.out.println("PID-NAME(ARGS)");
		
		Enumeration<ProcessPacket> items = runningProcesses.elements();
		while (items.hasMoreElements()) {
			ProcessPacket t = items.nextElement();
			System.out.println(t.pid + " - " + t.name);
		}
	}
	
	/**
	 * Generates a new PID whenever a new thread is spawn.
	 * 
	 * @return The PID to be assigned to the new thread.
	 */
	synchronized static long getPid() {
		return ++start_pid;
	}
	
	/**
	 * Makes a new object that contains all the necessary details about a
	 * process running on a node. Holds the PID, the object that is being
	 * run, the process name and it's arguments, and the thread being
	 * executed.
	 * 
	 * @param obj  The MigratableProcess that is being executed
	 * @param pid  The PID of the process
	 * @param name The process name and it's arguments.
	 * @param t    The thread that is being executed.
	 * @return     The object holding all the above information.
	 */
	private static ProcessPacket makeProcessPacket(MigratableProcess obj, 
										long pid, String name, Thread t) {
		ProcessPacket p = new ProcessPacket(obj, pid, name, t);
		return p;
	}
	
	/**
	 * Adds the ProcessPacket to the runningProcesses Hashtable.
	 * 
	 * @param obj  The MigratableProcess being run.
	 * @param pid  The process' PID.
	 * @param name The process name and it's arguments.
	 */
	private static void addToHash(MigratableProcess obj,long pid, String name) {
		Thread t = new Thread(obj);
		ProcessPacket process = makeProcessPacket(obj, pid, name, t);
		runningProcesses.put(pid, process);
		t.start();
	}
	
	/**
	 * Starts a new thread of a class that implements MigratableProcess.
	 * 
	 * @param args The class' name and it's arguments.
	 */
	public static void launchInvoke(String[] args) {
		String className = args[0];
		String[] arguments = Arrays.copyOfRange(args, 1, args.length);
		boolean argFlag = false;
		if(args.length > 1)
			argFlag = true;
		
		StringBuffer str = new StringBuffer();
		str.append(className + "(");
		for (String s:arguments) {
			str.append(s + ", ");
		}
		
		if(argFlag)
			str.setLength(str.length() - 2);
		str.append(")");
		String name = str.toString();
			
		try {
			Long pid;
		
			Class<?> classObj = Class.forName(className);
			Object o = new Object(); 
			o = arguments;
			Constructor<?> cons = classObj.getConstructor(String[].class);
			MigratableProcess obj = (MigratableProcess)cons.newInstance(o);
			
			pid = getPid();
			addToHash(obj, pid, name);
			
		} catch (ClassNotFoundException e) {
			System.out.println("The given process(class) is not found.");
		} catch (NoSuchMethodException e) {
			System.out.println("Method missing. Is your constructor correct?");
		} catch (InvocationTargetException e) {
			System.out.println("InvocationTargetException");
		} catch (InstantiationException e) {
			System.out.println("InstantiationException");
		} catch (IllegalAccessException e) {
			System.out.println("IllegalAccessException");
		} catch (ClassCastException e) {
			System.out.println("The given class doesn't confirm to "
					+ "MigratableProcess interface.");
		}
	}
	
	/**
	 * Serializes the object of the process that is requested for migration.
	 * 
	 * @param pid The PID of the requested process.
	 * @param ip  The Hostname of the destination node.
	 */
	private static void serialize(Long pid, String ip) {
		ProcessPacket p = runningProcesses.get(pid);
		p.mp.suspend();
		
		 File theDir = new File("migrate");

		 if (!theDir.exists()) {
		    try{
		        theDir.mkdir();
		     } catch(SecurityException se){
		        System.out.println("Don't have write permisssions.");
		     }        
		 }
		 
		ip = ip.substring(1);
		
		String fileName = "migrate/process_" + pid + ip + ".ser";
		
		try {
			FileOutputStream file = new FileOutputStream(fileName);
	        ObjectOutputStream out;
			out = new ObjectOutputStream(file);
			out.writeObject(p);
			out.close();
	        file.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * If the destination node is alive, migrate the process to it.
	 * 
	 * @param pid  The PID of the process to be transferred.
	 * @param ip   The hostname of the destination node.
	 * @param port The listen port of the destination port.
	 * @throws UnknownHostException
	 * @throws IOException
	 */
	@SuppressWarnings("deprecation")
	public static void nextStep(Long pid, String ip, int port) {
		try {
			Socket client = new Socket(ip, port);
			OutputStream outToServer = client.getOutputStream();
	         DataOutputStream out =
	                       new DataOutputStream(outToServer);
			
	   	 	serialize(pid, client.getLocalAddress().toString());
	
	   	 	String ip_send = client.getLocalAddress().toString().substring(1) ;
	   	 	out.writeUTF("pid:" + pid + ip_send);
	        ProcessPacket p = runningProcesses.get(pid);
	        p.t.stop();
	        runningProcesses.remove(pid);
	        out.close();
	        client.close();
		} catch (ConnectException e) {
			System.out.println("Destination node not configured properly");
		} catch (UnknownHostException e) {
			System.out.println("Destination node not configured properly");
		}  catch (IOException e) {
		 e	.printStackTrace();
		} 
    }
	
	/**
	 * The method to migrate from one node to another depending on the user's
	 * need.
	 * 
	 * @param pid  The PID of the process to migrate.
	 * @param ip   The hostname of the destination node.
	 * @param port The listen port number of the destination node.
	 */
	public static void migrateInvoke(Long pid, String ip, int port) {
		try {
			Socket client = new Socket(ip, port);
			OutputStream outToServer = client.getOutputStream();
	         DataOutputStream out =
	                       new DataOutputStream(outToServer);
	         out.writeUTF("migrate");
	         InputStream inFromServer = client.getInputStream();
	         DataInputStream in =
	                        new DataInputStream(inFromServer);
	         if(in.readUTF().equals("1")) {
	        	 out.close();
	        	 in.close();
	        	 client.close();
	        	 nextStep(pid, ip, port);
	         }
	         else
	        	 System.out.println("Destination node not configured properly");
			 } catch (ConnectException e) {
				 System.out.println("Destination node not configured properly");
			 } catch (UnknownHostException e) {
				 System.out.println("Destination node not configured properly");
			 }  catch (IOException e) {
				 e.printStackTrace();
			 } 
	}
	
	private static void talkToMaster(String ip, int port) {
		try {
			Socket client = new Socket(ip, port);
			OutputStream outToServer = client.getOutputStream();
	         DataOutputStream out =
	                       new DataOutputStream(outToServer);
	         out.writeUTF("addme");
	         InputStream inFromServer = client.getInputStream();
	         DataInputStream in =
	                        new DataInputStream(inFromServer);
	         if(in.readUTF().equals("1")) {
	        	 out.close();
	        	 in.close();
	        	 client.close();
	         }
	         else
	        	 System.out.println("Destination node not configured properly. "
	        	 		+ "Setup master first.");
	         	System.exit(9);
			 } catch (ConnectException e) {
				 System.out.println("Destination node not configured properly. "
				 		+ "Setup master first.");
				 System.exit(9);
			 } catch (UnknownHostException e) {
				 System.out.println("Destination node not configured properly. "
				 		+ "Setup master first.");
				 System.exit(9);
			 }  catch (IOException e) {
				 e.printStackTrace();
				 System.exit(9);
			 } 
	}
	
	/**
	 * Can kill threads that are specified by their PID.
	 * 
	 * @param pid The PID of the process to kill.
	 */
	@SuppressWarnings("deprecation")
	public static void killInvoke(Long pid) {
		ProcessPacket p = runningProcesses.get(pid);
		if(p != null) {
			p.mp.suspend();
			runningProcesses.remove(pid);
			p.t.stop();
		}
		else {
			System.out.println("Missing PID. Use \"ps\" to get correct values");
		}
		
	}
	
	/**
	 * Sets up the thread to listen to other nodes for communication.
	 * 
	 * @param port The port number to listen to.
	 */
	private static void setupListen(int port) {
		Listen l = new Listen(port);
		Thread t = new Thread(l);
		t.start();
	}
	
	/**
	 * Starts a thread to continuously poll the running threads to see if they
	 * are still alive.
	 */
	private static void setupThreadPoll() {
		Poll p = new Poll();
		Thread t = new Thread(p);
		t.start();
	}
	
	private static void printHelp() {
		System.out.println("Commands available: ");
		System.out.println("\"ps\" To print the currently running processes on"
				+ " this node.");
		System.out.println("\"launch\" To start a new process."
				+ "\nUsage: launch ProcessName Arguments");
		System.out.println("\"migrate\" To migrate the process to a new "
				+ "node. \nUsage: migrate PID NewHostName "
				+ "NewPortNumber");
		System.out.println("\"kill\" To kill a process."
				+ "\nUsage: kill PID");
		System.out.println("\"quit\" To exit the ProcessManager.");
	}
	
	private static void nsInvoke() {
		if(!nodeList.isEmpty())
			System.out.println("Node ID - IP - PORT - HOSTNAME");
		
		Enumeration<NodeInfo> items = nodeList.elements();
		while (items.hasMoreElements()) {
			NodeInfo t = items.nextElement();
			System.out.println(t.nid + " - " + t.ip + " - " + t.port
					+ " - " + t.hostname);
		}
	}
	
	@SuppressWarnings("unchecked")
	private static void getNodeListFromMaster(String ip, int port) {
		ConcurrentHashMap<Integer, NodeInfo> ret = null;

		try {
			Socket client = new Socket(ip, port);
			OutputStream outToServer = client.getOutputStream();
	         DataOutputStream out =
	                       new DataOutputStream(outToServer);
	         out.writeUTF("nodelist");
	         InputStream inFromServer = client.getInputStream();
	         ObjectInputStream in =
	                        new ObjectInputStream(inFromServer);
	         try {
				ret = (ConcurrentHashMap<Integer, NodeInfo>)in.readObject();
			} catch (ClassNotFoundException e) {
				e.printStackTrace();
			}
	         nodeList.clear();
	 		 nodeList = ret;
	 		 client.close();
		} catch (ConnectException e) {
			 System.out.println("Destination node not configured properly");
		} catch (UnknownHostException e) {
			 System.out.println("Destination node not configured properly");
		}  catch (IOException e) {
			 e.printStackTrace();
		} 
	}
		
	/**
	 * Provides an interface with the user to list the running processes,
	 * start new processes, kill processes, migrate processes and finally
	 * exit the program. 
	 * 
	 * @param args Contains the port number to listen on.
	 */
	@SuppressWarnings("deprecation")
	private static void prompt(String[] args, boolean master) {

		int port;
		boolean loop = true;
		Parser p = new Parser();
		Scanner in = new Scanner(System.in);
		String[] s;
		String masterIP;
		int masterPort;
		
		if(master)
			setupSlavePoll();
		
		setupThreadPoll();
			
		port = p.parseport(args, master);
		setupListen(port);
		
		if(!master) {
			masterIP = p.parseip(args);
			masterPort = p.parseport(args, master);
			talkToMaster(masterIP, masterPort);
		}
		
		System.out.println("Type \"help\" to find out how to use the program");
		String selection;
		while(loop) {
			System.out.print("> ");
			selection = in.nextLine();
			s = selection.split(" ");
			
			if(!s[0].equalsIgnoreCase("")) {
				if (selection.equalsIgnoreCase("ps")) {
					psInvoke();
				}
				
				else if (selection.equalsIgnoreCase("ns")) {
					if (master) {
						nsInvoke();
					}
					else {
						getNodeListFromMaster(masterIP, masterPort);
						nsInvoke();
					}
				}
								
				else if (s[0].equalsIgnoreCase("launch")) {
					if (s.length > 1) {
						launchInvoke(Arrays.copyOfRange(s, 1, s.length));
					}
					else {
						System.out.println("Wrong input. Try again.");
						System.out.println("Format required: launch "
										+ "ProcessName Arg1 Arg2 ... ArgN");
					}
				}
				
				else if (s[0].equalsIgnoreCase("migrate")) {
					
					if (s.length == 4) {
						try {
							migrateInvoke(Long.parseLong(s[1]), s[2], 
										Integer.parseInt(s[3]));
						} catch (NumberFormatException e) {
							System.out.println("Must type in a number.");
						}
					}
					else {
						System.out.println("Wrong input. Try again.");
						System.out.println("Format required: migrate PID "
												+ "NewHostName NewPortNumber");
					}
				}
				
				else if (s[0].equalsIgnoreCase("kill")) {
					if (s.length == 2) {
						try {
							killInvoke(Long.parseLong(s[1]));
						} catch (NumberFormatException e) {
							System.out.println("Must type in a number.");
						}
					}
					else {
						System.out.println("Wrong input. Try again.");
						System.out.println("Format required: kill PID");
					}
						
				}
				
				else if (s[0].equalsIgnoreCase("quit")) {
					for (Thread t : Thread.getAllStackTraces().keySet()) {  
						if (t.getState()==Thread.State.RUNNABLE) 
					     t.stop(); 
					} 
					System.exit(0);
				}
				
				else if (s[0].equalsIgnoreCase("help")) {
					printHelp();
				}
				
				else {
					printHelp();
				}
				System.out.printf("");
			}
		} 
		in.close();	
	}
	
	private static void setupSlavePoll() {
		
	}

	private static void printStartHelp() {
		System.out.println("Wrong command line arguments");
		System.out.println("Usage: \nProcessManager -p ListenPortNumber.");
		System.exit(1);
	}	
	
	/**
	 * The main method. Checks if the count of the arguments are correct
	 * and passes them to the prompt loop.
	 * 
	 * @param args Contains the port number of the node to listen on.
	 */
	public static void main(String[] args) {
		boolean flag = false;
		boolean master = false;
		
		if (!(args.length ==2)) {
			printStartHelp();
		}
		
		for(String s:args) {
			if(s.equalsIgnoreCase("-p")) {
				flag = true;
			}
			else if(s.equalsIgnoreCase("-m")) {
				master = true;
			}
		}
		
		if(!flag) {
			printStartHelp();
		}

		prompt(args, master);
	}	
	
}
