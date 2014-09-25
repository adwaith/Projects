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
import java.net.InetAddress;
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
	
	static ConcurrentHashMap<Long, NodeInfo> nodeList =
			new ConcurrentHashMap<Long, NodeInfo>();
	
	static ConcurrentHashMap<Long, ProcessPacket> tempProcesses = 
			new ConcurrentHashMap<Long, ProcessPacket>();
	
	static long myNid;
	
	/**
	 *  The starting process ID number.
	 */
	private static long start_pid = 0;
	
	/**
	 * The starting node ID number. 
	 */
	private static long start_nid = 0;
	
	/**
	 * The port which the master listens.
	 */
	private static int masterPort = 15000;
	
	/**
	 * The hostname of the master.
	 */
	private static String masterIP = "";
	
	/**
	 * Lists all the processes running on the current node. 
	 * Prints in the following format: PID - PROCESSNAME(ARGUMENTS)
	 */
	private static void psInvoke(ConcurrentHashMap<Long, ProcessPacket> temp) {
		if(!temp.isEmpty())
			System.out.println("PID-NAME(ARGS)");
		
		Enumeration<ProcessPacket> items = temp.elements();
		while (items.hasMoreElements()) {
			ProcessPacket t = items.nextElement();
			System.out.println(t.pid + " - " + t.name);
		}
	}
	
	/**
	 * Gets the ps for all the nodes.
	 */
	@SuppressWarnings("unchecked")
	private static void psAllInvoke() {
		ConcurrentHashMap<Long, ProcessPacket> ret = null;
		
		Enumeration<NodeInfo> items = nodeList.elements();
		while (items.hasMoreElements()) {
			NodeInfo n = items.nextElement();
			if(n.nid == myNid)
				continue;
			try {
				Socket client = new Socket(n.hostname, n.port);
				OutputStream outToServer = client.getOutputStream();
		        DataOutputStream out =
		                       new DataOutputStream(outToServer);
		        out.writeUTF("ps");
		        InputStream inFromServer = client.getInputStream();
		        ObjectInputStream in =
		                        new ObjectInputStream(inFromServer);
		        try {
					ret = (ConcurrentHashMap<Long, ProcessPacket>)in.readObject();
				} catch (ClassNotFoundException e) {
					e.printStackTrace();
				}
		        if(!ret.isEmpty())
		        	System.out.println("Hostname: " + n.hostname + " NodeID: "
		        			+ n.nid);
		        psInvoke(ret);
		 		client.close();
		 		ret.clear();
		 		if(!ret.isEmpty())
		 			System.out.println("");
			} catch (ConnectException e) {
			} catch (UnknownHostException e) {
			} catch (IOException e) {
			}
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
	 * Returns a new NID whenever a new slave joins.
	 * 
	 * @return The NID to be assigned to the new slave.
	 */
	synchronized static long getNid() {
		return ++start_nid;
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
	 * Adds the ProcessPacket to the runningProcesses HashMap.
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
	public static String launchInvoke(String[] args) {
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
			Long pid = getPidFromMaster();
		
			Class<?> classObj = Class.forName(className);
			Object o = new Object(); 
			o = arguments;
			Constructor<?> cons = classObj.getConstructor(String[].class);
			MigratableProcess obj = (MigratableProcess)cons.newInstance(o);
			
			addToHash(obj, pid, name);
			
		} catch (ClassNotFoundException e) {
			return "The given process(class) is not found.";
		} catch (NoSuchMethodException e) {
			return "Method missing. Is your constructor correct?";
		} catch (InvocationTargetException e) {
			return "InvocationTargetException";
		} catch (InstantiationException e) {
			return "InstantiationException";
		} catch (IllegalAccessException e) {
			System.out.println("IllegalAccessException");
		} catch (ClassCastException e) {
			return "The given class doesn't confirm to "
					+ "MigratableProcess interface.";
		}
		return "Works";
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
	public static void nextStep(Long pid, Long nid) {
		ProcessPacket temp = runningProcesses.get(pid);
		
		if(temp != null) {
			NodeInfo n = nodeList.get(nid);
			if(n == null)
				System.out.println("Node doesn't exist");
			else {
			
				try {
					Socket client = new Socket(n.hostname, n.port);
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
		}
		else
			System.out.println("Pid doesn't exist.");
    }
	

	/**
	 * When a slave spawns a new thread, it gets a new PID.
	 * 
	 * @return The PID of the new thread.
	 */
	private static long getPidFromMaster() {
		long a = 0;
		try {
			Socket client = new Socket(masterIP, masterPort);
			OutputStream outToServer = client.getOutputStream();
	         DataOutputStream out =
	                       new DataOutputStream(outToServer);
	         out.writeUTF("getpid");
	         InputStream inFromServer = client.getInputStream();
	         DataInputStream in =
	                        new DataInputStream(inFromServer);
	         a = in.readLong();
	         out.close();
	         in.close();
	         client.close();

			 } catch (ConnectException e) {
				 System.out.println("Destination node not configured properly");
			 } catch (UnknownHostException e) {
				 System.out.println("Destination node not configured properly");
			 }  catch (IOException e) {
				 e.printStackTrace();
			 }
		return a;
	}
	
	/**
	 * When a new slave is set up, it asks the master to add it to the nodeList.
	 * 
	 * @param ip   The hostname of the master
	 * @param port The port number of the master
	 */
	private static void talkToMaster(String ip, int port) {
		try {
			Socket client = new Socket(ip, masterPort);
			OutputStream outToServer = client.getOutputStream();
	         DataOutputStream out =
	                       new DataOutputStream(outToServer);
	         client.getLocalAddress();
			String s = "addme" + ":" + client.getLocalAddress().toString() + 
					":" + port + ":" + InetAddress.getLocalHost().getHostName();
	         out.writeUTF(s);
	         InputStream inFromServer = client.getInputStream();
	         DataInputStream in =
	                        new DataInputStream(inFromServer);
	         myNid = in.readLong();
        	 out.close();
        	 in.close();
        	 client.close();
	         
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
	
	/**
	 * The command line help.
	 */
	private static void printHelp() {
		System.out.println("Commands available: ");
		System.out.println("\"ps\" To print the currently running processes on"
				+ " this node.");
		System.out.println("\"launch\" To start a new process."
				+ "\nUsage: launch ProcessName Arguments");
		System.out.println("\"migrate\" To migrate the process to a new "
				+ "node. \nUsage: migrate PID NewNodeID "
				+ "NewPortNumber");
		System.out.println("\"launchremote\" To start a new process on a "
				+ "remote node. \nUsage: launchremote ProcessName Arguments "
				+ "NodeID");
		System.out.println("\"migrateremote\" To migrate a process from a "
				+ "remote node to a new node. \nUsgae: migrateremote PID "
				+ "NodeIDWhereProcessCurrentlyIs DestinationNodeID");
		System.out.println("\"kill\" To kill a process."
				+ "\nUsage: kill PID");
		System.out.println("\"quit\" To exit the ProcessManager.");
	}
	
	/**
	 * Prints the list of nodes in the system.
	 */
	static void nsInvoke() {
		if(!nodeList.isEmpty())
			System.out.println("Node ID - PORT - HOSTNAME");
		else
			System.out.println("Empty");
		
		Enumeration<NodeInfo> items = nodeList.elements();
		while (items.hasMoreElements()) {
			NodeInfo t = items.nextElement();
				System.out.printf(t.nid + " - " + t.port + " - " +
					t.hostname);
			
			if(t.nid == 1)
				 System.out.printf(" ----- Master");
			if(t.nid == myNid)
				System.out.printf(" ----- Current");
			System.out.println("");
		}
	}
	
	/**
	 * Gets an updated nodeList from the master.
	 */
	@SuppressWarnings("unchecked")
	static void getNodeListFromMaster() {
		ConcurrentHashMap<Long, NodeInfo> ret = null;

		try {
			Socket client = new Socket(masterIP, masterPort);
			OutputStream outToServer = client.getOutputStream();
	        DataOutputStream out =
	                       new DataOutputStream(outToServer);
	        out.writeUTF("nodelist");
	        InputStream inFromServer = client.getInputStream();
	        ObjectInputStream in =
	                        new ObjectInputStream(inFromServer);
	        try {
				ret = (ConcurrentHashMap<Long, NodeInfo>)in.readObject();
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
	 * Adds the slave to the node list.
	 * @param ip       The IP of the slave
	 * @param port     The port of the slave
	 * @param hostname The hostname of the slave
	 * @return         The NodeID of the slave.
	 */
	static long addToNodeList(String ip, int port, String hostname) {
		long nid = getNid();
		NodeInfo n = new NodeInfo(ip, port, hostname, nid);
		nodeList.put(nid, n);
		return nid;
	}
	
	/**
	 * Starts the migrate procedure from a remote location.
	 * 
	 * @param pid  The ProcessID to be migrated.
	 * @param nid1 The origin NodeID.
	 * @param nid2 The destination NodeID.
	 */
	static void migrateRemoteInvoke(Long pid, Long nid1, Long nid2) {
		NodeInfo n1 = nodeList.get(nid1);
		NodeInfo n2 = nodeList.get(nid2);
		
		if(n1 == null || n2 == null)
			System.out.println("Node doesn't exist. Try again.");
		else {
			try {
				Socket client = new Socket(n1.hostname, n1.port);
				OutputStream outToServer = client.getOutputStream();
		        DataOutputStream out =
		                       new DataOutputStream(outToServer);
		        String s = "remotemigrate" + ":" + pid + ":" + nid2;
		        out.writeUTF(s);
		        
		        
		        InputStream inFromServer = client.getInputStream();
		         DataInputStream in =
		                        new DataInputStream(inFromServer);
		         if(in.readUTF().equals("no"))
		        	 System.out.println("PID doesn't exist.");       
		        
		         out.close();
		 		client.close();
			} catch (ConnectException e) {
				 System.out.println("Destination node not configured properly");
			} catch (UnknownHostException e) {
				 System.out.println("Destination node not configured properly");
			}  catch (IOException e) {
				 e.printStackTrace();
			} 
		}
		
	}
	
	/**
	 * Starts the launch process from a remote location.
	 * 
	 * @param args The String[] containing the classname and arguments.
	 */
	static void launchRemoteInvoke(String[] args) {
		Long id = Long.parseLong(args[args.length - 1]);
		NodeInfo n = nodeList.get(id);
		
		StringBuilder str = new StringBuilder();
		
		for(String a:args) {
			str.append(a + ":");
		}
		
		str.setLength(str.length() - 2);
		
		String temp = str.toString();
		
		if(n == null)
			System.out.println("Node doesn't exist. Try again.");
		else {
			try {
				Socket client = new Socket(n.hostname, n.port);
				OutputStream outToServer = client.getOutputStream();
		        DataOutputStream out =
		                       new DataOutputStream(outToServer);
		        String s = "launch" + ":" + temp;
		        out.writeUTF(s);
		        
		        InputStream inFromServer = client.getInputStream();
		         DataInputStream in =
		                        new DataInputStream(inFromServer);
		         String value = in.readUTF(); 
		         if(!value.equals("Works"))
		        	 System.out.println(value);
		        
		         out.close();
		 		client.close();
			} catch (ConnectException e) {
				 System.out.println("Destination node not configured properly");
			} catch (UnknownHostException e) {
				 System.out.println("Destination node not configured properly");
			}  catch (IOException e) {
				 e.printStackTrace();
			} 
		}
	}
	
	/**
	 * Sends the quit message to the slaves.
	 */
	private static void sendQuitToSlaves() {
		if(!nodeList.isEmpty()) {
			Enumeration<NodeInfo> items = nodeList.elements();
			while (items.hasMoreElements()) {
				NodeInfo n = items.nextElement();
				if(n.nid == 1)
					continue;
				try {
					Socket client = new Socket(n.hostname, n.port);
					OutputStream outToServer = client.getOutputStream();
			         DataOutputStream out =
			                       new DataOutputStream(outToServer);
			         out.writeUTF("die");
			         InputStream inFromServer = client.getInputStream();
			         DataInputStream in =
			                        new DataInputStream(inFromServer);
			         if(!in.readUTF().equals("ok")) {
			        	 ProcessManager.nodeList.remove(n.nid);
			         }
			         
			         out.close();
		        	 in.close();
		        	 client.close();

				} catch (ConnectException e) {
					ProcessManager.nodeList.remove(n.nid);
				} catch (UnknownHostException e) {
					 ProcessManager.nodeList.remove(n.nid);
				}  catch (IOException e) {
					 ProcessManager.nodeList.remove(n.nid);
				} 
			}
		}
	}
		
	/**
	 * Provides an interface with the user to list the running processes,
	 * start new processes, kill processes, migrate processes and finally
	 * exit the program. 
	 * 
	 * @param args Contains the port number to listen on.
	 */
	private static void prompt(String[] args, boolean master) {

		int port = 0;
		boolean loop = true;
		Parser p = new Parser();
		Scanner in = new Scanner(System.in);
		String[] s;
		
		setupThreadPoll();
		
		if(!master) {
			port = p.parseport(args);
			masterIP = p.parseHn(args);
			talkToMaster(masterIP, port);
		}
		
		if(master) {
			setupSlavePoll();
			setupListen(masterPort);
			String host = "";
			try {
				host = InetAddress.getLocalHost().getHostName();
			} catch (UnknownHostException e) {
				e.printStackTrace();
			}
			myNid = addToNodeList(masterIP, masterPort, host);
		}
		else
			setupListen(port);
		
		System.out.println("Type \"help\" to find out how to use the program");
		String selection;
		while(loop) {
			System.out.print("> ");
			selection = in.nextLine();
			s = selection.split(" ");
			
			if(!s[0].equalsIgnoreCase("")) {
				if (selection.equalsIgnoreCase("ps")) {
					psInvoke(runningProcesses);
				}
				
				else if (selection.equalsIgnoreCase("psall")) {
					getNodeListFromMaster();
					psAllInvoke();
				}
				
				else if (selection.equalsIgnoreCase("ns")) {
					if(master)
						nsInvoke();
					else{
						getNodeListFromMaster();
						nsInvoke();
					}
						
				}
				
				else if (s[0].equalsIgnoreCase("launch")) {
					if (s.length > 1) {
						String ret = launchInvoke(Arrays.copyOfRange(s, 1, 
													s.length));
						if(!ret.equals("Works"))
							System.out.println(ret);
					}
					else {
						System.out.println("Wrong input. Try again.");
						System.out.println("Format required: launch "
										+ "ProcessName Arg1 Arg2 ... ArgN");
					}
				}
				
				else if (s[0].equalsIgnoreCase("launchremote")) {
					getNodeListFromMaster();
					if (s.length > 1) {
						launchRemoteInvoke(Arrays.copyOfRange(s, 1, s.length));
					}
					else {
						System.out.println("Wrong input. Try again.");
						System.out.println("Format required: launchremote "
										+ "ProcessName Arg1 Arg2 ... ArgN +"
										+ "NodeNumber");
					}
				}
				
				else if (s[0].equalsIgnoreCase("migrate")) {
					getNodeListFromMaster();
					if (s.length == 3) {
						if(myNid == Long.parseLong(s[2]))
							System.out.println("Can't migrate to itself");
						else {
							try {
								nextStep(Long.parseLong(s[1]),
										Long.parseLong(s[2]));
							} catch (NumberFormatException e) {
								System.out.println("Must type in a number.");
							}
						}
					}
					else {
						System.out.println("Wrong input. Try again.");
						System.out.println("Format required: migrate PID "
												+ "NodeID");
					}
				}
				
				else if (s[0].equalsIgnoreCase("migrateremote")) {
					getNodeListFromMaster();
					if (s.length == 4) {
						if(Long.parseLong(s[2]) != Long.parseLong(s[3])) {
							try {
								migrateRemoteInvoke(Long.parseLong(s[1]), 
										Long.parseLong(s[2]), Long.parseLong(s[3]));
							} catch (NumberFormatException e) {
								System.out.println("Must type in a number for"
										+ " the NodeID.");
							}
						}
						else {
							System.out.println("Cant migrate it to itself.");
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
					getNodeListFromMaster();
					if(master)
						sendQuitToSlaves();
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
		SlavePoll p = new SlavePoll();
		Thread t = new Thread(p);
		t.start();
	}

	private static void printStartHelp() {
		System.out.println("Wrong command line arguments");
		System.out.println("Usage: \nProcessManager -m For Master mode.");
		System.out.println("ProcessManager -p ListenPortNumber -hn "
							+ "MasterHostName.");
		System.exit(1);
	}	
	
	/**
	 * The main method. Checks if the count of the arguments are correct
	 * and passes them to the prompt loop.
	 * 
	 * @param args Contains the port number of the node to listen on.
	 */
	public static void main(String[] args) {
		boolean first = false, second = false;
		boolean master = false;
		if (!(args.length == 1 || args.length == 4)) {
			printStartHelp();
		}
		
		for(String s:args) {
			if(s.equalsIgnoreCase("-m")) {
				master = true;
				break;
			}
		}
		
		if(!master) {
			for(String s:args) {
				if(s.equalsIgnoreCase("-p")) {
					first = true;
				}
				else if(s.equalsIgnoreCase("-hn")) {
					second = true;
				}
			}
		}
		
		if((first && second) || master)
			prompt(args, master);
		else
			printStartHelp();
	}	
	
}
