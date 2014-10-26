import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.OutputStream;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Proxy;
import java.net.ConnectException;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Scanner;
import java.util.LinkedList;

/**
 * The class that the user uses to invoke remote methods. 
 * The available methods are: add, sub, mul, div, upper and lower.
 *
 */
public class Client {
    
    /**
     * An enumeration object for the calculator methods.
     *
     */
    public enum Calc {
        ADD, SUB, MUL, DIV
    }
    
    /**
     * An enumeration object for the case methods.
     *
     */
    public enum Case {
        UPPER, LOWER
    }
    
    /**
     * A linked list containing the HostName and PortNumber of all the
     * servers on the system.
     */
    static LinkedList<RegPacket> regList = new LinkedList<RegPacket>();
    
	/**
	 * Returns a proxy object that is used as a stub to call the remote 
	 * methods.
	 * @param name The name of the class to create a proxy object for.
	 * @param res  The remote object reference of the given class.
	 * @return     The proxy object for the requested class.
	 */
	private static Object getProxyObject(String name, 
	                            RemoteObjectReference res) {
	    
	    InvocationHandler handler = new RemoteHandler(res.ip, res.port);

        Class<?> className;
        Object proxyObject = null;
        try {
            className = Class.forName(name);
            proxyObject =  Proxy.newProxyInstance(
                    className.getClassLoader(),
                    new Class[]{className},
                    handler);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }

        return proxyObject;
	}
	
	/**
	 * A method to invoke any of the Calc class' methods.
	 * @param nums   An array containing the numbers on which operation is 
	 *               performed.
	 * @param res    The remote object reference of the Calc class.
	 * @param method The method to be performed on the numbers
	 * @return       The result of the method operation.
	 */
	private static double callCalc(String[] nums, RemoteObjectReference res, 
	                                                    Calc method){
	    
        CalcInterface remoteadd = 
                (CalcInterface)getProxyObject(res.classInterface, res);
        double ret = 0;
        double arg1 = 0, arg2 = 0;
        
        try {
            arg1 = Double.parseDouble(nums[0]);
            arg2 = Double.parseDouble(nums[1]);
        } catch (NumberFormatException e) {
            System.out.println("Enter valid numbers");
        }
        
        switch(method) {
            case ADD:
                ret = remoteadd.add(arg1, arg2);
                break;
            case SUB:
                ret = remoteadd.sub(arg1, arg2);
                break;
            case MUL:
                ret = remoteadd.mul(arg1, arg2);
                break;
            case DIV:
                ret = remoteadd.div(arg1, arg2);
                break;
            default:
                System.out.println("Wrong method");
        }
        
        return ret;
	}
	
	/**
	 * A method to invoke any of the Case class' methods.
	 * @param str     The string on which the operation is performed
	 * @param res     The remote object reference of the Case class
	 * @param method  The method to be performed on the string
	 * @return        The result of the method operation.
	 */
	private static String callCase(String str, RemoteObjectReference res, 
	                                                            Case method) {
	    CaseInterface remotecase = 
	            (CaseInterface)getProxyObject(res.classInterface, res);
	    String ret = null;
	    
	    switch(method) {
	        case UPPER:
	            ret = remotecase.upper(str);
	            break;
	        case LOWER:
	            ret = remotecase.lower(str);
	            break;
	        default:
                System.out.println("Wrong method");
	    }
	    
	    return ret;
	}
	
	/**
	 * Check if the requested method is present on the server
	 * @param ip         The HostName of the server
	 * @param port       The PortNumber of the server
	 * @param selection  The method to be invoked.
	 * @return           The remote object reference of the class that is 
	 *                   needed.
	 */
	private static RemoteObjectReference callLookUp(String ip, int port, 
	                                                    String selection) {
	    RemoteObjectReference ret = null;
	    try {
	        
            Socket client = new Socket(ip, port);
            OutputStream outToServer = client.getOutputStream();
            DataOutputStream out =
                           new DataOutputStream(outToServer);
            String s = "lookup" + ":" + selection;
            out.writeUTF(s);
            
            InputStream inFromServer = client.getInputStream();
            ObjectInputStream inServer =
                    new ObjectInputStream(inFromServer);

            ret = (RemoteObjectReference)inServer.readObject();
            inServer.close();
            client.close();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (ConnectException e) {
            System.out.println("Destination node not configured properly");
            System.exit(1);
        } catch (UnknownHostException e) {
            System.out.println("Destination node not configured properly");
            System.exit(1);
        } catch (IOException e) {
            e.printStackTrace();
        }   
	    return ret;
	}

	/**
	 * Query all the servers on the system to see if the requested method is
	 * present
	 * 
	 * @param  selection The requested method
	 * @return The remote object reference of the class containing the method.
	 */
	private static RemoteObjectReference lookUpMethod(String selection) {
	    RemoteObjectReference ret = null;
	    for(RegPacket r:regList) {
	        ret = callLookUp(r.ip, r.port, selection);
	        if(ret != null) {
	            return ret;
	        }
	    }
	    
	    return null;
	}
	
	/**
	 * List of all the methods available to the user.
	 */
	private static void printHelp() {
	    System.out.println("The available remote methods are: ");
	    System.out.println("Add 2 numbers\nUsage: > add Number1 Number2");
	    System.out.println("Subtract 2 numbers\nUsage: > sub Number1 Number2");
	    System.out.println("Multiply 2 numbers\nUsage: > mul Number1 Number2");
	    System.out.println("Divide 2 numbers\nUsage: > div Number1 Number2");
	    System.out.println("");
	    System.out.println("Convert string to upper case\n"
	            + "Usage: > upper StringToConvert");
	    System.out.println("Convert string to lower case\n"
                + "Usage: > lower StringToConvert");
	    System.out.println("");
	    System.out.println("\"quit\" to exit the program");
	}
	
	/**
	 * The interface the user uses to invoke the remote methods.
	 */
	private static void prompt(){
		boolean prompt_loop = true;
		String selection;
		String[] broken;
		RemoteObjectReference res;
		
		Scanner in = new Scanner(System.in);
		
		System.out.println("Type \"help\" to find the available methods");
		
		while(prompt_loop) {
			System.out.print("> ");
			selection = in.nextLine();
			broken = selection.split(" ");

			if(!broken[0].equalsIgnoreCase("")) {
			    res = lookUpMethod(broken[0]);
			    
    			if(broken[0].equalsIgnoreCase("add")) {
    			    if(broken.length != 3)
    			        System.out.println("Give the correct number of "
    			                + "arguments");
    			    else
    			        System.out.println(callCalc(Arrays.copyOfRange(broken, 
    			                1, broken.length), res, Calc.ADD));
    			} else if(broken[0].equalsIgnoreCase("sub")) {
    			    if(broken.length != 3)
                        System.out.println("Give the correct number of "
                                + "arguments");
                    else
                        System.out.println(callCalc(Arrays.copyOfRange(broken, 
                                1, broken.length), res, Calc.SUB));
    			} else if(broken[0].equalsIgnoreCase("mul")) {
    			    if(broken.length != 3)
                        System.out.println("Give the correct number of "
                                + "arguments");
                    else
                        System.out.println(callCalc(Arrays.copyOfRange(broken, 
                                1, broken.length), res, Calc.MUL));
                } else if(broken[0].equalsIgnoreCase("div")) {
                    if(broken.length != 3)
                        System.out.println("Give the correct number of "
                                + "arguments");
                    else
                        System.out.println(callCalc(Arrays.copyOfRange(broken, 
                                1, broken.length), res, Calc.DIV));
                } else if(broken[0].equalsIgnoreCase("upper")) {
                        System.out.println(callCase(selection.substring(6), 
                                res, Case.UPPER));
                } else if(broken[0].equalsIgnoreCase("lower")) {
                        System.out.println(callCase(selection.substring(6), 
                                res, Case.LOWER));
                } else if(broken[0].equalsIgnoreCase("quit")) {
    				System.exit(0);
    			} else if(broken[0].equalsIgnoreCase("help")) {
    			    printHelp();
                } else {
    			    printHelp();
    			}
    			System.out.printf("");
			}
		}
		in.close();
	}
	
	/**
	 * Adds the HostName and Port into the registry list.
	 * @param args
	 */
	static void addToRegList(String[] args) {
	    try {
	        int port = Integer.parseInt(args[1]);
	        
	        if(port <= 0 || port > 65535) {
	            System.out.println("Port number has to be within proper range");
	        } else {
	            regList.add(new RegPacket(args[0], port));
	        }
	    } catch (NumberFormatException e) {
	        System.out.println("Port has to be a number.");
	    }
	}
	
	/**
	 * The main function. 
	 * @param args Has the HostName and PortNumber of the registry to interface 
	 *             to.
	 */
	public static void main(String[] args){
	    
	    if(args.length != 2) {
	        System.out.println("Wrong arguments.");
	        System.out.println("Use ./run_client RegistryHostName "
	                + "RegistryPort");
	        System.exit(1);
	    }
	    
	    addToRegList(args);
		prompt();
	}
}
